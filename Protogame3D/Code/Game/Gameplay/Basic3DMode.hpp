#pragma once

#include "Game/Gameplay/GameMode.hpp"


enum class MaterialEffect {
	NoEffect = -1,
	ColorBanding,
	Pixelized,
	Grayscale,
	Inverted,
	DistanceFog,
	NUM_EFFECTS
};

class Basic3DMode : public GameMode {
public:
	Basic3DMode(Game* game, Vec2 const& UISize);
	virtual void Startup() override;
	virtual void Update(float deltaSeconds);
	virtual void Render() override;
	virtual void Shutdown() override;

	static bool DebugSpawnWorldWireSphere(EventArgs& eventArgs);
	static bool DebugSpawnWorldLine3D(EventArgs& eventArgs);
	static bool DebugClearShapes(EventArgs& eventArgs);
	static bool DebugToggleRenderMode(EventArgs& eventArgs);
	static bool DebugSpawnPermanentBasis(EventArgs& eventArgs);
	static bool DebugSpawnWorldWireCylinder(EventArgs& eventArgs);
	static bool DebugSpawnBillboardText(EventArgs& eventArgs);
	static bool GetControls(EventArgs& eventArgs);

protected:
	virtual void UpdateDeveloperCheatCodes(float deltaSeconds);
	virtual void UpdateInput(float deltaSeconds) override;
	virtual void CreateResourceDescriptors();
	void CreateGPUBuffers();
	void StartUpRayTracing();

private:
	void DisplayClocksInfo() const;

private:
	float m_fps = 0.0f;
	Texture* m_rtRenderTarget = nullptr;
	Buffer* m_rtGeomBuffer = nullptr;
	Buffer* m_scratchBuffer = nullptr;
	Buffer* m_BLASbuffer = nullptr;
	Buffer* m_TLASbuffer = nullptr;
	Buffer* m_instanceBuffer = nullptr;
	CommandList** m_rtCommandLists = nullptr;
};