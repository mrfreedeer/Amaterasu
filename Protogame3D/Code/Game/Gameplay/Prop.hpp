#pragma once
#include "Game/Gameplay/Entity.hpp"

class Texture;

enum class PropRenderType {
	CUBE = 0,
	GRID,
	SPHERE,
	NUM_PROP_RENDER_TYPES
};

class Prop : public Entity {
public:
	Prop(Game* pointerToGame, Vec3 const& startingWorldPosition, PropRenderType renderType, IntVec2 gridSize);
	Prop(Game* pointerToGame, Vec3 const& startingWorldPosition, PropRenderType renderType, float radius);
	Prop(Game* pointerToGame, Vec3 const& startingWorldPosition, PropRenderType renderType);
	~Prop();

	void Update(float deltaSeconds) override;
	void Render(CommandList* commandList) const override;
	Texture* GetUsedTexture() const override;
	Buffer* CreateVertexBuffer();
private:
	void InitializeLocalVerts();

	void InitiliazeLocalVertsCube();
	void InitiliazeLocalVertsGrid();
	void InitializeLocalVertsSphere();

	Texture* m_texture = nullptr;
	PropRenderType m_type = PropRenderType::CUBE;
	IntVec2 m_gridSize = IntVec2::ZERO;
	float m_sphereRadius = 0.0f;

};