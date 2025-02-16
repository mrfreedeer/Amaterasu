#include "Game/Gameplay/Entity.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Engine/Renderer/Interfaces/Buffer.hpp"
#include "Engine/Renderer/ConstantBuffers.hpp"
#include "Engine/Renderer/Renderer.hpp"

Entity::Entity(Game* pointerToGame, Vec3 const& startingWorldPosition):
	m_game(pointerToGame),
	m_position(startingWorldPosition)
{
}

Entity::~Entity()
{
	m_game = nullptr;
}

void Entity::Update(float deltaTime)
{
	m_position += m_velocity * deltaTime;
	m_orientation += m_angularVelocity * deltaTime;
}

void Entity::CreateModelBuffer(Renderer* renderer)
{
	BufferDesc bufferDesc = {};
	bufferDesc.m_debugName = "Model Buffer";
	bufferDesc.m_memoryUsage = MemoryUsage::Dynamic;
	bufferDesc.m_size = sizeof(ModelConstants);
	bufferDesc.m_stride.m_strideBytes = sizeof(ModelConstants);

	m_modelBuffer = renderer->CreateBuffer(bufferDesc);
}

Mat44 Entity::GetModelMatrix() const
{
	Mat44 model = m_orientation.GetMatrix_XFwd_YLeft_ZUp();
	model.SetTranslation3D(m_position);
	return model;
}

