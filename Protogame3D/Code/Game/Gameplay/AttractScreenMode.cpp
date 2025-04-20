#include "Game/Gameplay/AttractScreenMode.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Interfaces/Buffer.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"


AttractScreenMode::~AttractScreenMode()
{

}

void AttractScreenMode::Startup()
{
	GameMode::Startup();
	RenderContextDesc ctxDesc = {};
	CommandListDesc cmdListDesc = {};
	cmdListDesc.m_initialState = nullptr;
	cmdListDesc.m_type = CommandListType::DIRECT;
	cmdListDesc.m_debugName = "AttractGraphics";

	ctxDesc.m_cmdListDesc = cmdListDesc;
	ctxDesc.m_renderer = g_theRenderer;
	
	DescriptorHeapDesc heapDesc = {};
	heapDesc.m_debugName = "AttractScreen Heap";
	heapDesc.m_flags = DescriptorHeapFlags::ShaderVisible;
	heapDesc.m_numDescriptors = 128;
	heapDesc.m_type = DescriptorHeapType::CBV_SRV_UAV;
	unsigned int descriptorCounts[(int)DescriptorHeapType::NUM_TYPES] = { 128, 1, 1, 8 };

	ctxDesc.m_descriptorCounts = descriptorCounts;

	m_renderContext = new RenderContext(ctxDesc);

	cmdListDesc.m_type = CommandListType::COPY;
	cmdListDesc.m_debugName = "AttractCopy";

	m_copyCmdLists = new CommandList*[2];
	// For double buffering
	m_copyCmdLists[0] = g_theRenderer->CreateCommandList(cmdListDesc);
	m_copyCmdLists[1] = g_theRenderer->CreateCommandList(cmdListDesc);

	m_currentText = g_gameConfigBlackboard.GetValue("GAME_TITLE", "Protogame3D");
	m_startTextColor = GetRandomColor();
	m_endTextColor = GetRandomColor();
	m_transitionTextColor = true;

	if (g_sounds[GAME_SOUND::CLAIRE_DE_LUNE] != -1) {
		g_soundPlaybackIDs[GAME_SOUND::CLAIRE_DE_LUNE] = g_theAudio->StartSound(g_sounds[GAME_SOUND::CLAIRE_DE_LUNE]);
	}

	DebugRenderClear();

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

	m_testTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");

	// Alpha for font rendering
	psoDesc.m_blendModes[0] = BlendMode::ALPHA;
	psoDesc.m_debugName ="Alpha2D";
	m_alphaDefault2D = g_theRenderer->CreatePipelineState(psoDesc);

	AABB2 UICamBounds = m_UICamera.GetCameraBounds();
	Vec2 camSize = UICamBounds.GetDimensions();

	Vertex_PCU triangleVertices[] =
	{
		Vertex_PCU(Vec3(camSize.x * 0.25f, camSize.y * 0.25f, 0.5f), Rgba8(255, 0, 0, 255), Vec2(0.0f, 0.0f)),
		Vertex_PCU(Vec3(camSize.x * 0.75f, camSize.y * 0.25f, 0.5f), Rgba8(0, 255, 0, 255), Vec2(1.0f, 0.0f)),
		Vertex_PCU(Vec3(camSize.x * 0.5f, camSize.y * 0.75f, 0.5f), Rgba8(0, 0, 255, 255), Vec2(0.5f, 1.0f))
	};
	BufferDesc bufferDesc = {};
	bufferDesc.m_data = triangleVertices;
	bufferDesc.m_debugName = "TriVertBuffer";
	bufferDesc.m_memoryUsage = MemoryUsage::Default;
	bufferDesc.m_size = sizeof(Vertex_PCU) * 3;
	bufferDesc.m_stride.m_strideBytes = sizeof(Vertex_PCU);

	m_triangleVertsBuffer = g_theRenderer->CreateBuffer(bufferDesc);
	m_resources.push_back(m_triangleVertsBuffer);

	bufferDesc.m_memoryUsage = MemoryUsage::Dynamic;
	Buffer* intermediateBuffer = g_theRenderer->CreateBuffer(bufferDesc);

	CommandList* copyCmdList = m_copyCmdLists[m_renderContext->GetBufferIndex()];
	copyCmdList->CopyBuffer(intermediateBuffer, m_triangleVertsBuffer);

	m_copyFence = g_theRenderer->CreateFence(CommandListType::COPY);
	m_frameFence = g_theRenderer->CreateFence(CommandListType::DIRECT);

	copyCmdList->Close();
	g_theRenderer->ExecuteCmdLists(CommandListType::COPY, 1, &copyCmdList);
	unsigned int fenceVal = m_copyFence->Signal();

	m_copyFence->Wait(fenceVal);

	// Waiting for the copying to be done
	g_theRenderer->WaitForOtherQueue(CommandListType::DIRECT, m_copyFence);

	delete intermediateBuffer;

	// Create descriptors for resources
	DescriptorHeap* samplerHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::Sampler);
	DescriptorHeap* resourcesHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE nextSamplerHandle = samplerHeap->GetNextCPUHandle();
	m_defaultSampler = g_theRenderer->CreateSampler(nextSamplerHandle.ptr, SamplerMode::BILINEARCLAMP);

	BufferDesc cameraBuffDesc = {};
	CameraConstants cameraConstants = m_UICamera.GetCameraConstants();
	cameraBuffDesc.m_data = &cameraConstants;
	cameraBuffDesc.m_debugName = "UICamConst";
	cameraBuffDesc.m_memoryUsage = MemoryUsage::Dynamic;
	cameraBuffDesc.m_size = sizeof(cameraConstants);
	cameraBuffDesc.m_stride.m_strideBytes = sizeof(CameraConstants);
	cameraBuffDesc.m_type = BufferType::Constant;

	m_UICameraBuffer = g_theRenderer->CreateBuffer(cameraBuffDesc);
	g_theRenderer->CreateConstantBufferView(resourcesHeap->GetNextCPUHandle().ptr, m_UICameraBuffer);
	g_theRenderer->CreateConstantBufferView(resourcesHeap->GetNextCPUHandle().ptr, g_theRenderer->GetDefaultModelBuffer());
	g_theRenderer->CreateShaderResourceView(resourcesHeap->GetNextCPUHandle().ptr, g_theRenderer->GetDefaultTexture());

	// It's expected that the cmd list will be closed
	m_renderContext->CloseAll();
}

void AttractScreenMode::Update(float deltaSeconds)
{
	GameMode::Update(deltaSeconds);

	if (!m_useTextAnimation) {
		m_useTextAnimation = true;
		m_currentText = g_gameConfigBlackboard.GetValue("GAME_TITLE", "Protogame3D");
		m_transitionTextColor = true;
		m_startTextColor = GetRandomColor();
		m_endTextColor = GetRandomColor();
	}

	UpdateInput(deltaSeconds);
	m_timeAlive += deltaSeconds;

	if (m_useTextAnimation) {
		UpdateTextAnimation(deltaSeconds, m_currentText, Vec2(m_UICenter.x, m_UISize.y * m_textAnimationPosPercentageTop));
	}

}

void AttractScreenMode::Render()
{
	/// <summary>
	/// ADDD RESOURCE BARRIERS!!!!! AND COPY RT TO BACKBUFFER
	/// </summary>
	m_UICamera.SetColorTarget(m_renderTarget);
	m_UICamera.SetDepthTarget(m_depthTarget);

	m_renderContext->Reset();

	TransitionBarrier resourceBarriers[2] = {};
	resourceBarriers[0] = m_renderTarget->GetTransitionBarrier(ResourceStates::RenderTarget);
	resourceBarriers[1] = m_depthTarget->GetTransitionBarrier(ResourceStates::DepthWrite);
	CommandList* cmdList = m_renderContext->GetCommandList();


	m_renderContext->BeginCamera(m_UICamera);
	{
		DescriptorHeap* cbvSRVUAVHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::CBV_SRV_UAV);
		DescriptorHeap* samplerHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::Sampler);
		DescriptorHeap* rtHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::RenderTargetView);
		cmdList->ResourceBarrier(2, resourceBarriers);

		cmdList->SetTopology(TopologyType::TRIANGLELIST);
		cmdList->ClearRenderTarget(m_renderTarget, Rgba8::BLUE);
		cmdList->ClearDepthStencilView(m_depthTarget, 1.0f);
		cmdList->BindPipelineState(m_opaqueDefault2D);
		cmdList->SetDescriptorSet(m_renderContext->GetDescriptorSet());
		cmdList->SetDescriptorTable(PARAM_CAMERA_BUFFERS, cbvSRVUAVHeap->GetGPUHandleHeapStart(), PipelineType::Graphics);
		cmdList->SetDescriptorTable(PARAM_MODEL_BUFFERS, cbvSRVUAVHeap->GetGPUHandleAtOffset(1), PipelineType::Graphics);
		cmdList->SetDescriptorTable(PARAM_TEXTURES, cbvSRVUAVHeap->GetGPUHandleAtOffset(2), PipelineType::Graphics);
		cmdList->SetDescriptorTable(PARAM_SAMPLERS, samplerHeap->GetGPUHandleHeapStart(), PipelineType::Graphics);

		unsigned int drawConstants[16] = {0};
		cmdList->SetGraphicsRootConstants(16, drawConstants);

		cmdList->SetVertexBuffers(&m_triangleVertsBuffer, 1);
		cmdList->DrawInstance(3, 1, 0, 0);

		resourceBarriers[0] = m_renderTarget->GetTransitionBarrier(ResourceStates::Common);
		cmdList->ResourceBarrier(1, resourceBarriers);
		//g_theRenderer->ClearScreen(Rgba8::BLACK);
		//Material* def2DMat = g_theRenderer->GetDefaultMaterial(false);
		//g_theRenderer->BindMaterial(def2DMat);
		//g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		//if (m_useTextAnimation) {
		//	RenderTextAnimation();
		//}

		//Texture* testTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");
		//std::vector<Vertex_PCU> testTextVerts;
		//AABB2 testTextureAABB2(740.0f, 150.0f, 1040.0f, 450.f);
		//AddVertsForAABB2D(testTextVerts, testTextureAABB2, Rgba8());
		//g_theRenderer->BindTexture(testTexture);
		//g_theRenderer->DrawVertexArray((int)testTextVerts.size(), testTextVerts.data());
		////g_theRenderer->BindTexture(nullptr);


	}
	m_renderContext->EndCamera(m_UICamera);

	DebugRenderWorld(m_worldCamera);
	RenderUI();
	DebugRenderScreen(m_UICamera);

	m_renderContext->EndFrame();

	Texture* backBuffer = g_theRenderer->GetActiveBackBuffer();
	resourceBarriers[0] = m_renderTarget->GetTransitionBarrier(ResourceStates::CopySrc);
	resourceBarriers[1] = backBuffer->GetTransitionBarrier(ResourceStates::CopyDest);
	 
	cmdList->ResourceBarrier(2, resourceBarriers);
	cmdList->CopyResource(backBuffer, m_renderTarget);

	resourceBarriers[0]  = backBuffer->GetTransitionBarrier(ResourceStates::Present);
	cmdList->ResourceBarrier(1, resourceBarriers);

	cmdList->Close();

	g_theRenderer->ExecuteCmdLists(CommandListType::DIRECT, 1, &cmdList);

	g_theRenderer->Present();

	unsigned int waitVal = m_frameFence->Signal();
	m_frameFence->Wait(waitVal);

	g_theRenderer->HandleStateDecay(m_resources);
}


void AttractScreenMode::Shutdown()
{
	delete m_opaqueDefault2D;
	m_opaqueDefault2D = nullptr;

	delete m_alphaDefault2D;
	m_alphaDefault2D = nullptr;
	
	// Should destroy textures?? probably not, just null them
	m_testTexture = nullptr;

	delete m_triangleVertsBuffer;
	m_triangleVertsBuffer = nullptr;

    delete m_defaultSampler;
    m_defaultSampler = nullptr;

	
	GameMode::Shutdown();
}

void AttractScreenMode::UpdateInput(float deltaSeconds)
{
	UpdateDeveloperCheatCodes(deltaSeconds);

	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	bool exitAttractMode = false;
	exitAttractMode = g_theInput->WasKeyJustPressed(KEYCODE_SPACE);
	exitAttractMode = exitAttractMode || controller.WasButtonJustPressed(XboxButtonID::Start);
	exitAttractMode = exitAttractMode || controller.WasButtonJustPressed(XboxButtonID::A);

	if (exitAttractMode) {
		m_game->EnterGameMode();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_game->HandleQuitRequested();
	}
}

void AttractScreenMode::RenderUI() const
{
	m_renderContext->BeginCamera(m_UICamera);
	if (m_useTextAnimation) {

		RenderTextAnimation();
	}

	//AABB2 devConsoleBounds(m_UICamera.GetOrthoBottomLeft(), m_UICamera.GetOrthoTopRight());
	//AABB2 screenBounds(m_UICamera.GetOrthoBottomLeft(), m_UICamera.GetOrthoTopRight());

	//Material* default2DMat = g_theMaterialSystem->GetMaterialForName("Default2DMaterial");
	//g_theRenderer->BindMaterial(default2DMat);

	//std::vector<Vertex_PCU> gameInfoVerts;

	//g_theConsole->Render(devConsoleBounds);
	//g_theRenderer->EndCamera(m_UICamera);

	m_renderContext->EndCamera(m_UICamera);


}

void AttractScreenMode::UpdateTextAnimation(float deltaTime, std::string text, Vec2 textLocation)
{
	m_timeTextAnimation += deltaTime;
	m_textAnimTriangles.clear();

	Rgba8 usedTextColor = m_textAnimationColor;
	if (m_transitionTextColor) {

		float timeAsFraction = GetFractionWithin(m_timeTextAnimation, 0, m_textAnimationTime);

		usedTextColor.r = static_cast<unsigned char>(Interpolate(m_startTextColor.r, m_endTextColor.r, timeAsFraction));
		usedTextColor.g = static_cast<unsigned char>(Interpolate(m_startTextColor.g, m_endTextColor.g, timeAsFraction));
		usedTextColor.b = static_cast<unsigned char>(Interpolate(m_startTextColor.b, m_endTextColor.b, timeAsFraction));
		usedTextColor.a = static_cast<unsigned char>(Interpolate(m_startTextColor.a, m_endTextColor.a, timeAsFraction));

	}

	g_squirrelFont->AddVertsForText2D(m_textAnimTriangles, Vec2::ZERO, m_textCellHeightAttractScreen, text, usedTextColor, 0.6f);

	float fullTextWidth = g_squirrelFont->GetTextWidth(m_textCellHeightAttractScreen, text, 0.6f);
	float textWidth = GetTextWidthForAnim(fullTextWidth);

	Vec2 iBasis = GetIBasisForTextAnim();
	Vec2 jBasis = GetJBasisForTextAnim();

	TransformText(iBasis, jBasis, Vec2(textLocation.x - textWidth * 0.5f, textLocation.y));

	if (m_timeTextAnimation >= m_textAnimationTime) {
		m_useTextAnimation = false;
		m_timeTextAnimation = 0.0f;
	}

}

void AttractScreenMode::RenderTextAnimation() const
{
	if (m_textAnimTriangles.size() > 0) {
		//g_theRenderer->BindTexture(&g_squirrelFont->GetTexture());
		//g_theRenderer->DrawVertexArray(int(m_textAnimTriangles.size()), m_textAnimTriangles.data());
	}
}

float AttractScreenMode::GetTextWidthForAnim(float fullTextWidth)
{
	float timeAsFraction = GetFractionWithin(m_timeTextAnimation, 0.0f, m_textAnimationTime);
	float textWidth = 0.0f;
	if (timeAsFraction < m_textMovementPhaseTimePercentage) {
		if (timeAsFraction < m_textMovementPhaseTimePercentage * 0.5f) {
			textWidth = RangeMapClamped(timeAsFraction, 0.0f, m_textMovementPhaseTimePercentage * 0.5f, 0.0f, fullTextWidth);
		}
		else {
			return fullTextWidth;
		}
	}
	else if (timeAsFraction > m_textMovementPhaseTimePercentage && timeAsFraction < 1 - m_textMovementPhaseTimePercentage) {
		return fullTextWidth;
	}
	else {
		if (timeAsFraction < 1.0f - m_textMovementPhaseTimePercentage * 0.5f) {
			return fullTextWidth;
		}
		else {
			textWidth = RangeMapClamped(timeAsFraction, 1.0f - m_textMovementPhaseTimePercentage * 0.5f, 1.0f, fullTextWidth, 0.0f);
		}
	}
	return textWidth;
}

Vec2 const AttractScreenMode::GetIBasisForTextAnim()
{
	float timeAsFraction = GetFractionWithin(m_timeTextAnimation, 0.0f, m_textAnimationTime);
	float iScale = 0.0f;
	if (timeAsFraction < m_textMovementPhaseTimePercentage) {
		if (timeAsFraction < m_textMovementPhaseTimePercentage * 0.5f) {
			iScale = RangeMapClamped(timeAsFraction, 0.0f, m_textMovementPhaseTimePercentage * 0.5f, 0.0f, 1.0f);
		}
		else {
			iScale = 1.0f;
		}
	}
	else if (timeAsFraction > m_textMovementPhaseTimePercentage && timeAsFraction < 1 - m_textMovementPhaseTimePercentage) {
		iScale = 1.0f;
	}
	else {
		if (timeAsFraction < 1.0f - m_textMovementPhaseTimePercentage * 0.5f) {
			iScale = 1.0f;
		}
		else {
			iScale = RangeMapClamped(timeAsFraction, 1.0f - m_textMovementPhaseTimePercentage * 0.5f, 1.0f, 1.0f, 0.0f);
		}
	}

	return Vec2(iScale, 0.0f);
}

Vec2 const AttractScreenMode::GetJBasisForTextAnim()
{
	float timeAsFraction = GetFractionWithin(m_timeTextAnimation, 0.0f, m_textAnimationTime);
	float jScale = 0.0f;
	if (timeAsFraction < m_textMovementPhaseTimePercentage) {
		if (timeAsFraction < m_textMovementPhaseTimePercentage * 0.5f) {
			jScale = 0.05f;
		}
		else {
			jScale = RangeMapClamped(timeAsFraction, m_textMovementPhaseTimePercentage * 0.5f, m_textMovementPhaseTimePercentage, 0.05f, 1.0f);

		}
	}
	else if (timeAsFraction > m_textMovementPhaseTimePercentage && timeAsFraction < 1 - m_textMovementPhaseTimePercentage) {
		jScale = 1.0f;
	}
	else {
		if (timeAsFraction < 1.0f - m_textMovementPhaseTimePercentage * 0.5f) {
			jScale = RangeMapClamped(timeAsFraction, 1.0f - m_textMovementPhaseTimePercentage, 1.0f - m_textMovementPhaseTimePercentage * 0.5f, 1.0f, 0.05f);
		}
		else {
			jScale = 0.05f;
		}
	}

	return Vec2(0.0f, jScale);
}

void AttractScreenMode::TransformText(Vec2 iBasis, Vec2 jBasis, Vec2 translation)
{
	for (int vertIndex = 0; vertIndex < int(m_textAnimTriangles.size()); vertIndex++) {
		Vec3& vertPosition = m_textAnimTriangles[vertIndex].m_position;
		TransformPositionXY3D(vertPosition, iBasis, jBasis, translation);
	}
}

