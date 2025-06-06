#pragma once

#include "Game/Gameplay/GameMode.hpp"

class Buffer;
struct Sampler;

class AttractScreenMode : public GameMode {
public:
	AttractScreenMode(Game* game, Vec2 UISize) : GameMode(game, UISize) {m_UICenter = UISize * 0.5f; }
	~AttractScreenMode();

	virtual void Startup() override;
	virtual void Update(float deltaSeconds) override;
	virtual void Render() override;
	virtual void Shutdown() override;
protected:
	virtual void UpdateInput(float deltaSeconds);
	virtual void RenderUI() override;

private:
	void UpdateTextAnimation(float deltaTime, std::string text, Vec2 textLocation);
	void RenderTextAnimation();
	
	float GetTextWidthForAnim(float fullTextWidth);
	Vec2 const GetIBasisForTextAnim();
	Vec2 const GetJBasisForTextAnim();
	void TransformText(Vec2 iBasis, Vec2 jBasis, Vec2 translation);
	void CreateResources();

private:
	std::string m_currentText = "";
	std::vector	<Vertex_PCU> m_textAnimTriangles = std::vector<Vertex_PCU>();
	Rgba8 m_textAnimationColor = Rgba8(255, 255, 255, 255);
	Rgba8 m_startTextColor;
	Rgba8 m_endTextColor;
	bool m_useTextAnimation = true;
	bool m_transitionTextColor = false;
	Vec2 m_UICenter = {};
	float m_textAnimationPosPercentageTop = g_gameConfigBlackboard.GetValue("TEXT_ANIMATION_POS_PERCENTAGE_TOP", 0.98f);
	float m_textCellHeightAttractScreen = g_gameConfigBlackboard.GetValue("TEXT_CELL_HEIGHT_ATTRACT_SCREEN", 40.0f);
	float m_textAnimationTime = g_gameConfigBlackboard.GetValue("TEXT_ANIMATION_TIME", 4.0f);
	float m_textMovementPhaseTimePercentage = g_gameConfigBlackboard.GetValue("TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE", 0.25f);
	float m_timeTextAnimation = 0.0f;
	
	Texture* m_testTexture = nullptr;
	Buffer* m_triangleVertsBuffer = nullptr;
	Buffer* m_testTexBuffer = nullptr;
	Buffer* m_textBuffer = nullptr;

};