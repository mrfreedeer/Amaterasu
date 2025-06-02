#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include <vector>

class Game;
class Buffer;
class CommandList;
class Texture;

class Entity {
public:
	Entity(Game* pointerToGame, Vec3 const& startingWorldPosition);
	virtual ~Entity();

	virtual void Update(float deltaTime);
	virtual void Render(CommandList* cmdList) const = 0;
	virtual void CreateModelBuffer(Renderer* renderer);
	virtual void CreateDrawInfoBuffer(Renderer* renderer);

	virtual Texture* GetUsedTexture() const { return nullptr; }
	virtual Mat44 GetModelMatrix() const;
	virtual Buffer* GetDrawInfoBuffer() const { return m_drawConstants; }
	virtual Buffer* GetModelBuffer() const { return m_modelBuffer; }
	virtual Buffer* GetVertexBuffer() const { return m_vertexBuffer; }
	virtual void UpdateModelBuffer();
	virtual void UpdateDrawInfoBuffer();
	virtual void SetDrawConstants(unsigned int cameraInd, unsigned int modelInd, unsigned int textureInd);
public:
	Vec3 m_position = Vec3::ZERO;
	Vec3 m_velocity = Vec3::ZERO;
	EulerAngles m_orientation = {};
	EulerAngles m_angularVelocity = {};

protected:
	Game* m_game = nullptr;
	Buffer* m_modelBuffer = nullptr;
	Buffer* m_drawConstants = nullptr;
	Buffer* m_vertexBuffer = nullptr;
	std::vector<Vertex_PCU> m_verts;

	Rgba8 m_modelColor = Rgba8::WHITE;

	// For managing bindless style resource access
	// Camera index will most likely be always 0
	unsigned int m_cameraIndex = 0;
	unsigned int m_textureIndex = 0;
	unsigned int m_modelIndex = 0;
};

typedef std::vector<Entity*> EntityList;