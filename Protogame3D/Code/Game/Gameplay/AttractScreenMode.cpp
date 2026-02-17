#include "Game/Gameplay/AttractScreenMode.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Interfaces/Buffer.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"


AttractScreenMode::~AttractScreenMode()
{

}

void AttractScreenMode::CreateResources()
{
	// Test triangle
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

	Buffer* intermTestTri = nullptr, *intermTestTex = nullptr;
	m_triangleVertsBuffer = g_theRenderer->CreateDefaultBuffer(bufferDesc, &intermTestTri);

	
	std::vector<Vertex_PCU> testTextVerts;
	AABB2 testTextureAABB2(740.0f, 150.0f, 1040.0f, 450.f);
	AddVertsForAABB2D(testTextVerts, testTextureAABB2, Rgba8());

	BufferDesc testTexBufferDesc = {};
	testTexBufferDesc.m_data = testTextVerts.data();
	testTexBufferDesc.m_debugName = "TestAABBBuffer";
	testTexBufferDesc.m_memoryUsage = MemoryUsage::Default;
	testTexBufferDesc.m_size = sizeof(Vertex_PCU) * testTextVerts.size();
	testTexBufferDesc.m_stride.m_strideBytes = sizeof(Vertex_PCU);

	m_testTexBuffer = g_theRenderer->CreateDefaultBuffer(testTexBufferDesc, &intermTestTex);

	CommandList* copyCmdList = m_copyCmdLists[m_renderContext->GetBufferIndex()];
	copyCmdList->CopyResource(m_triangleVertsBuffer, intermTestTri);
	copyCmdList->CopyResource(m_testTexBuffer, intermTestTex);


	m_testTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png", copyCmdList);
	copyCmdList->Close();
	g_theRenderer->ExecuteCmdLists(CommandListType::COPY, 1, &copyCmdList);
	m_copyFence->SignalGPU();

	// Waiting for the copying to be done
	g_theRenderer->InsertWaitInQueue(CommandListType::DIRECT, m_copyFence);
	//g_theRenderer->InsertWaitInQueue(CommandListType::COPY, m_copyFence);
	DebuggerPrintf("CopyQueueValue: %d", m_copyFence->GetCompletedValue());

	m_copyFence->Wait();
	delete intermTestTri;
	delete intermTestTex;

	// Push into resource management vector
	m_resources.push_back(m_triangleVertsBuffer);
	m_resources.push_back(m_testTexBuffer);
	m_resources.push_back(&g_squirrelFont->GetTexture());

}

void AttractScreenMode::Startup()
{
	GameMode::Startup();
	
	unsigned int descriptorCounts[] = {128, 2, 1, 8};
	CreateRendererObjects("AttractScreen", descriptorCounts);
	CreateResources();
	m_currentText = g_gameConfigBlackboard.GetValue("GAME_TITLE", "Protogame3D");
	m_startTextColor = GetRandomColor();
	m_endTextColor = GetRandomColor();
	m_transitionTextColor = true;

	if (g_sounds[GAME_SOUND::CLAIRE_DE_LUNE] != -1) {
		g_soundPlaybackIDs[GAME_SOUND::CLAIRE_DE_LUNE] = g_theAudio->StartSound(g_sounds[GAME_SOUND::CLAIRE_DE_LUNE]);
	}

	DebugRenderClear();


	// Create descriptors for resources
	DescriptorHeap* resourcesHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::CBV_SRV_UAV);
	DescriptorHeap* samplerHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::Sampler);
	D3D12_CPU_DESCRIPTOR_HANDLE nextSamplerHandle = samplerHeap->GetNextCPUHandle();
	m_defaultSampler = g_theRenderer->CreateSampler(nextSamplerHandle.ptr, SamplerMode::BILINEARCLAMP);
	nextSamplerHandle = samplerHeap->GetNextCPUHandle();
	m_defaultTextSampler = g_theRenderer->CreateSampler(nextSamplerHandle.ptr, SamplerMode::POINTCLAMP);

	g_theRenderer->CreateConstantBufferView(resourcesHeap->GetNextCPUHandle().ptr, m_UICamera.GetCameraBuffer());
	g_theRenderer->CreateConstantBufferView(resourcesHeap->GetNextCPUHandle().ptr, g_theRenderer->GetDefaultModelBuffer());
	g_theRenderer->CreateShaderResourceView(resourcesHeap->GetNextCPUHandle().ptr, g_theRenderer->GetDefaultTexture());
	g_theRenderer->CreateShaderResourceView(resourcesHeap->GetNextCPUHandle().ptr, m_testTexture);
	g_theRenderer->CreateShaderResourceView(resourcesHeap->GetNextCPUHandle().ptr, &g_squirrelFont->GetTexture());

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

	TransitionBarrier resourceBarriers[3] = {};
	resourceBarriers[0] = m_renderTarget->GetTransitionBarrier(ResourceStates::RenderTarget);
	resourceBarriers[1] = m_depthTarget->GetTransitionBarrier(ResourceStates::DepthWrite);
	resourceBarriers[2] = m_testTexture->GetTransitionBarrier(ResourceStates::PixelRsc);
	CommandList* cmdList = m_renderContext->GetCommandList();


	m_renderContext->BeginCamera(m_UICamera);
	{
		DescriptorHeap* cbvSRVUAVHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::CBV_SRV_UAV);
		DescriptorHeap* samplerHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::Sampler);
		cmdList->ResourceBarrier(3, resourceBarriers);

		cmdList->SetTopology(TopologyType::TRIANGLELIST);
		cmdList->ClearRenderTarget(m_renderTarget, Rgba8::BLACK);
		cmdList->ClearDepthStencilView(m_depthTarget, 1.0f);
		cmdList->BindPipelineState(m_opaqueDefault2D);
		cmdList->SetDescriptorSet(m_renderContext->GetDescriptorSet());
		cmdList->SetDescriptorTable(PARAM_CAMERA_BUFFERS, cbvSRVUAVHeap->GetGPUHandleHeapStart(), PipelineType::Graphics);
		cmdList->SetDescriptorTable(PARAM_MODEL_BUFFERS, cbvSRVUAVHeap->GetGPUHandleAtOffset(1), PipelineType::Graphics);
		cmdList->SetDescriptorTable(PARAM_TEXTURES, cbvSRVUAVHeap->GetGPUHandleAtOffset(3), PipelineType::Graphics);
		cmdList->SetDescriptorTable(PARAM_SAMPLERS, samplerHeap->GetGPUHandleHeapStart(), PipelineType::Graphics);

		unsigned int drawConstants[16] = {0};
		cmdList->SetGraphicsRootConstants(16, drawConstants);

		/*cmdList->SetVertexBuffers(&m_triangleVertsBuffer, 1);
		cmdList->DrawInstance(3, 1, 0, 0);*/
		cmdList->SetVertexBuffers(&m_testTexBuffer, 1);
		cmdList->DrawInstance(6, 1, 0, 0);

		
	}
	m_renderContext->EndCamera(m_UICamera);

	DebugRenderWorld(m_worldCamera);
	RenderUI();
	DebugRenderScreen(m_UICamera);


	Texture* backBuffer = g_theRenderer->GetActiveBackBuffer();
	cmdList->CopyTexture(backBuffer, m_renderTarget);

	resourceBarriers[0]  = backBuffer->GetTransitionBarrier(ResourceStates::Present);
	cmdList->ResourceBarrier(1, resourceBarriers);

	cmdList->Close();

	g_theRenderer->ExecuteCmdLists(CommandListType::DIRECT, 1, &cmdList);

	g_theRenderer->Present(1);

	// I want the GPU to be done, before continuing
	m_frameFence->SignalGPU();
	m_frameFence->Wait();

	g_theRenderer->HandleStateDecay(m_resources);
	m_renderContext->EndFrame();
}


void AttractScreenMode::Shutdown()
{
	// Wait for things to finish rendering to destroy resources
	m_frameFence->SignalGPU();
	m_frameFence->Wait();
	
	// Should destroy textures?? probably not, just null them
	m_testTexture = nullptr;

	delete m_triangleVertsBuffer;
	m_triangleVertsBuffer = nullptr;

	delete m_testTexBuffer;
	m_testTexBuffer = nullptr;

	delete m_textBuffer;
	m_textBuffer = nullptr;

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

void AttractScreenMode::RenderUI()
{
	m_renderContext->BeginCamera(m_UICamera);
	{
		if (m_useTextAnimation) {

			RenderTextAnimation();
		}
	}
	m_renderContext->EndCamera(m_UICamera);

	GameMode::RenderUI();
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

	// Create buffer if first time using it
	size_t bufferSize = m_textAnimTriangles.size() * sizeof(Vertex_PCU);
	if (!m_textBuffer) {
		BufferDesc textBufDesc = {};
		textBufDesc.m_data = m_textAnimTriangles.data();
		textBufDesc.m_debugName = "TextAnim Buffer";
		textBufDesc.m_memoryUsage = MemoryUsage::Dynamic;
		textBufDesc.m_size = bufferSize;
		textBufDesc.m_stride.m_strideBytes = sizeof(Vertex_PCU);
		textBufDesc.m_type = BufferType::Vertex;

		m_textBuffer = g_theRenderer->CreateBuffer(textBufDesc);
		m_resources.push_back(m_textBuffer);
	}

	m_textBuffer->CopyToBuffer(m_textAnimTriangles.data(), bufferSize);
}

void AttractScreenMode::RenderTextAnimation()
{
	if (m_textAnimTriangles.size() > 0) {

		m_UICamera.SetColorTarget(m_renderTarget);
		m_UICamera.SetDepthTarget(m_depthTarget);

		TransitionBarrier resourceBarriers[2] = {};

		resourceBarriers[0] = m_textBuffer->GetTransitionBarrier(ResourceStates::VertexAndCBuffer);
		CommandList* cmdList = m_renderContext->GetCommandList();

		DescriptorHeap* cbvSRVUAVHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::CBV_SRV_UAV);
		DescriptorHeap* samplerHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::Sampler);
		cmdList->ResourceBarrier(1, resourceBarriers);

		cmdList->SetTopology(TopologyType::TRIANGLELIST);
		cmdList->BindPipelineState(m_alphaDefault2D);
		cmdList->SetDescriptorSet(m_renderContext->GetDescriptorSet());
		cmdList->SetDescriptorTable(PARAM_CAMERA_BUFFERS, cbvSRVUAVHeap->GetGPUHandleHeapStart(), PipelineType::Graphics);
		cmdList->SetDescriptorTable(PARAM_MODEL_BUFFERS, cbvSRVUAVHeap->GetGPUHandleAtOffset(1), PipelineType::Graphics);
		cmdList->SetDescriptorTable(PARAM_TEXTURES, cbvSRVUAVHeap->GetGPUHandleAtOffset(4), PipelineType::Graphics);
		cmdList->SetDescriptorTable(PARAM_SAMPLERS, samplerHeap->GetGPUHandleAtOffset(1), PipelineType::Graphics);

		unsigned int drawConstants[16] = { 0 };
		cmdList->SetGraphicsRootConstants(16, drawConstants);

		cmdList->SetVertexBuffers(&m_textBuffer, 1);
		cmdList->DrawInstance((unsigned int)m_textAnimTriangles.size(), 1, 0, 0);

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

