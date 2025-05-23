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
		if(playbackId == MISSING_SOUND_ID) continue;

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

	delete m_worldCameraBuffer;
	m_worldCameraBuffer = nullptr;

	delete m_UICameraBuffer;
	m_UICameraBuffer = nullptr;
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

void GameMode::UpdateEntities(float deltaSeconds)
{
	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity* entity = m_allEntities[entityIndex];
		entity->Update(deltaSeconds);
	}
}

void GameMode::RenderEntities() const
{
	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity* entity = m_allEntities[entityIndex];
		entity->Render();
	}
}

void GameMode::RenderUI() 
{
	m_renderContext->BeginCamera(m_UICamera);

	//AABB2 devConsoleBounds(m_UICamera.GetOrthoBottomLeft(), m_UICamera.GetOrthoTopRight());
	//AABB2 screenBounds(m_UICamera.GetOrthoBottomLeft(), m_UICamera.GetOrthoTopRight());

	//Material* default2DMat = g_theMaterialSystem->GetMaterialForName("Default2DMaterial");
	//g_theRenderer->BindMaterial(default2DMat);

	//std::vector<Vertex_PCU> gameInfoVerts;

	//g_theConsole->Render(devConsoleBounds);
	m_renderContext->EndCamera(m_UICamera);

}
