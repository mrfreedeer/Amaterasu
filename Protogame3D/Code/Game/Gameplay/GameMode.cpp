#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Engine/Renderer/Interfaces/Buffer.hpp"
#include "Game/Gameplay/GameMode.hpp"
#include "Game/Framework/GameCommon.hpp"

GameMode::~GameMode()
{

}

void GameMode::Startup()
{
	g_theConsole->SetCamera(m_UICamera);
	m_isCursorHidden = false;
	m_isCursorClipped = false;
	m_isCursorRelative = false;

	Vec3 iBasis(0.0f, -1.0f, 0.0f);
	Vec3 jBasis(0.0f, 0.0f, 1.0f);
	Vec3 kBasis(1.0f, 0.0f, 0.0f);
	m_worldCamera.SetViewToRenderTransform(iBasis, jBasis, kBasis); // Sets view to render to match D11 handedness and coordinate system

	TextureDesc defaultRtDesc = {};
	defaultRtDesc.m_bindFlags = ResourceBindFlagBit::RESOURCE_BIND_SHADER_RESOURCE_BIT | ResourceBindFlagBit::RESOURCE_BIND_RENDER_TARGET_BIT;
	defaultRtDesc.m_clearFormat = TextureFormat::R8G8B8A8_UNORM;
	defaultRtDesc.m_format = TextureFormat::R8G8B8A8_UNORM;
	defaultRtDesc.m_name = "DefaultRT";
	defaultRtDesc.m_dimensions = Window::GetWindowContext()->GetClientDimensions();
	defaultRtDesc.m_stride = sizeof(Rgba8);

	m_renderTarget = g_theRenderer->CreateTexture(defaultRtDesc);

	TextureDesc defaultDrtDesc = {};
	defaultDrtDesc.m_bindFlags = ResourceBindFlagBit::RESOURCE_BIND_SHADER_RESOURCE_BIT | ResourceBindFlagBit::RESOURCE_BIND_DEPTH_STENCIL_BIT;
	defaultDrtDesc.m_clearFormat = TextureFormat::D24_UNORM_S8_UINT;
	defaultDrtDesc.m_format = TextureFormat::R24G8_TYPELESS;
	defaultDrtDesc.m_name = "DefaultDRT";
	defaultDrtDesc.m_dimensions = Window::GetWindowContext()->GetClientDimensions();
	defaultDrtDesc.m_stride = sizeof(float);

	m_depthTarget = g_theRenderer->CreateTexture(defaultDrtDesc);


	m_copyFence = g_theRenderer->CreateFence(CommandListType::COPY);
	m_frameFence = g_theRenderer->CreateFence(CommandListType::DIRECT);
	m_gpuFence = g_theRenderer->CreateFence(CommandListType::DIRECT);

	//// Add these to resource vector to better handle state decay
	//m_resources.push_back(m_renderTarget);
	//m_resources.push_back(m_depthTarget);
}

void GameMode::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	CheckIfWindowHasFocus();
}

void GameMode::Render()
{
	DebugRenderWorld(m_worldCamera);
	RenderUI();
	DebugRenderScreen(m_UICamera);
}

void GameMode::Shutdown()
{
	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity*& entity = m_allEntities[entityIndex];
		if (entity) {
			delete entity;
			entity = nullptr;
		}
	}
	m_allEntities.resize(0);
	m_player = nullptr;

	for (SoundPlaybackID playbackId : g_soundPlaybackIDs) {
		if (playbackId == MISSING_SOUND_ID) continue;

		g_theAudio->StopSound(playbackId);
	}

	delete m_renderContext;
	m_renderContext = nullptr;

	for (unsigned int listIndex = 0; listIndex < g_theRenderer->GetBackBufferCount(); listIndex++) {
		delete m_copyCmdLists[listIndex];
		m_copyCmdLists[listIndex] = nullptr;
	}

	delete[] m_copyCmdLists;

	delete m_copyFence;
	m_copyFence = nullptr;

	delete m_frameFence;
	m_frameFence = nullptr;

	delete m_gpuFence;
	m_gpuFence = nullptr;

	Buffer* worldCameraBuffer = m_worldCamera.GetCameraBuffer();
	m_worldCamera.SetCameraBuffer(nullptr);

	Buffer* UICameraBuffer = m_UICamera.GetCameraBuffer();
	m_UICamera.SetCameraBuffer(nullptr);

	if (worldCameraBuffer) {
		delete worldCameraBuffer;
	}

	if (UICameraBuffer) {
		delete UICameraBuffer;
	}

	delete m_opaqueDefault2D;
	m_opaqueDefault2D = nullptr;

	delete m_alphaDefault2D;
	m_alphaDefault2D = nullptr;

}

void GameMode::UpdateDeveloperCheatCodes(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	XboxController controller = g_theInput->GetController(0);
	Clock& sysClock = Clock::GetSystemClock();

	if (g_theInput->WasKeyJustPressed('T')) {
		sysClock.SetTimeDilation(0.1);
	}

	if (g_theInput->WasKeyJustPressed('O')) {
		Clock::GetSystemClock().StepFrame();
	}

	if (g_theInput->WasKeyJustPressed('P')) {
		sysClock.TogglePause();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F1)) {
		g_drawDebug = !g_drawDebug;
	}


	if (g_theInput->WasKeyJustPressed(KEYCODE_F8)) {
		Shutdown();
		Startup();
	}

	if (g_theInput->WasKeyJustReleased('T')) {
		sysClock.SetTimeDilation(1);
	}

	if (g_theInput->WasKeyJustPressed('K')) {
		g_soundPlaybackIDs[GAME_SOUND::DOOR_KNOCKING_SOUND] = g_theAudio->StartSound(g_sounds[DOOR_KNOCKING_SOUND]);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_TILDE)) {
		g_theConsole->ToggleMode(DevConsoleMode::SHOWN);
		g_theInput->ResetMouseClientDelta();
	}
}

Rgba8 const GameMode::GetRandomColor() const
{
	unsigned char randomR = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomG = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomB = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomA = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	return Rgba8(randomR, randomG, randomB, randomA);
}

void GameMode::CheckIfWindowHasFocus()
{
	bool isDevConsoleShown = g_theConsole->GetMode() == DevConsoleMode::SHOWN;
	if (isDevConsoleShown) {
		g_theInput->ConsumeAllKeysJustPressed();
	}

	if (g_theWindow->HasFocus()) {
		if (m_lostFocusBefore) {
			g_theInput->ResetMouseClientDelta();
			m_lostFocusBefore = false;
		}
		if (isDevConsoleShown) {
			g_theInput->SetMouseMode(false, false, false);
		}
		else {
			g_theInput->SetMouseMode(m_isCursorHidden, m_isCursorClipped, m_isCursorRelative);
		}
	}
	else {
		m_lostFocusBefore = true;
	}
}

void GameMode::UpdateCamera(Camera const& camera, Buffer* cBuffer)
{
	CameraConstants constants = camera.GetCameraConstants();
	cBuffer->CopyToBuffer(&constants, sizeof(constants));
}

void GameMode::UpdateEntities(float deltaSeconds)
{
	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity* entity = m_allEntities[entityIndex];
		entity->Update(deltaSeconds);
	}
}

void GameMode::RenderEntities() const
{
	CommandList* cmdList = m_renderContext->GetCommandList();
	DescriptorHeap* cbvHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::CBV_SRV_UAV);
	unsigned int drawConstants[16] = { 0 };
	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity* entity = m_allEntities[entityIndex];
		if (entity == m_player) continue;

		drawConstants[0] = entity->m_cameraIndex;
		drawConstants[1] = entity->m_modelIndex;
		drawConstants[2] = entity->m_textureIndex;

		cmdList->SetGraphicsRootConstants(16, drawConstants);

		entity->Render(cmdList);
	}
}

void GameMode::RenderUI()
{
	m_UICamera.SetColorTarget(m_renderTarget);
	m_UICamera.SetDepthTarget(m_depthTarget);
	m_renderContext->BeginCamera(m_UICamera);
	CommandList* cmdList = m_renderContext->GetCommandList();
	DescriptorHeap* resourcesHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::CBV_SRV_UAV);
	DescriptorHeap* samplerHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::Sampler);

	unsigned int cameraDescriptorStart= m_renderContext->GetDescriptorStart(PARAM_CAMERA_BUFFERS);
	unsigned int modelDescriptorStart= m_renderContext->GetDescriptorStart(PARAM_MODEL_BUFFERS);

	cmdList->BindPipelineState(m_alphaDefault2D);
	cmdList->SetRenderTargets(1, &m_renderTarget, false, m_depthTarget);
	cmdList->SetDescriptorSet(m_renderContext->GetDescriptorSet());
	// UI Camera will be slot 1
	cmdList->SetDescriptorTable(PARAM_CAMERA_BUFFERS, resourcesHeap->GetGPUHandleAtOffset(cameraDescriptorStart + 1), PipelineType::Graphics);
	cmdList->SetDescriptorTable(PARAM_MODEL_BUFFERS, resourcesHeap->GetGPUHandleAtOffset(modelDescriptorStart), PipelineType::Graphics);
	cmdList->SetDescriptorTable(PARAM_TEXTURES, resourcesHeap->GetGPUHandleHeapStart(), PipelineType::Graphics);
	// Text Sampler will always be allocated at descriptor slot 1
	cmdList->SetDescriptorTable(PARAM_SAMPLERS, samplerHeap->GetGPUHandleAtOffset(1), PipelineType::Graphics);
	cmdList->SetTopology(TopologyType::TRIANGLELIST);
	unsigned int drawConstants[16] = { 0 };
	cmdList->SetGraphicsRootConstants(16, drawConstants);

	AABB2 devConsoleBounds(m_UICamera.GetOrthoBottomLeft(), m_UICamera.GetOrthoTopRight());
	AABB2 screenBounds(m_UICamera.GetOrthoBottomLeft(), m_UICamera.GetOrthoTopRight());

	g_theConsole->Render(devConsoleBounds, cmdList);
	m_renderContext->EndCamera(m_UICamera);

}

void GameMode::CreateRendererObjects(char const* debugName, unsigned int* descriptorCounts)
{
	RenderContextDesc ctxDesc = {};
	CommandListDesc cmdListDesc = {};
	cmdListDesc.m_initialState = nullptr;
	cmdListDesc.m_type = CommandListType::DIRECT;
	cmdListDesc.m_debugName = debugName ;
	cmdListDesc.m_debugName += "Graphics";

	ctxDesc.m_cmdListDesc = cmdListDesc;
	ctxDesc.m_renderer = g_theRenderer;

	cmdListDesc.m_type = CommandListType::COPY;
	cmdListDesc.m_debugName = debugName;
	cmdListDesc.m_debugName += "CopyList";

	m_copyCmdLists = new CommandList * [2];
	// For double buffering
	m_copyCmdLists[0] = g_theRenderer->CreateCommandList(cmdListDesc);
	m_copyCmdLists[1] = g_theRenderer->CreateCommandList(cmdListDesc);

	DescriptorHeapDesc heapDesc = {};
	heapDesc.m_debugName = debugName;
	heapDesc.m_debugName += "Heap";
	heapDesc.m_flags = DescriptorHeapFlags::ShaderVisible;
	heapDesc.m_numDescriptors = 128;
	heapDesc.m_type = DescriptorHeapType::CBV_SRV_UAV;

	ctxDesc.m_descriptorCounts = descriptorCounts;

	unsigned int rscDescriptorCounts = descriptorCounts[(int)DescriptorHeapType::CBV_SRV_UAV];
	unsigned int equalDistribution = (rscDescriptorCounts - 2) / 5; 

	ctxDesc.m_rscDescriptorDistribution[PARAM_CAMERA_BUFFERS] = 2;		// Just 2 needed for world and UI
	ctxDesc.m_rscDescriptorDistribution[PARAM_MODEL_BUFFERS]  = equalDistribution;
	ctxDesc.m_rscDescriptorDistribution[PARAM_DRAW_INFO_BUFFERS] = equalDistribution;
	ctxDesc.m_rscDescriptorDistribution[PARAM_GAME_BUFFERS] = equalDistribution;
	ctxDesc.m_rscDescriptorDistribution[PARAM_TEXTURES] = equalDistribution;
	ctxDesc.m_rscDescriptorDistribution[PARAM_GAME_UAVS] = equalDistribution;
	m_renderContext = new RenderContext(ctxDesc);

	ShaderPipeline defaultLegacy = g_theRenderer->GetEngineShader(EngineShaderPipelines::LegacyForward);
	PipelineStateDesc psoDesc = {};
	psoDesc.m_blendModes[0] = BlendMode::OPAQUE;
	psoDesc.m_byteCodes[ShaderType::Vertex] = defaultLegacy.m_firstShader;
	psoDesc.m_byteCodes[ShaderType::Pixel] = defaultLegacy.m_pixelShader;
	psoDesc.m_cullMode = CullMode::BACK;
	psoDesc.m_debugName = "Opaque2D";
	psoDesc.m_depthEnable = true;
	psoDesc.m_depthFunc = DepthFunc::LESSEQUAL;
	psoDesc.m_depthStencilFormat = TextureFormat::D24_UNORM_S8_UINT;
	psoDesc.m_fillMode = FillMode::SOLID;
	psoDesc.m_renderTargetCount = 1;
	psoDesc.m_renderTargetFormats[0] = TextureFormat::R8G8B8A8_UNORM;
	psoDesc.m_topology = TopologyType::TRIANGLELIST;
	psoDesc.m_type = PipelineType::Graphics;
	psoDesc.m_windingOrder = WindingOrder::COUNTERCLOCKWISE;

	m_opaqueDefault2D = g_theRenderer->CreatePipelineState(psoDesc);

	// Alpha for font rendering
	psoDesc.m_blendModes[0] = BlendMode::ALPHA;
	psoDesc.m_debugName = "Alpha2D";
	m_alphaDefault2D = g_theRenderer->CreatePipelineState(psoDesc);

	BufferDesc cameraBuffDesc = {};
	CameraConstants cameraConstants = m_worldCamera.GetCameraConstants();
	cameraBuffDesc.m_data = &cameraConstants;
	cameraBuffDesc.m_debugName = "UICamConst";
	cameraBuffDesc.m_memoryUsage = MemoryUsage::Dynamic;
	cameraBuffDesc.m_size = sizeof(cameraConstants);
	cameraBuffDesc.m_stride.m_strideBytes = sizeof(CameraConstants);
	cameraBuffDesc.m_type = BufferType::Constant;

	Buffer* worldCameraBuffer = g_theRenderer->CreateBuffer(cameraBuffDesc);
	m_worldCamera.SetCameraBuffer(worldCameraBuffer);

	cameraConstants = m_UICamera.GetCameraConstants();
	cameraBuffDesc.m_data = &cameraConstants;
	Buffer* UICameraBuffer = g_theRenderer->CreateBuffer(cameraBuffDesc);
	m_UICamera.SetCameraBuffer(UICameraBuffer);

	g_theConsole->SetPSO(m_alphaDefault2D);
}
