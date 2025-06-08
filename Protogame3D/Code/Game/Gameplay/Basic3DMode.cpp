
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Interfaces/Buffer.hpp"
#include "Game/Gameplay/Basic3DMode.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Prop.hpp"
#include "Game/Gameplay/Game.hpp"

Basic3DMode* pointerToSelf = nullptr;

Basic3DMode::Basic3DMode(Game* game, Vec2 const& UISize) :
	GameMode(game, UISize)
{
	m_worldCamera.SetPerspectiveView(g_theWindow->GetConfig().m_clientAspect, 60, 0.1f, 100.0f);
}

void Basic3DMode::Startup()
{
	GameMode::Startup();

	unsigned int descriptorCounts[] = { 128, 2, 1, 8 };
	CreateRendererObjects("Basic3DMode", descriptorCounts);

	SubscribeEventCallbackFunction("DebugAddWorldWireSphere", DebugSpawnWorldWireSphere);
	SubscribeEventCallbackFunction("DebugAddWorldLine", DebugSpawnWorldLine3D);
	SubscribeEventCallbackFunction("DebugRenderClear", DebugClearShapes);
	SubscribeEventCallbackFunction("DebugRenderToggle", DebugToggleRenderMode);
	SubscribeEventCallbackFunction("DebugAddBasis", DebugSpawnPermanentBasis);
	SubscribeEventCallbackFunction("DebugAddWorldWireCylinder", DebugSpawnWorldWireCylinder);
	SubscribeEventCallbackFunction("DebugAddBillboardText", DebugSpawnBillboardText);
	SubscribeEventCallbackFunction("Controls", GetControls);

	pointerToSelf = this;


	Player* player = new Player(m_game, Vec3(27.0f, -12.0f, 14.0f), &m_worldCamera);
	m_player = player;
	m_player->m_orientation = EulerAngles(521.0f, 36.0f, 0.0f);

	Prop* cubeProp = new Prop(m_game, Vec3(-2.0f, 2.0f, 0.0f), PropRenderType::CUBE);
	cubeProp->m_angularVelocity.m_yawDegrees = 45.0f;

	Prop* gridProp = new Prop(m_game, Vec3::ZERO, PropRenderType::GRID, IntVec2(100,100));

	Prop* sphereProp = new Prop(m_game, Vec3(10.0f, -5.0f, 1.0f), PropRenderType::SPHERE, 1.0f);
	sphereProp->m_angularVelocity.m_pitchDegrees = 20.0f;
	sphereProp->m_angularVelocity.m_yawDegrees = 20.0f;

	m_allEntities.push_back(player);
	m_allEntities.push_back(cubeProp);
	m_allEntities.push_back(gridProp);
	m_allEntities.push_back(sphereProp);

	CreateResourceDescriptors();
	CreateGPUBuffers();
	

	m_isCursorHidden = true;
	m_isCursorClipped = true;
	m_isCursorRelative = true;

	g_theInput->ResetMouseClientDelta();

	//#TODO fix this
	DebugAddWorldBasis(Mat44(), -1.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);

	float axisLabelTextHeight = 0.25f;
	Mat44 xLabelTransformMatrix(Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3::ZERO);
	float xLabelWidth = g_squirrelFont->GetTextWidth(axisLabelTextHeight, "X - Forward");
	xLabelTransformMatrix.SetTranslation3D(Vec3(xLabelWidth * 0.7f, 0.0f, axisLabelTextHeight));

	DebugAddWorldText("X - Forward", xLabelTransformMatrix, axisLabelTextHeight, Vec2(0.5f, 0.5f), -1.0f, Rgba8::RED, Rgba8::RED, DebugRenderMode::USEDEPTH);

	Mat44 yLabelTransformMatrix(Vec3(0.0f, 1.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3::ZERO);
	float yLabelWidth = g_squirrelFont->GetTextWidth(axisLabelTextHeight, "Y - Left");
	yLabelTransformMatrix.SetTranslation3D(Vec3(-axisLabelTextHeight, yLabelWidth * 0.7f, 0.0f));

	DebugAddWorldText("Y - Left", yLabelTransformMatrix, axisLabelTextHeight, Vec2(0.5f, 0.5f), -1.0f, Rgba8::GREEN, Rgba8::GREEN, DebugRenderMode::USEDEPTH);

	Mat44 zLabelTransformMatrix(Vec3(0.0f, 0.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), Vec3::ZERO);
	float zLabelWidth = g_squirrelFont->GetTextWidth(axisLabelTextHeight, "Z - Up");
	zLabelTransformMatrix.SetTranslation3D(Vec3(0.0f, axisLabelTextHeight, zLabelWidth * 0.7f));

	DebugAddWorldText("Z - Up", zLabelTransformMatrix, axisLabelTextHeight, Vec2(0.5f, 0.5f), -1.0f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USEDEPTH);


	m_renderContext->CloseAll();

	//TextureCreateInfo colorInfo;
	//colorInfo.m_dimensions = g_theWindow->GetClientDimensions();
	//colorInfo.m_format = TextureFormat::R8G8B8A8_UNORM;
	//colorInfo.m_bindFlags = TEXTURE_BIND_RENDER_TARGET_BIT | TEXTURE_BIND_SHADER_RESOURCE_BIT;
	//colorInfo.m_memoryUsage = MemoryUsage::Default;

	//m_effectsMaterials[(int)MaterialEffect::ColorBanding] = g_theMaterialSystem->GetMaterialForName("ColorBandFX");
	//m_effectsMaterials[(int)MaterialEffect::Grayscale] = g_theMaterialSystem->GetMaterialForName("GrayScaleFX");
	//m_effectsMaterials[(int)MaterialEffect::Inverted] = g_theMaterialSystem->GetMaterialForName("InvertedColorFX");
	//m_effectsMaterials[(int)MaterialEffect::Pixelized] = g_theMaterialSystem->GetMaterialForName("PixelizedFX");
	//m_effectsMaterials[(int)MaterialEffect::DistanceFog] = g_theMaterialSystem->GetMaterialForName("DistanceFogFX");

}

void Basic3DMode::Update(float deltaSeconds)
{
	GameMode::Update(deltaSeconds);
	m_fps = 1.0f / deltaSeconds;
	UpdateInput(deltaSeconds);
	Vec2 mouseClientDelta = g_theInput->GetMouseClientDelta();


	std::string playerPosInfo = Stringf("Player position: (%.2f, %.2f, %.2f)", m_player->m_position.x, m_player->m_position.y, m_player->m_position.z);
	DebugAddMessage(playerPosInfo, 0, Rgba8::WHITE, Rgba8::WHITE);
	std::string gameInfoStr = Stringf("Delta: (%.2f, %.2f)", mouseClientDelta.x, mouseClientDelta.y);
	DebugAddMessage(gameInfoStr, 0.0f, Rgba8::WHITE, Rgba8::WHITE);

	UpdateDeveloperCheatCodes(deltaSeconds);
	UpdateEntities(deltaSeconds);

	DisplayClocksInfo();


	UpdateCamera(m_worldCamera, m_worldCamera.GetCameraBuffer());
}

void Basic3DMode::Render()
{
	m_renderContext->Reset();

	CommandList* cmdList = m_renderContext->GetCommandList();
	TransitionBarrier rscBarriers[2] = {};
	rscBarriers[0] = m_renderTarget->GetTransitionBarrier(ResourceStates::RenderTarget);
	rscBarriers[1] = m_depthTarget->GetTransitionBarrier(ResourceStates::DepthWrite);
	m_worldCamera.SetColorTarget(m_renderTarget);
	m_worldCamera.SetDepthTarget(m_depthTarget);
	DescriptorHeap* cbvSRVUAVHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::CBV_SRV_UAV);
	DescriptorHeap* samplerHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::Sampler);
	DescriptorHeap* rtHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::RenderTargetView);
	{
		m_renderContext->BeginCamera(m_worldCamera);
		cmdList->ResourceBarrier(_countof(rscBarriers), rscBarriers);
		cmdList->BindPipelineState(m_opaqueDefault2D);
		cmdList->ClearRenderTarget(m_renderTarget, Rgba8::BLACK);
		cmdList->ClearDepthStencilView(m_depthTarget, 1.0f);
		cmdList->SetRenderTargets(1, &m_renderTarget, false, m_depthTarget);
		cmdList->SetDescriptorSet(m_renderContext->GetDescriptorSet());
		cmdList->SetDescriptorTable(PARAM_CAMERA_BUFFERS, cbvSRVUAVHeap->GetGPUHandleAtOffset(m_cameraCBVStart), PipelineType::Graphics);
		cmdList->SetDescriptorTable(PARAM_MODEL_BUFFERS, cbvSRVUAVHeap->GetGPUHandleAtOffset(m_modelCBVStart), PipelineType::Graphics);
		cmdList->SetDescriptorTable(PARAM_TEXTURES, cbvSRVUAVHeap->GetGPUHandleHeapStart(), PipelineType::Graphics);
		cmdList->SetDescriptorTable(PARAM_SAMPLERS, samplerHeap->GetGPUHandleHeapStart(), PipelineType::Graphics);
		cmdList->SetTopology(TopologyType::TRIANGLELIST);
		unsigned int drawConstants[16] = { 0 };
		cmdList->SetGraphicsRootConstants(16, drawConstants);

		RenderEntities();
		m_renderContext->EndCamera(m_worldCamera);
	}



	for (int effectInd = 0; effectInd < (int)MaterialEffect::NUM_EFFECTS; effectInd++) {
		if (m_applyEffects[effectInd]) {
			//g_theRenderer->ApplyEffect(m_effectsMaterials[effectInd], &m_worldCamera);
		}
	}

	GameMode::Render();

	m_renderContext->EndFrame();
	Texture* backBuffer = g_theRenderer->GetActiveBackBuffer();
	cmdList->CopyTexture(backBuffer, m_renderTarget);
	
	TransitionBarrier presentBarrier = backBuffer->GetTransitionBarrier(ResourceStates::Present);
	cmdList->ResourceBarrier(1, &presentBarrier);

	cmdList->Close();



	g_theRenderer->ExecuteCmdLists(CommandListType::DIRECT, 1, &cmdList);
	g_theRenderer->Present(1);

	// I want the GPU to be done, before continuing
	m_frameFence->SignalGPU();
	m_frameFence->Wait();


}

void Basic3DMode::Shutdown()
{
	pointerToSelf = nullptr;
	m_frameFence->SignalGPU();
	m_frameFence->Wait();
	m_copyFence->SignalGPU();
	m_copyFence->Wait();

	GameMode::Shutdown();
	UnsubscribeEventCallbackFunction("DebugAddWorldWireSphere", DebugSpawnWorldWireSphere);
	UnsubscribeEventCallbackFunction("DebugAddWorldLine", DebugSpawnWorldLine3D);
	UnsubscribeEventCallbackFunction("DebugRenderClear", DebugClearShapes);
	UnsubscribeEventCallbackFunction("DebugRenderToggle", DebugToggleRenderMode);
	UnsubscribeEventCallbackFunction("DebugAddBasis", DebugSpawnPermanentBasis);
	UnsubscribeEventCallbackFunction("DebugAddWorldWireCylinder", DebugSpawnWorldWireCylinder);
	UnsubscribeEventCallbackFunction("DebugAddBillboardText", DebugSpawnBillboardText);
	UnsubscribeEventCallbackFunction("Controls", GetControls);

}

void Basic3DMode::UpdateDeveloperCheatCodes(float deltaSeconds)
{
	GameMode::UpdateDeveloperCheatCodes(deltaSeconds);


	if (g_theInput->WasKeyJustPressed('1')) {
		FireEvent("DebugAddWorldWireSphere");
	}

	if (g_theInput->WasKeyJustPressed('2')) {
		FireEvent("DebugAddWorldLine");
	}

	if (g_theInput->WasKeyJustPressed('3')) {
		FireEvent("DebugAddBasis");
	}

	if (g_theInput->WasKeyJustPressed('4')) {
		FireEvent("DebugAddBillboardText");
	}

	if (g_theInput->WasKeyJustPressed('5')) {
		FireEvent("DebugAddWorldWireCylinder");
	}

	if (g_theInput->WasKeyJustPressed('6')) {
		EulerAngles cameraOrientation = m_worldCamera.GetViewOrientation();
		std::string cameraOrientationInfo = Stringf("Camera Orientation: (%.2f, %.2f, %.2f)", cameraOrientation.m_yawDegrees, cameraOrientation.m_pitchDegrees, cameraOrientation.m_rollDegrees);
		DebugAddMessage(cameraOrientationInfo, 5.0f, Rgba8::WHITE, Rgba8::RED);
	}

	if (g_theInput->WasKeyJustPressed('9')) {
		Clock const& debugClock = DebugRenderGetClock();
		double newTimeDilation = debugClock.GetTimeDilation();
		newTimeDilation -= 0.1;
		if (newTimeDilation < 0.1) newTimeDilation = 0.1;
		DebugRenderSetTimeDilation(newTimeDilation);
	}

	if (g_theInput->WasKeyJustPressed('0')) {
		Clock const& debugClock = DebugRenderGetClock();
		double newTimeDilation = debugClock.GetTimeDilation();
		newTimeDilation += 0.1;
		if (newTimeDilation > 10.0) newTimeDilation = 10.0;
		DebugRenderSetTimeDilation(newTimeDilation);
	}
}

void Basic3DMode::UpdateInput(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_game->ExitToAttractScreen();
	}


	for (int effectIndex = 0; effectIndex < (int)MaterialEffect::NUM_EFFECTS; effectIndex++) {
		int keyCode = 97 + effectIndex; // 97 is 1 numpad
		if (g_theInput->WasKeyJustPressed((unsigned short)keyCode)) {
			m_applyEffects[effectIndex] = !m_applyEffects[effectIndex];
		}
	}
	if (g_theInput->WasKeyJustPressed('F')) {
		m_applyEffects[0] = !m_applyEffects[0];
	}
	if (g_theInput->WasKeyJustPressed('G')) {
		m_applyEffects[1] = !m_applyEffects[1];
	}
	if (g_theInput->WasKeyJustPressed('J')) {
		m_applyEffects[2] = !m_applyEffects[2];
	}
	if (g_theInput->WasKeyJustPressed('K')) {
		m_applyEffects[3] = !m_applyEffects[3];
	}
	if (g_theInput->WasKeyJustPressed('L')) {
		m_applyEffects[4] = !m_applyEffects[4];
	}
}

void Basic3DMode::CreateResourceDescriptors()
{
	DescriptorHeap* resourcesHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::CBV_SRV_UAV);
	DescriptorHeap* samplerHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::Sampler);

	unsigned int texturesPerEntity = 1;
	m_cbvStart= (texturesPerEntity * (unsigned int)m_allEntities.size()) + 1; // First will be default texture
	m_drawInfoCBVStart = m_cbvStart;
	m_cameraCBVStart = m_cbvStart + (unsigned int)m_allEntities.size();
	m_modelCBVStart = m_cameraCBVStart + 2; // 2 Cameras
	D3D12_CPU_DESCRIPTOR_HANDLE defaultTexHandle = resourcesHeap->GetNextCPUHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE defaultModelBuffer = resourcesHeap->GetCPUHandleAtOffset(m_modelCBVStart);
	g_theRenderer->CreateShaderResourceView(defaultTexHandle.ptr, g_theRenderer->GetDefaultTexture());
	g_theRenderer->CreateShaderResourceView(defaultModelBuffer.ptr, g_theRenderer->GetDefaultModelBuffer());

	for (unsigned int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity* entity = m_allEntities[entityIndex];
		if(entity == m_player) continue;
		D3D12_CPU_DESCRIPTOR_HANDLE nextSRV = resourcesHeap->GetNextCPUHandle();
		Texture* tex = entity->GetUsedTexture();
		unsigned int currentSRV = 0;
		if (tex) {
			currentSRV = entityIndex;
			g_theRenderer->CreateShaderResourceView(nextSRV.ptr, tex);
		}
		// Model buffer is at slot 0
		unsigned int currentModelCBV = m_modelCBVStart + entityIndex + 1;
		unsigned int currentDrawCBV = m_drawInfoCBVStart + entityIndex;
		D3D12_CPU_DESCRIPTOR_HANDLE nextModelCBV = resourcesHeap->GetCPUHandleAtOffset(currentModelCBV);
		D3D12_CPU_DESCRIPTOR_HANDLE nextDrawCBV = resourcesHeap->GetCPUHandleAtOffset(currentDrawCBV);
		Buffer* modelBuffer = entity->GetModelBuffer();
		Buffer* drawInfoBuffer = entity->GetDrawInfoBuffer();
		g_theRenderer->CreateConstantBufferView(nextModelCBV.ptr, modelBuffer);
		g_theRenderer->CreateConstantBufferView(nextDrawCBV.ptr, drawInfoBuffer);

		entity->SetDrawConstants(0, entityIndex + 1, currentSRV);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE nextSamplerHandle = samplerHeap->GetNextCPUHandle();
	m_defaultSampler = g_theRenderer->CreateSampler(nextSamplerHandle.ptr, SamplerMode::BILINEARCLAMP);
	nextSamplerHandle = samplerHeap->GetNextCPUHandle();
	m_defaultTextSampler = g_theRenderer->CreateSampler(nextSamplerHandle.ptr, SamplerMode::POINTCLAMP);


	g_theRenderer->CreateConstantBufferView(resourcesHeap->GetCPUHandleAtOffset(m_cameraCBVStart).ptr, m_worldCamera.GetCameraBuffer());
	g_theRenderer->CreateConstantBufferView(resourcesHeap->GetCPUHandleAtOffset(m_cameraCBVStart + 1).ptr, m_UICamera.GetCameraBuffer());

}

void Basic3DMode::CreateGPUBuffers()
{
	CommandList* copyCmdList = m_copyCmdLists[m_renderContext->GetBufferIndex()];
	std::vector<Buffer*> intermediateBuffers;
	intermediateBuffers.reserve(m_allEntities.size());
	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity* entity = m_allEntities[entityIndex];
		if(entity == m_player) continue;

		Prop* prop = reinterpret_cast<Prop*>(entity);
		Buffer* intermeadiateBuffer = prop->CreateVertexBuffer();

		copyCmdList->CopyResource(entity->GetVertexBuffer(), intermeadiateBuffer);
		intermediateBuffers.push_back(intermeadiateBuffer);
	}

	copyCmdList->Close();
	g_theRenderer->ExecuteCmdLists(CommandListType::COPY, 1, &copyCmdList);

	m_copyFence->SignalGPU();
	m_copyFence->Wait();

	// Waiting for the copying to be done
	g_theRenderer->InsertWaitInQueue(CommandListType::DIRECT, m_copyFence);

	for (int bufIndex = 0; bufIndex < intermediateBuffers.size(); bufIndex++) {
		delete intermediateBuffers[bufIndex];
	}

}

bool Basic3DMode::DebugSpawnWorldWireSphere(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	DebugAddWorldWireSphere(pointerToSelf->m_player->m_position, 1, 5, Rgba8::GREEN, Rgba8::RED, DebugRenderMode::USEDEPTH);
	return false;
}

bool Basic3DMode::DebugSpawnWorldLine3D(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	DebugAddWorldLine(pointerToSelf->m_player->m_position, Vec3::ZERO, 0.125f, 5.0f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::XRAY);
	return false;
}

bool Basic3DMode::DebugClearShapes(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	DebugRenderClear();
	return false;
}

bool Basic3DMode::DebugToggleRenderMode(EventArgs& eventArgs)
{
	static bool isDebuggerRenderSystemVisible = true;
	UNUSED(eventArgs);

	isDebuggerRenderSystemVisible = !isDebuggerRenderSystemVisible;
	if (isDebuggerRenderSystemVisible) {
		DebugRenderSetVisible();
	}
	else {
		DebugRenderSetHidden();
	}

	return false;
}

bool Basic3DMode::DebugSpawnPermanentBasis(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	Mat44 invertedView = pointerToSelf->m_worldCamera.GetViewMatrix();
	DebugAddWorldBasis(invertedView.GetOrthonormalInverse(), -1.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);
	return false;
}

bool Basic3DMode::DebugSpawnWorldWireCylinder(EventArgs& eventArgs)
{
	UNUSED(eventArgs);

	float cylinderHeight = 2.0f;
	Vec3 cylbase = pointerToSelf->m_player->m_position;
	cylbase.z -= cylinderHeight * 0.5f;

	Vec3 cylTop = cylbase;
	cylTop.z += cylinderHeight;
	DebugAddWorldWireCylinder(cylbase, cylTop, 0.5f, 10.0f, Rgba8::WHITE, Rgba8::RED, DebugRenderMode::USEDEPTH);
	return false;
}

bool Basic3DMode::DebugSpawnBillboardText(EventArgs& eventArgs)
{
	Player* const& player = pointerToSelf->m_player;
	std::string playerInfo = Stringf("Position: (%f, %f, %f)\nOrientation: (%f, %f, %f)", player->m_position.x, player->m_position.y, player->m_position.z, player->m_orientation.m_yawDegrees, player->m_orientation.m_pitchDegrees, player->m_orientation.m_rollDegrees);
	DebugAddWorldBillboardText(playerInfo, player->m_position, 0.25f, Vec2(0.5f, 0.5f), 10.0f, Rgba8::WHITE, Rgba8::RED, DebugRenderMode::USEDEPTH);
	UNUSED(eventArgs);
	return false;
}

bool Basic3DMode::GetControls(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	std::string controlsStr = "";
	controlsStr += "~       - Toggle dev console\n";
	controlsStr += "ESC     - Exit to attract screen\n";
	controlsStr += "F8      - Reset game\n";
	controlsStr += "W/S/A/D - Move forward/backward/left/right\n";
	controlsStr += "Z/C     - Move upward/downward\n";
	controlsStr += "Q/E     - Roll to left/right\n";
	controlsStr += "Mouse   - Aim camera\n";
	controlsStr += "Shift   - Sprint\n";
	controlsStr += "T       - Slow mode\n";
	controlsStr += "Y       - Fast mode\n";
	controlsStr += "O       - Step frame\n";
	controlsStr += "P       - Toggle pause\n";
	controlsStr += "1       - Add debug wire sphere\n";
	controlsStr += "2       - Add debug world line\n";
	controlsStr += "3       - Add debug world basis\n";
	controlsStr += "4       - Add debug world billboard text\n";
	controlsStr += "5       - Add debug wire cylinder\n";
	controlsStr += "6       - Add debug camera message\n";
	controlsStr += "9       - Decrease debug clock speed\n";
	controlsStr += "0       - Increase debug clock speed";

	Strings controlStringsSplit = SplitStringOnDelimiter(controlsStr, '\n');

	for (int stringIndex = 0; stringIndex < controlStringsSplit.size(); stringIndex++) {
		g_theConsole->AddLine(DevConsole::INFO_MINOR_COLOR, controlStringsSplit[stringIndex]);
	}

	return false;
}

void Basic3DMode::DisplayClocksInfo() const
{
	Clock& devClock = g_theConsole->m_clock;
	Clock const& debugClock = DebugRenderGetClock();

	double devClockFPS = 1.0 / devClock.GetDeltaTime();
	double gameFPS = m_fps;
	double debugClockFPS = 1.0 / debugClock.GetDeltaTime();

	double devClockTotalTime = devClock.GetTotalTime();
	double gameTotalTime = m_clock.GetTotalTime();
	double debugTotalTime = debugClock.GetTotalTime();

	double devClockScale = devClock.GetTimeDilation();
	double gameScale = m_clock.GetTimeDilation();
	double debugScale = debugClock.GetTimeDilation();


	std::string devClockInfo = Stringf("Dev Console:\t | Time: %.2f  FPS: %.2f  Scale: %.2f", devClockTotalTime, devClockFPS, devClockScale);
	std::string debugClockInfo = Stringf("Debug Render:\t | Time: %.2f  FPS: %.2f  Scale: %.2f", debugTotalTime, debugClockFPS, debugScale);
	std::string gameClockInfo = Stringf("Game:\t | Time: %.2f  FPS: %.2f  Scale: %.2f", gameTotalTime, gameFPS, gameScale);

	Vec2 devClockInfoPos(m_UISize.x - g_squirrelFont->GetTextWidth(m_textCellHeight, devClockInfo), m_UISize.y - m_textCellHeight);
	Vec2 gameClockInfoPos(m_UISize.x - g_squirrelFont->GetTextWidth(m_textCellHeight, gameClockInfo), m_UISize.y - (m_textCellHeight * 2.0f));
	Vec2 debugClockInfoPos(m_UISize.x - g_squirrelFont->GetTextWidth(m_textCellHeight, debugClockInfo), m_UISize.y - (m_textCellHeight * 3.0f));

	DebugAddScreenText(devClockInfo, devClockInfoPos, 0.0f, Vec2(1.0f, 0.0f), m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddScreenText(debugClockInfo, debugClockInfoPos, 0.0f, Vec2(1.0f, 0.0f), m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddScreenText(gameClockInfo, gameClockInfoPos, 0.0f, Vec2(1.0f, 0.0f), m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
}