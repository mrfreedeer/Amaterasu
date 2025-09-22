#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Interfaces/PipelineState.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Interfaces/CommandList.hpp"
#include "Engine/Renderer/Interfaces/Buffer.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Network/RemoteConsole.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Game//EngineBuildPreferences.hpp"

Rgba8 const DevConsole::ERROR_COLOR = Rgba8(255, 0, 0, 255);
Rgba8 const DevConsole::WARNING_COLOR = Rgba8(255, 255, 0, 255);
Rgba8 const DevConsole::INFO_MAJOR_COLOR = Rgba8(0, 255, 0, 255);
Rgba8 const DevConsole::INFO_MINOR_COLOR = Rgba8(0, 0, 255, 255);

DevConsole* g_theConsole = nullptr;

DevConsole::DevConsole(DevConsoleConfig const& config) :
	m_config(config),
	m_mode(config.m_mode),
	m_maxCommandHistory(config.m_maxCommandHistory)
{
	SubscribeEventCallbackFunction("HandleKeyPressedDev", Event_KeyPressed);
	SubscribeEventCallbackFunction("HandleCharInputDev", Event_CharInput);
	SubscribeEventCallbackFunction("Clear", Command_Clear);
	SubscribeEventCallbackFunction("Help", Command_Help);
	SubscribeEventCallbackFunction("PasteText", Command_Paste_Text);
	SubscribeEventCallbackFunction("ExecuteXMLFile", this, &DevConsole::EventExecuteXMLFile);
	m_caretStopwatch.Start(&m_clock, 0.5f);
	m_commandHistory.resize(m_maxCommandHistory);
	m_historyIndex = 0;
	m_scrollingIndex = m_historyIndex;

	Renderer* renderer =  m_config.m_renderer;
}

DevConsole::~DevConsole()
{
}

void DevConsole::Startup()
{
	m_linesMutex.lock();
	m_lines.clear();
	m_lines.reserve(600);
	m_linesMutex.unlock();

	AddLine(INFO_MINOR_COLOR, "Type help for a list of commands");

#if defined(ENGINE_USE_NETWORK)
	RemoteConsoleConfig remoteConfig;
	remoteConfig.m_console = this;

	m_remoteConsole = new RemoteConsole(remoteConfig);
	m_remoteConsole->Startup();
#endif

	DescriptorHeapDesc desc = {};
	desc.m_debugName = "DevConsoleRscHeap";
	desc.m_flags = DescriptorHeapFlags::ShaderVisible;
	desc.m_heap = nullptr;
	// 0. Default Tex
	// 1. Font Rendering
	// 2. CBV
	// 3. Model
	desc.m_numDescriptors = 4;	
	desc.m_type = DescriptorHeapType::CBV_SRV_UAV;

	m_rscHeap = m_config.m_renderer->CreateDescriptorHeap(desc, desc.m_debugName.c_str());

	desc.m_debugName = "DevConsoleSamplerHeap";
	desc.m_type = DescriptorHeapType::Sampler;
	desc.m_numDescriptors = 1;
	
	m_samplerHeap = m_config.m_renderer->CreateDescriptorHeap(desc, desc.m_debugName.c_str());
	m_config.m_renderer->CreateSampler(m_samplerHeap->GetNextCPUHandle().ptr, SamplerMode::POINTCLAMP);

	// Create layout VBuffer as it will always have the same size, but verts may vary
	BufferDesc vBufferDesc = {};
	vBufferDesc.m_data = nullptr;
	vBufferDesc.m_debugName = "DevConsoleLayotVBuffer";
	vBufferDesc.m_memoryUsage = MemoryUsage::Dynamic;
	vBufferDesc.m_size = sizeof(Vertex_PCU) * 12;
	vBufferDesc.m_stride.m_strideBytes = sizeof(Vertex_PCU);
	vBufferDesc.m_type = BufferType::Vertex;

	m_layoutVBuffer = m_config.m_renderer->CreateBuffer(vBufferDesc);
}

void DevConsole::Shutdown()
{
#if defined(ENGINE_USE_NETWORK)
	m_remoteConsole->Shutdown();
#endif
	delete m_samplerHeap;
	m_samplerHeap = nullptr;

	delete m_rscHeap;
	m_rscHeap = nullptr;

	delete m_textVBuffer;
	m_textVBuffer = nullptr;

	delete m_layoutVBuffer;
	m_layoutVBuffer = nullptr;
}

void DevConsole::BeginFrame()
{
	bool changeVisibility = m_caretStopwatch.CheckDurationElapsedAndDecrement();
	if (changeVisibility) {
		m_caretVisible = !m_caretVisible;
	}
}

void DevConsole::EndFrame()
{
	m_frameNumber++;
#if defined(ENGINE_USE_NETWORK)
	m_remoteConsole->Update();
#endif
}

bool DevConsole::Execute(std::string const& consoleCommandText)
{
	if (consoleCommandText.empty()) return false;
	Strings multipleCommands = SplitStringOnDelimiter(consoleCommandText, '\n');
	for (int commandIndex = 0; commandIndex < multipleCommands.size(); commandIndex++) {
		std::string const& commandString = multipleCommands[commandIndex];
		if (commandString.empty() || IsStringAllWhitespace(commandString)) continue;
		Strings nameArgumentPairs = ProcessCommandLine(commandString);

		std::string commandName = TrimStringCopy(nameArgumentPairs[0]);


		EventArgs commandArgs;
		AddLine(INFO_MINOR_COLOR, consoleCommandText);
		for (int argsIndex = 1; argsIndex < nameArgumentPairs.size(); argsIndex += 2) {
			int nextArg = argsIndex + 1;

			std::string argName = TrimStringCopy(nameArgumentPairs[argsIndex]);
			std::string argValue = "";
			if (nextArg < nameArgumentPairs.size()) {
				argValue = TrimStringCopy(nameArgumentPairs[nextArg]);
				commandArgs.SetValue(argName, argValue);
			}
			else {
				AddLine(DevConsole::WARNING_COLOR, Stringf("Malformed argument: %s", argName.c_str()));
			}

		}

		bool wasEventFired = g_theEventSystem->FireEvent(commandName, commandArgs);

		if (m_historyIndex >= m_maxCommandHistory) m_historyIndex = 0;
		m_commandHistory[m_historyIndex] = commandString;
		m_historyIndex++;
		m_scrollingIndex = m_historyIndex;

		if (!wasEventFired) {
			AddLine(DevConsole::WARNING_COLOR, "Command not found/fired");
			return false;
		}
	}

	return true;
}

bool DevConsole::EventExecuteXMLFile(EventArgs& args)
{
	std::string fileName = args.GetValue("filename", "unnamed");
	if(fileName == "unnamed") return false;

	return ExecuteXmlCommandScriptFile(fileName);
}

bool DevConsole::ExecuteXmlCommandScriptNode(XMLElement const& cmdScriptXmlElement)
{

	bool wasAnyExecuted = false;

	std::string cmdString = "";
	cmdString += cmdScriptXmlElement.Name();

	tinyxml2::XMLAttribute const* currentAttribute = cmdScriptXmlElement.FirstAttribute();

	while (currentAttribute) {
		cmdString += Stringf(" %s=", currentAttribute->Name());
		cmdString += Stringf("\"%s\" ", currentAttribute->Value());

		currentAttribute = currentAttribute->Next();
	}

	wasAnyExecuted = Execute(cmdString);

	return wasAnyExecuted;
}

bool DevConsole::ExecuteXmlCommandScriptFile(std::filesystem::path const& filePath)
{
	tinyxml2::XMLDocument xmlDoc;
	std::string fileString = filePath.string().c_str();
	XMLError loadConfigStatus = xmlDoc.LoadFile(filePath.string().c_str());
	GUARANTEE_OR_DIE(loadConfigStatus == XMLError::XML_SUCCESS, "XML COMMAND FILE DOES NOT EXIST OR CANNOT BE FOUND");

	XMLElement const* currentElement = xmlDoc.FirstChildElement("CommandScript")->FirstChildElement();
	bool wasAnyExecuted = false;

	while (currentElement) {
		bool wasExecutedSuccessfully = ExecuteXmlCommandScriptNode(*currentElement);
		wasAnyExecuted = wasAnyExecuted || wasExecutedSuccessfully;
		currentElement = currentElement->NextSiblingElement();
	}

	return wasAnyExecuted;
}

void DevConsole::AddLine(Rgba8 const& color, std::string const& text)
{
	m_linesMutex.lock();
	m_lines.emplace_back(color, text, m_frameNumber);
	m_linesMutex.unlock();
}

void DevConsole::Render(AABB2 const& bounds, CommandList* cmdListOverride)
{
	bool hasAnyCmdList = (m_config.m_cmdList || cmdListOverride);
	if (m_mode == DevConsoleMode::HIDDEN || !hasAnyCmdList) return;

	CommandList* usedCmdList = m_config.m_cmdList;
	if (cmdListOverride) {
		usedCmdList = cmdListOverride;
	}

	std::string fullFontPath = "Data/Images/" + m_config.m_font;

	static BitmapFont* usedFont = m_config.m_renderer->CreateOrGetBitmapFont(fullFontPath.c_str());
	static bool copiedDescriptors = false;

	if (!copiedDescriptors) {
		copiedDescriptors = true;
		
		Texture* defaultTex = m_config.m_renderer->GetDefaultTexture();
		Texture* squirrelFontTex = &usedFont->GetTexture();
		Buffer* defaultModelBuffer = m_config.m_renderer->GetDefaultModelBuffer();

		size_t defaultTexSRV = defaultTex->GetShaderResourceView()->m_descriptor.ptr;
		size_t texSRV = squirrelFontTex->GetShaderResourceView()->m_descriptor.ptr;
		size_t cbv = m_usedCamera->GetCameraBuffer()->GetConstantBufferView()->m_descriptor.ptr;
		size_t modelCBV = defaultModelBuffer->GetConstantBufferView()->m_descriptor.ptr;

		m_config.m_renderer->CopyDescriptor(m_rscHeap, defaultTexSRV);
		m_config.m_renderer->CopyDescriptor(m_rscHeap, texSRV, 1);
		m_config.m_renderer->CopyDescriptor(m_rscHeap, cbv, 2);
		m_config.m_renderer->CopyDescriptor(m_rscHeap, modelCBV, 3);
	}

	Render_OpenFull(bounds, *usedCmdList, *usedFont, m_config.m_fontAspect);
}

DevConsoleMode DevConsole::GetMode() const
{
	return m_mode;
}

void DevConsole::SetMode(DevConsoleMode newMode)
{
	m_mode = newMode;
	if (m_mode == DevConsoleMode::SHOWN) {
		m_caretStopwatch.Restart();
	}
}

void DevConsole::SetCommandList(CommandList* cmdList)
{
	m_config.m_cmdList = cmdList;
}

void DevConsole::SetRenderer(Renderer* renderer)
{
	m_config.m_renderer = renderer;
}

void DevConsole::SetPSO(PipelineState* pso)
{
	m_pso = pso;
}

void DevConsole::SetCamera(Camera const& camera)
{
	m_usedCamera = &camera;
}

void DevConsole::ToggleMode(DevConsoleMode mode)
{
	switch (m_mode)
	{
	case DevConsoleMode::HIDDEN:
		m_mode = mode;
		break;
	case DevConsoleMode::SHOWN:
		m_mode = DevConsoleMode::HIDDEN;
		break;
	default:
		m_mode = DevConsoleMode::HIDDEN;
		break;
	}

	if (m_mode == DevConsoleMode::SHOWN) {
		m_caretStopwatch.Restart();
	}
}

void DevConsole::Clear()
{
	m_linesMutex.lock();
	m_lines.clear();
	m_linesMutex.unlock();
}

void DevConsole::Render_OpenFull(AABB2 const& bounds, CommandList& cmdList, BitmapFont& font, float fontAspect)
{
	AABB2 inputBounds = bounds;
	float cellHeight = bounds.m_maxs.y / m_config.m_maxLinesShown;

	inputBounds.m_maxs.y = cellHeight;

	float minGroupTextHeight = bounds.m_mins.y + cellHeight;

	std::vector<Vertex_PCU> overlayVerts;

	overlayVerts.reserve(12);

	AddVertsForAABB2D(overlayVerts, bounds, Rgba8::TRANSPARENT_BLACK);
	AddVertsForAABB2D(overlayVerts, inputBounds, Rgba8::TRANSPARENT_WHITE);

	std::vector<Vertex_PCU> textVerts;
	textVerts.reserve(m_lines.size() * 40);

	int roundedUpMaxLinesShown = RoundDownToInt(m_config.m_maxLinesShown) + 2;
	float lineWidth = 0;

	m_linesMutex.lock();
	for (int lineIndex = (int)m_lines.size() - 1; lineIndex >= 0; lineIndex-- && roundedUpMaxLinesShown >= 0, roundedUpMaxLinesShown--) {
		DevConsoleLine const& line = m_lines[lineIndex];
		lineWidth = font.GetTextWidth(cellHeight, line.m_text);

		AABB2 lineAABB2(Vec2::ZERO, Vec2(lineWidth, cellHeight));
		bounds.AlignABB2WithinBounds(lineAABB2, Vec2(0.0f, 1.0f));

		lineAABB2.m_mins.y = minGroupTextHeight + (((int)m_lines.size() - 1 - lineIndex) * cellHeight);

		font.AddVertsForTextInBox2D(textVerts, lineAABB2, cellHeight, line.m_text, line.m_color, fontAspect, Vec2::ZERO, TextBoxMode::OVERRUN);
	}

	m_linesMutex.unlock();

	m_layoutVBuffer->CopyToBuffer(overlayVerts.data(), overlayVerts.size() * sizeof(Vertex_PCU));

	size_t textVBufferSize = textVerts.size() * sizeof(Vertex_PCU);

	ResizeOrCopyToBuffer(textVerts.data(), textVBufferSize, m_textVBuffer);


	//#TODO DX12 FIXTHIS
	cmdList.BindPipelineState(m_pso);
	DescriptorHeap* usedHeaps[2] = {m_rscHeap, m_samplerHeap};
	cmdList.SetDescriptorHeaps(2, usedHeaps);

	cmdList.SetDescriptorTable(PARAM_TEXTURES, m_rscHeap->GetGPUHandleHeapStart(), PipelineType::Graphics);
	cmdList.SetDescriptorTable(PARAM_CAMERA_BUFFERS, m_rscHeap->GetGPUHandleAtOffset(2), PipelineType::Graphics);
	cmdList.SetDescriptorTable(PARAM_MODEL_BUFFERS, m_rscHeap->GetGPUHandleAtOffset(3), PipelineType::Graphics);
	cmdList.SetDescriptorTable(PARAM_SAMPLERS, m_samplerHeap->GetGPUHandleHeapStart(), PipelineType::Graphics);

	unsigned int drawConstants[16] = {0,0,0};
	cmdList.SetGraphicsRootConstants(16, drawConstants);

	cmdList.SetVertexBuffers(&m_layoutVBuffer, 1);
	cmdList.DrawInstance((unsigned int)overlayVerts.size(), 1, 0, 0);

	drawConstants[1] = {1};
	cmdList.SetGraphicsRootConstants(16, drawConstants);
	cmdList.SetVertexBuffers(&m_textVBuffer, 1);
	cmdList.DrawInstance((unsigned int)textVerts.size(), 1, 0, 0);


	/*renderer.BindTexture(nullptr);
	renderer.SetBlendMode(BlendMode::ALPHA);
	renderer.DrawVertexArray(overlayVerts);

	Render_InputCaret(renderer, font, fontAspect, cellHeight);

	renderer.BindTexture(&font.GetTexture());
	renderer.DrawVertexArray(textVerts);*/

	Render_UserInput(cmdList, font, fontAspect, cellHeight);

#if defined(ENGINE_USE_NETWORK)
	m_remoteConsole->Render(renderer);
#endif
}

void DevConsole::Render_InputCaret(CommandList& cmdList, BitmapFont& font, float fontAspect, float cellHeight)
{
	std::string strAtCaretPos = m_inputText.substr(0, m_caretPosition);
	float inputTextWidth = font.GetTextWidth(cellHeight, strAtCaretPos, fontAspect);

	AABB2 caretAABB2(Vec2::ZERO, Vec2(2.0f, cellHeight));
	caretAABB2.Translate(Vec2(inputTextWidth, 0.0f));

	std::vector<Vertex_PCU> caretVertexes;
	caretVertexes.reserve(6);

	Rgba8 caretColor = Rgba8::GREEN;
	caretColor.a = (m_caretVisible) ? 255 : 0;

	AddVertsForAABB2D(caretVertexes, caretAABB2, caretColor);
	//#TODO DX12 FIXTHIS

	//renderer.DrawVertexArray(caretVertexes);
}

void DevConsole::Render_UserInput(CommandList& cmdList, BitmapFont& font, float fontAspect, float cellHeight)
{
	std::vector<Vertex_PCU> userInputTextVerts;

	float lineWidth = font.GetTextWidth(cellHeight, m_inputText);
	AABB2 inputLineAABB2(Vec2::ZERO, Vec2(lineWidth, cellHeight));
	font.AddVertsForTextInBox2D(userInputTextVerts, inputLineAABB2, cellHeight, m_inputText, Rgba8::CYAN, fontAspect, Vec2::ZERO, TextBoxMode::OVERRUN);
	//#TODO DX12 FIXTHIS

	//renderer.DrawVertexArray(userInputTextVerts);
}

void DevConsole::ResizeOrCopyToBuffer(void* data, size_t totalSize, Buffer*& buffer)
{
	// VBuffer does not exist or is too small
	if (!buffer || (totalSize > buffer->GetSize())) {
		if (buffer) delete buffer;

		BufferDesc vBufferDesc = {};
		vBufferDesc.m_data = data;
		vBufferDesc.m_debugName = "DevConsoleTextVBuffer";
		vBufferDesc.m_memoryUsage = MemoryUsage::Dynamic;
		vBufferDesc.m_size = totalSize;
		vBufferDesc.m_stride.m_strideBytes = sizeof(Vertex_PCU);
		vBufferDesc.m_type = BufferType::Vertex;

		buffer = m_config.m_renderer->CreateBuffer(vBufferDesc);
	}
	else {
		buffer->CopyToBuffer(data, totalSize);
	}
}

Strings DevConsole::ProcessCommandLine(std::string const& commandLine) const
{
	Strings processedCmd;
	// Search for command name

	int prevIndex = 0;
	int currentIndex = 0;
	std::string commandName = "";
	bool breakString = false;

	for (; currentIndex < commandLine.size() && !breakString; currentIndex++) {
		if (std::isspace(commandLine[currentIndex]) || (currentIndex == commandLine.size() - 1)) {
			if (currentIndex == commandLine.size() - 1) {
				currentIndex++;
			}
			commandName = commandLine.substr(prevIndex, currentIndex);
			breakString = true;
		}
	}

	processedCmd.push_back(commandName);

	prevIndex = currentIndex;

	bool foundArgName = false;
	bool argValueByQuote = false;
	breakString = false;
	std::string argName = "";
	for (; currentIndex < commandLine.size(); currentIndex++) {
		char const& currentChar = commandLine[currentIndex];

		if ((!foundArgName) && (currentChar == '=')) {
			argName = std::string(&commandLine[prevIndex], &commandLine[currentIndex]);
			prevIndex = currentIndex + 1;

			processedCmd.push_back(argName);
			int nextIndex = currentIndex + 1;
			if (nextIndex < commandLine.size() && (commandLine[nextIndex] == '"')) {
				argValueByQuote = true;
				prevIndex++;
			}

			currentIndex = nextIndex;
			foundArgName = true;
		}

		if (foundArgName) {
			if (argValueByQuote) {
				if (currentChar == '"') {
					breakString = true;
				}
			}
			else {
				if (std::isspace(currentChar) || (currentIndex == commandLine.size() - 1)) {
					breakString = true;
					if (currentIndex == commandLine.size() - 1) {
						currentIndex++;
					}
				}
			}

			if (breakString) {
				std::string argValue = std::string(&commandLine[prevIndex], &commandLine[currentIndex]);
				processedCmd.push_back(argValue);
				foundArgName = false;
				argValueByQuote = false;
				breakString = false;
				prevIndex = currentIndex + 1;
			}
		}

		if (!foundArgName && (currentIndex == commandLine.size() - 1)) {
			if (prevIndex < currentIndex) {
				std::string incompleteArg = std::string(&commandLine[prevIndex], &commandLine[currentIndex]);
				processedCmd.push_back(incompleteArg);
			}
		}
	}



	return processedCmd;
}

DevConsoleLine::DevConsoleLine(Rgba8 const& color, std::string const& text, int frameNumber) :
	m_color(color),
	m_text(text),
	m_frameNumber(frameNumber)
{
}

bool DevConsole::Command_Test(EventArgs& args)
{
	UNUSED(args);
	g_theConsole->AddLine(DevConsole::INFO_MAJOR_COLOR, "Loud and Clear");
	return false;
}

bool DevConsole::Event_KeyPressed(EventArgs& args)
{
	if (g_theConsole->m_mode == DevConsoleMode::HIDDEN) return false;

	unsigned short charCode = args.GetValue("inputChar", (unsigned short)0);
	int& caretPos = g_theConsole->m_caretPosition;
	int& scrollingIndex = g_theConsole->m_scrollingIndex;
	std::string& inputText = g_theConsole->m_inputText;

	switch (charCode) {
	case 8: { // Backspace
		int posToErase = caretPos - 1;
		if (posToErase < 0) return false;
		inputText.erase(posToErase, 1);
		caretPos--;
		break;
	}
	case 13: { // Enter
		g_theConsole->Execute(inputText);
		inputText.clear();
		caretPos = 0;
		break;
	}
	case 27: { // Esc
		inputText.clear();
		caretPos = 0;
		break;
	}
	case 35: { // End
		caretPos = static_cast<int>(inputText.length());
		break;
	}
	case 36: { // Home
		caretPos = 0;
		break;
	}
	case 37: {  // Left Arrow
		caretPos--;
		if (caretPos < 0)caretPos = 0;
		break;
	}
	case 38: { // Up Arrow
		scrollingIndex--;
		if (scrollingIndex < 0) scrollingIndex = g_theConsole->m_historyIndex - 1;
		if (scrollingIndex < 0) scrollingIndex = 0; // m_historyIndex might be -1

		inputText = g_theConsole->m_commandHistory[scrollingIndex];
		caretPos = static_cast<int>(inputText.length());
		break;
	}
	case 39: { // Right Arrow
		caretPos++;
		if (caretPos > inputText.length()) caretPos = static_cast<int>(inputText.length());
		break;
	}
	case 40: { // Down Arrow
		if (scrollingIndex + 1 >= g_theConsole->m_historyIndex) scrollingIndex = -1;
		if (scrollingIndex == -1) {
			inputText.clear();
			caretPos = 0;
			return false;
		}
		scrollingIndex++;

		inputText = g_theConsole->m_commandHistory[scrollingIndex];
		caretPos = static_cast<int>(inputText.length());
		break;
	}
	case 46: { // Delete
		int posToErase = caretPos;
		if (posToErase > inputText.length()) return false;
		inputText.erase(posToErase, 1);
		break;
	}
	}

	return false;
}

bool DevConsole::Event_CharInput(EventArgs& args)
{
	if (g_theConsole->m_mode == DevConsoleMode::HIDDEN) return false;

	int charCode = args.GetValue("inputChar", 0);
	int& caretPos = g_theConsole->m_caretPosition;
	bool validCharCode = (charCode >= 32) && (charCode <= 126) && (charCode != '`') && (charCode != '~');
	if (!validCharCode) return false;

	g_theConsole->m_inputText.insert(caretPos, 1, char(charCode));
	g_theConsole->m_caretPosition++;

	return true;
}

bool DevConsole::Command_Clear(EventArgs& args)
{
	UNUSED(args);
	g_theConsole->Clear();
	return false;
}

bool DevConsole::Command_Help(EventArgs& args)
{
	std::vector<std::string> registeredEventNames;
	g_theEventSystem->GetRegisteredEventNames(registeredEventNames);

	std::string filter = args.GetValue("Filter", "NoFilter");

	g_theConsole->AddLine(DevConsole::INFO_MAJOR_COLOR, Stringf("# Registered Commands [%d] (case insensitive) #", registeredEventNames.size()));
	if (filter != "NoFilter") {
		for (int stringIndex = 0; stringIndex < registeredEventNames.size(); stringIndex++) {
			if (registeredEventNames[stringIndex].find(filter) != std::string::npos) {
				g_theConsole->AddLine(DevConsole::INFO_MINOR_COLOR, registeredEventNames[stringIndex]);
			}
		}
	}
	else {
		for (int stringIndex = 0; stringIndex < registeredEventNames.size(); stringIndex++) {
			g_theConsole->AddLine(DevConsole::INFO_MINOR_COLOR, registeredEventNames[stringIndex]);
		}
	}

	return false;

}

bool DevConsole::Command_Paste_Text(EventArgs& args)
{
	if (g_theConsole->GetMode() == DevConsoleMode::SHOWN) {
		std::string pasteText = args.GetValue("PasteCmdText", "");
		g_theConsole->m_inputText += pasteText;
		g_theConsole->m_caretPosition = (int)pasteText.length();
	}


	return false;
}
