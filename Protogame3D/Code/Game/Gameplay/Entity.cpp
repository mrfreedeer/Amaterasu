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
	if (m_modelBuffer) {
		delete m_modelBuffer;
		m_modelBuffer = nullptr;
	}

	if (m_vertexBuffer) {
		delete m_vertexBuffer;
		m_vertexBuffer = nullptr;
	}

	if (m_drawConstants) {
		delete m_drawConstants;
		m_drawConstants = nullptr;
	}
}

void Entity::Update(float deltaTime)
{
	m_position += m_velocity * deltaTime;
	m_orientation += m_angularVelocity * deltaTime;
	if (m_modelBuffer) {
		UpdateModelBuffer();
	}
}

void Entity::CreateModelBuffer(Renderer* renderer)
{
	BufferDesc bufferDesc = {};
	bufferDesc.m_debugName = "Model Buffer";
	bufferDesc.m_memoryUsage = MemoryUsage::Dynamic;
	bufferDesc.m_size = sizeof(ModelConstants);
	bufferDesc.m_stride.m_strideBytes = sizeof(ModelConstants);
	bufferDesc.m_type = BufferType::Constant;

	m_modelBuffer = renderer->CreateBuffer(bufferDesc);
}

void Entity::CreateDrawInfoBuffer(Renderer* renderer)
{
	BufferDesc bufferDesc = {};
	bufferDesc.m_debugName = "Draw CBuffer";
	bufferDesc.m_memoryUsage = MemoryUsage::Dynamic;
	bufferDesc.m_size = sizeof(DrawInfoConstants);
	bufferDesc.m_stride.m_strideBytes = sizeof(DrawInfoConstants);
	bufferDesc.m_type = BufferType::Constant;

	m_drawConstants = renderer->CreateBuffer(bufferDesc);
}

Mat44 Entity::GetModelMatrix() const
{
	Mat44 model = m_orientation.GetMatrix_XFwd_YLeft_ZUp();
	model.SetTranslation3D(m_position);
	return model;
}

void Entity::UpdateModelBuffer()
{
	ModelConstants updatedModel = {};
	updatedModel.ModelMatrix = GetModelMatrix();
	m_modelColor.GetAsFloats(updatedModel.ModelColor);
	m_modelBuffer->CopyToBuffer(&updatedModel, sizeof(ModelConstants));

}

void Entity::UpdateDrawInfoBuffer()
{
	DrawInfoConstants updatedDrawInfo = {};
	updatedDrawInfo.CameraBufferInd = m_cameraIndex;
	updatedDrawInfo.ModelBufferInd = m_modelIndex;
	updatedDrawInfo.TextureStart = m_textureIndex;

	m_drawConstants->CopyToBuffer(&updatedDrawInfo, sizeof(DrawInfoConstants));
}

void Entity::SetDrawConstants(unsigned int cameraInd, unsigned int modelInd, unsigned int textureInd)
{
	m_cameraIndex = cameraInd;
	m_modelIndex = modelInd;
	m_textureIndex = textureInd;
}

