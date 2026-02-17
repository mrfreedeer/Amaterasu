#pragma  once

#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Stopwatch.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"

#include <vector>
enum class ScreenTextType {
	ScreenMessage,
	FreeText,
	NUM_SCREEN_TEXT_TYPES
};

class Renderer;
class Camera;
class Texture;
struct Mat44;

enum DebugShapeFlagsBit : unsigned int {
	DebugWorldShape = (1 << 0),
	DebugWorldShapeText = (1 << 1),
	DebugWorldShapeValid = (1 << 2)
};

enum DebugShapeType : unsigned int {
	INVALID = -1,
	DEBUG_RENDER_WORLD_POINT,
	DEBUG_RENDER_WORLD_LINE,
	DEBUG_RENDER_WORLD_ARROW,
	DEBUG_RENDER_WORLD_BOX,
	DEBUG_RENDER_WORLD_WIRE_CYLINDER,
	DEBUG_RENDER_WORLD_WIRE_SPHERE,
	DEBUG_RENDER_WORLD_WIRE_BOX,
	DEBUG_RENDER_WORLD_BASIS,
	DEBUG_RENDER_WORLD_TEXT,
	DEBUG_RENDER_WORLD_BILLBOARD_TEXT,
	DEBUG_RENDER_SCREEN_TEXT,
	DEBUG_RENDER_MESSAGE,
	DEBUG_RENDER_NUM_TYPES
};

struct DebugShapeInfo {
	unsigned int m_startVertex = 0;
	unsigned int m_vertexCount = 0;
	unsigned int m_flags = 0;
	unsigned char m_stacks = 16;		// 16 is the default of the systen for stacks and slices
	unsigned char m_slices = 16;
	float m_duration = 0.0f;
	float m_radius = 0.0f;
	Vec3 m_start = Vec3::ZERO;
	Vec3 m_end = Vec3::ZERO;
	Stopwatch m_stopwach;
	DebugShapeType m_shapeType = INVALID;
	Mat44 m_modelMatrix = Mat44();
	DebugRenderMode m_renderMode = DebugRenderMode::UNDEFINED;
	ScreenTextType m_textType = ScreenTextType::FreeText;
	Rgba8 m_startColor = Rgba8::WHITE;
	Rgba8 m_endColor = Rgba8::WHITE;
	std::string m_text = "";
};

struct DebugShape {

public:
	DebugShape(DebugShapeInfo const& info) : m_info(info) {}

	bool CanShapeBeDeleted();
	bool IsShapeValid() const { return m_info.m_flags & DebugShapeFlagsBit::DebugWorldShapeValid; }
	void MarkForDeletion() { m_info.m_flags &= ~DebugShapeFlagsBit::DebugWorldShapeValid; }
	void Render(Renderer* renderer) const;
	Mat44 const& GetModelMatrix() const { return m_info.m_modelMatrix; }
	Mat44 const GetBillboardModelMatrix(Camera const& camera) const;
	Rgba8 const GetModelColor() const;
	unsigned int GetVertexCount() const { return m_info.m_vertexCount; }
	DebugRenderMode GetRenderMode() const { return m_info.m_renderMode; }
	DebugShapeType GetType() const { return m_info.m_shapeType; }
	float GetRadius() const { return m_info.m_radius; }
	unsigned char GetStacks() const { return m_info.m_stacks; }
	unsigned char GetSlices() const { return m_info.m_slices; }
	Vec3 GetStart() const { return m_info.m_start; }
	Vec3 GetEnd() const { return m_info.m_end; }
	unsigned int GetVertexStart() const { return m_info.m_startVertex; }
	bool IsBillboarded() const { return m_info.m_shapeType == DEBUG_RENDER_WORLD_BILLBOARD_TEXT; }
public:
	DebugShapeInfo m_info = {};
	unsigned int m_modelMatrixOffset = 0;
	unsigned int m_debugVertOffset = 0;
};