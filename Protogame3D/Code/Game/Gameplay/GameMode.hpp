#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Game/Gameplay/Entity.hpp"
#include "Game/Gameplay/Player.hpp"
#include "Engine/Renderer/Interfaces/PipelineState.hpp"
#include "Engine/Renderer/Interfaces/Fence.hpp"

class Game;
class Fence;
class CommandList;
class Resource;

class GameMode {
public:
	GameMode(Game* game, Vec2 const& UISize) : m_game(game), m_UISize(UISize) { m_UICamera.SetOrthoView(Vec2::ZERO, UISize); }
	virtual ~GameMode();

	virtual void Startup();
	virtual void Update(float deltaSeconds);
	virtual void Render();
	virtual void Shutdown();

protected:
	virtual void UpdateDeveloperCheatCodes(float deltaSeconds);
	virtual void UpdateInput(float deltaSeconds) = 0;
	Rgba8 const GetRandomColor() const;
	void CheckIfWindowHasFocus();

	virtual void UpdateEntities(float deltaSeconds);
	virtual void RenderEntities() const;
	virtual void RenderUI();
protected:
	Game* m_game = nullptr;
	RenderContext* m_renderContext = nullptr;
	Fence* m_copyFence = nullptr;
	Fence* m_frameFence = nullptr;
	Fence* m_gpuFence = nullptr;
	CommandList** m_copyCmdLists = nullptr;
	Texture* m_renderTarget = nullptr;
	Texture* m_depthTarget = nullptr;
	Buffer* m_UICameraBuffer = nullptr;
	Buffer* m_worldCameraBuffer = nullptr;
	std::vector<Resource*> m_resources;
	Camera m_worldCamera;
	Camera m_UICamera;
	Vec2 m_UISize = {};
	Vec2 m_worldSize = {};

	float m_timeAlive = 0.0f;
	Clock m_clock;

	EntityList m_allEntities;

	Player* m_player = nullptr;

	bool m_isCursorHidden = false;
	bool m_isCursorClipped = false;
	bool m_isCursorRelative = false;

	bool m_lostFocusBefore = false;

	float m_textCellHeight = g_gameConfigBlackboard.GetValue("TEXT_CELL_HEIGHT", 20.0f);


};