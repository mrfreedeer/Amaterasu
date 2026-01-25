#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/DebugShape.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Renderer/Billboard.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include <vector>




class DebugRenderSystem {

public:
	DebugRenderSystem(DebugRenderConfig config);
	~DebugRenderSystem();

	// Getters
	Clock const& GetClock() const { return m_clock; }
	unsigned int GetVertexCount(DebugRenderMode renderMode) const { return m_vertexCounts[(int)renderMode]; }

	// Setters
	void SetVisibility(bool isHidden) { m_hidden = isHidden; }
	void SetParentClock(Clock& parentClock) { m_clock.SetParent(parentClock); }
	void SetTimeDilation(double timeDilation) { m_clock.SetTimeDilation(timeDilation); }

	// Actions
	void Startup();
	void Clear();
	void ClearVertices() { m_debugVerts.clear();}
	void RenderWorld(Camera const& worldCamera);
	void RenderScreen(Camera const& screenCamera);
	void ClearExpiredShapes();
	void AddShape(DebugShape const& newShape);

private:
	void AddVertsForShapes();

private:
	bool m_hidden = false;
	DebugRenderConfig m_config = {};
	Clock m_clock = {};
	CommandList** m_cmdLists = nullptr;
	PipelineState* m_debugPSOs[(int)DebugRenderMode::NUM_DEBUG_RENDER_MODES] = {};
	std::vector<DebugShape> m_debugShapes[(int)DebugRenderMode::NUM_DEBUG_RENDER_MODES] = {};
	unsigned int m_vertexCounts[(int)DebugRenderMode::NUM_DEBUG_RENDER_MODES] = {};
	std::vector<Vertex_PCU> m_debugVerts = {};
};

DebugRenderSystem* s_debugRenderSystem = nullptr;

void DebugRenderSystemStartup(const DebugRenderConfig& config)
{
	s_debugRenderSystem = new DebugRenderSystem(config);
}

void DebugRenderSystemShutdown()
{
	delete s_debugRenderSystem;
	s_debugRenderSystem = nullptr;
}

void DebugRenderSetVisible()
{
	s_debugRenderSystem->SetVisibility(false);
}

void DebugRenderSetHidden()
{
	s_debugRenderSystem->SetVisibility(true);
}

void DebugRenderClear()
{
	s_debugRenderSystem->Clear();
}

void DebugRenderSetParentClock(Clock& parent)
{
	s_debugRenderSystem->SetParentClock(parent);
}

void DebugRenderSetTimeDilation(double timeDilation)
{
	s_debugRenderSystem->SetTimeDilation(timeDilation);
}

Clock const& DebugRenderGetClock()
{
	return s_debugRenderSystem->GetClock();
}

void DebugRenderBeginFrame()
{
	s_debugRenderSystem->ClearVertices();
}

void DebugRenderWorld(const Camera& camera)
{
	s_debugRenderSystem->RenderWorld(camera);
}

void DebugRenderScreen(const Camera& camera)
{
	s_debugRenderSystem->RenderScreen(camera);
}

void DebugRenderEndFrame()
{
	// Clear expired shapes for next frame
	s_debugRenderSystem->ClearExpiredShapes();
}

void DebugAddWorldPoint(const Vec3& pos, float radius, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode, int stacks, int slices)
{
	Mat44 modelMatrix;
	modelMatrix.AppendTranslation3D(pos);
	unsigned int vertexCount = CalcVertCountForSphere(stacks, slices);
	unsigned int vertexStart = s_debugRenderSystem->GetVertexCount(mode);

	DebugShapeInfo shapeInfo = {};
	shapeInfo.m_startVertex = vertexStart;																			
	shapeInfo.m_vertexCount = vertexCount;																			
	shapeInfo.m_flags		= DebugShapeFlagsBit::DebugWorldShape | DebugShapeFlagsBit::DebugWorldShapeValid;		
	shapeInfo.m_stacks		= stacks;																				
	shapeInfo.m_slices		= slices;																				
	shapeInfo.m_renderMode	= mode;																
	shapeInfo.m_duration	= duration;																				
	shapeInfo.m_radius		= radius;																				
	shapeInfo.m_startColor	= startColor;																			
	shapeInfo.m_endColor	= endColor;																				
	shapeInfo.m_shapeType	= DEBUG_RENDER_WORLD_POINT;														

	DebugShape newShape(shapeInfo);
	unsigned int vertexStart = s_debugRenderSystem->GetVertexCount(mode);
	s_debugRenderSystem->AddShape(newShape);

	if (mode == DebugRenderMode::XRAY) {
		unsigned int xRayStart = s_debugRenderSystem->GetVertexCount(DebugRenderMode::ALWAYS);
		
		shapeInfo.m_renderMode	= DebugRenderMode::ALWAYS;
		shapeInfo.m_startVertex = xRayStart;
		DebugShape xrayShape(shapeInfo);
		s_debugRenderSystem->AddShape(xrayShape);
	}

}

void DebugAddWorldLine(const Vec3& start, const Vec3& end, float radius, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
	Mat44 modelMatrix;
	unsigned int vertexCount = CalcVertCountForCylinder();
	unsigned int vertexStart = s_debugRenderSystem->GetVertexCount(mode);

	DebugShapeInfo shapeInfo	= {};
	shapeInfo.m_startVertex		= vertexStart;
	shapeInfo.m_vertexCount		= vertexCount;
	shapeInfo.m_flags			= DebugShapeFlagsBit::DebugWorldShape | DebugShapeFlagsBit::DebugWorldShapeValid;
	shapeInfo.m_renderMode		= mode;
	shapeInfo.m_duration		= duration;
	shapeInfo.m_radius			= radius;
	shapeInfo.m_startColor		= startColor;
	shapeInfo.m_endColor		= endColor;
	shapeInfo.m_shapeType		= DEBUG_RENDER_WORLD_LINE;

	DebugShape newShape(shapeInfo);
	s_debugRenderSystem->AddShape(newShape);

	if (mode == DebugRenderMode::XRAY) {
		unsigned int xrayStart = s_debugRenderSystem->GetVertexCount(DebugRenderMode::ALWAYS);
		shapeInfo.m_renderMode = DebugRenderMode::ALWAYS;

		DebugShape xrayShape(shapeInfo);

		s_debugRenderSystem->AddShape(xrayShape);
	}

}

void DebugAddWorldWireCylinder(const Vec3& base, const Vec3& top, float radius, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
	Mat44 modelMatrix;
	unsigned int vertexStart = s_debugRenderSystem->GetVertexCount(mode);
	unsigned int vertexCount = CalcVertCountForCylinder();

	DebugShapeInfo shapeInfo	= {};
	shapeInfo.m_startVertex		= vertexStart;
	shapeInfo.m_vertexCount		= vertexCount;
	shapeInfo.m_flags			= DebugShapeFlagsBit::DebugWorldShape | DebugShapeFlagsBit::DebugWorldShapeValid;
	shapeInfo.m_renderMode		= mode;
	shapeInfo.m_duration		= duration;
	shapeInfo.m_radius			= radius;
	shapeInfo.m_startColor		= startColor;
	shapeInfo.m_endColor		= endColor;
	shapeInfo.m_shapeType		= DEBUG_RENDER_WORLD_WIRE_CYLINDER;

	DebugShape newShape(shapeInfo);
	s_debugRenderSystem->AddShape(newShape);
}

void DebugAddWorldWireSphere(const Vec3& center, float radius, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode, int stacks, int slices)
{
	Mat44 modelMatrix;
	modelMatrix.AppendTranslation3D(center);
	unsigned int vertexCount = CalcVertCountForSphere(stacks, slices);

	unsigned int vertexStart = s_debugRenderSystem->GetVertexCount(mode);
	DebugShapeInfo shapeInfo = {};
	shapeInfo.m_startVertex = vertexStart;
	shapeInfo.m_vertexCount = vertexCount;
	shapeInfo.m_flags = DebugShapeFlagsBit::DebugWorldShape | DebugShapeFlagsBit::DebugWorldShapeValid;
	shapeInfo.m_stacks = stacks;
	shapeInfo.m_slices = slices;
	shapeInfo.m_renderMode = mode;
	shapeInfo.m_duration = duration;
	shapeInfo.m_radius = radius;
	shapeInfo.m_startColor = startColor;
	shapeInfo.m_endColor = endColor;
	shapeInfo.m_shapeType = DEBUG_RENDER_WORLD_WIRE_SPHERE;
}

void DebugAddWorldArrow(const Vec3& start, const Vec3& end, float radius, float duration, const Rgba8& baseColor, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
	Mat44 modelMatrix;
	unsigned int vertexCount = CalcVertCountForArrow3D();
	unsigned int vertexStart = s_debugRenderSystem->GetVertexCount(mode);

	DebugShapeInfo shapeInfo = {};
	shapeInfo.m_startVertex = vertexStart;
	shapeInfo.m_vertexCount = vertexCount;
	shapeInfo.m_flags = DebugShapeFlagsBit::DebugWorldShape | DebugShapeFlagsBit::DebugWorldShapeValid;
	shapeInfo.m_renderMode = mode;
	shapeInfo.m_duration = duration;
	shapeInfo.m_radius = radius;
	shapeInfo.m_startColor = startColor;
	shapeInfo.m_endColor = endColor;
	shapeInfo.m_shapeType = DEBUG_RENDER_WORLD_POINT;

	DebugShape newShape(shapeInfo);
	s_debugRenderSystem->AddShape(newShape);

	if (mode == DebugRenderMode::XRAY) {
		unsigned int xrayStart = s_debugRenderSystem->GetVertexCount(DebugRenderMode::ALWAYS);
		shapeInfo.m_renderMode = DebugRenderMode::ALWAYS;

		DebugShape xrayShape(shapeInfo);

		s_debugRenderSystem->AddShape(xrayShape);
	}
}

void DebugAddWorldBox(const AABB3& bounds, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
	Mat44 modelMatrix;
	unsigned int vertexCount = CalcVertCountForAABB3D();
	unsigned int vertexStart = s_debugRenderSystem->GetVertexCount(mode);

	DebugShapeInfo shapeInfo = {};
	shapeInfo.m_startVertex = vertexStart;
	shapeInfo.m_vertexCount = vertexCount;
	shapeInfo.m_flags = DebugShapeFlagsBit::DebugWorldShape | DebugShapeFlagsBit::DebugWorldShapeValid;
	shapeInfo.m_renderMode = mode;
	shapeInfo.m_duration = duration;
	shapeInfo.m_startColor = startColor;
	shapeInfo.m_endColor = endColor;
	shapeInfo.m_shapeType = DEBUG_RENDER_WORLD_BOX;

	DebugShape newShape(shapeInfo);
	s_debugRenderSystem->AddShape(newShape);

	if (mode == DebugRenderMode::XRAY) {
		unsigned int xrayStart = s_debugRenderSystem->GetVertexCount(DebugRenderMode::ALWAYS);
		shapeInfo.m_renderMode = DebugRenderMode::ALWAYS;

		DebugShape xrayShape(shapeInfo);

		s_debugRenderSystem->AddShape(xrayShape);
	}
}

void DebugAddWorldBasis(const Mat44& basis, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
	Mat44 modelMatrix;
	modelMatrix.AppendTranslation3D(basis.GetTranslation3D());

	unsigned int vertexCount = CalcVertCountForArrow3D() * 3;
	unsigned int vertexStart = s_debugRenderSystem->GetVertexCount(mode);
	constexpr float basisRadius = 0.075f;

	DebugShapeInfo shapeInfo = {};
	shapeInfo.m_startVertex = vertexStart;
	shapeInfo.m_vertexCount = vertexCount;
	shapeInfo.m_flags = DebugShapeFlagsBit::DebugWorldShape | DebugShapeFlagsBit::DebugWorldShapeValid;
	shapeInfo.m_renderMode = mode;
	shapeInfo.m_duration = duration;
	shapeInfo.m_startColor = startColor;
	shapeInfo.m_endColor = endColor;
	shapeInfo.m_shapeType = DEBUG_RENDER_WORLD_POINT;

	DebugShape newShape(shapeInfo);
	s_debugRenderSystem->AddShape(newShape);

	if (mode == DebugRenderMode::XRAY) {
		unsigned int xrayStart = s_debugRenderSystem->GetVertexCount(DebugRenderMode::ALWAYS);
		shapeInfo.m_renderMode = DebugRenderMode::ALWAYS;

		DebugShape xrayShape(shapeInfo);

		s_debugRenderSystem->AddShape(xrayShape);
	}
}

void DebugAddWorldText(const std::string& text, const Mat44& transform, float textHeight, const Vec2& alignment, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{

}

void DebugAddWorldBillboardText(const std::string& text, const Vec3& origin, float textHeight, const Vec2& alignment, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{

}

void DebugAddScreenText(const std::string& text, const Vec2& position, float duration, const Vec2& alignment, float size, const Rgba8& startColor, const Rgba8& endColor)
{

}

void DebugAddMessage(const std::string& text, float duration, const Rgba8& startColor, const Rgba8& endColor)
{

}

DebugRenderSystem::DebugRenderSystem(DebugRenderConfig config) :
	m_config(config)
{
	Renderer* renderer = m_config.m_renderer;
	unsigned int backbufferCount = renderer->GetBackBufferCount();

	char const* baseName = "DebugRendererCmdList";
	CommandListDesc cmdDesc = {};
	cmdDesc.m_type = CommandListType::DIRECT;

	m_cmdLists = new CommandList * [backbufferCount];

	for (int bufferIndex = 0; bufferIndex < backbufferCount; bufferIndex++)
	{
		cmdDesc.m_debugName = baseName;
		baseName += bufferIndex;
		m_cmdLists[bufferIndex] = renderer->CreateCommandList(cmdDesc);
	}

}

DebugRenderSystem::~DebugRenderSystem()
{
	Renderer* renderer = m_config.m_renderer;
	unsigned int backbufferCount = renderer->GetBackBufferCount();

	for (int bufferIndex = 0; bufferIndex < backbufferCount; bufferIndex++)
	{
		delete m_cmdLists[bufferIndex];
	}

	delete[] m_cmdLists;
	m_cmdLists = nullptr;
}

void DebugRenderSystem::Startup()
{
	// This is a totally arbitrary reserve size 
	static constexpr int reserveSize = 100;

	for (int debugRenderTypeIndex = 0; debugRenderTypeIndex < (int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugRenderTypeIndex++) {
		std::vector<DebugShape>& debugShapeList = m_debugShapes[debugRenderTypeIndex];
		debugShapeList.reserve(reserveSize);
	}
}

void DebugRenderSystem::Clear()
{
	for (int debugMode = 0; debugMode < (int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugMode++) {
		m_debugShapes[debugMode].clear();
	}
}

void DebugRenderSystem::RenderWorld(Camera const& worldCamera)
{



}

void DebugRenderSystem::RenderScreen(Camera const& screenCamera)
{

}

void DebugRenderSystem::ClearExpiredShapes()
{
	for (unsigned int debugMode = 0; debugMode < (int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugMode++) {
		std::vector<DebugShape>& shapesContainer = m_debugShapes[debugMode];

		for (unsigned int shapeIndex = 0; shapeIndex < shapesContainer.size(); shapeIndex++) {
			DebugShape& shape = shapesContainer[shapeIndex];
			if (shape.IsShapeValid() && shape.CanShapeBeDeleted()) {
				shape.MarkForDeletion();
			}
		}
	}

}

void DebugRenderSystem::AddShape(DebugShape const& newShape)
{
	DebugRenderMode renderMode = newShape.GetRenderMode();

	std::vector<DebugShape>& shapesContainer = m_debugShapes[(int)renderMode];
	unsigned int& vertexCount = m_vertexCounts[(int)renderMode];
	bool wasShapeAdded = false;
	for (unsigned int shapeIndex = 0; shapeIndex < shapesContainer.size(); shapeIndex++) {
		DebugShape& shape = shapesContainer[shapeIndex];
		if (!shape.IsShapeValid()) {
			shape = newShape;
			wasShapeAdded = true;
			break;
		}
	}

	// no slot was found, then pushback to vector
	if (wasShapeAdded)
	{
		shapesContainer.push_back(newShape);
	}

	vertexCount += newShape.GetVertexCount();
}

void DebugRenderSystem::AddVertsForShapes()
{
	// Go through all the shapes, and build add the verts to the vertex buffer
	for (unsigned int debugMode = 0; debugMode < (int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugMode++) {
		std::vector<DebugShape>& shapesContainer = m_debugShapes[debugMode];
		for (unsigned int shapeIndex = 0; shapeIndex < shapesContainer.size(); shapeIndex++) {
			DebugShape& shape = shapesContainer[shapeIndex];
			if(!shape.IsShapeValid()) continue; // Shape is marked as deleted, so continue

		
		}
	}
}
