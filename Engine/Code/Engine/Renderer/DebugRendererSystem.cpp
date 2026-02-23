#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/DebugShape.hpp"
#include "Engine/Renderer/Billboard.hpp"
#include "Engine/Renderer/Interfaces/Buffer.hpp"
#include "Engine/Renderer/Interfaces/Fence.hpp"
#include "Engine/Renderer/Interfaces/PipelineState.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Window/Window.hpp"
#include <vector>


constexpr unsigned int MODEL_BUFFER_COUNT = 4096;

class DebugRenderSystem {

public:
	DebugRenderSystem(DebugRenderConfig config);
	~DebugRenderSystem();

	// Getters
	Clock const& GetClock() const { return m_clock; }
	Fence* GetFence() const { return m_renderFence; }
	BitmapFont* GetFont() const { return m_font; }
	PipelineState* GetPSO(DebugRenderMode renderMode);

	// Setters
	void SetVisibility(bool isHidden) { m_hidden = isHidden; }
	void SetParentClock(Clock& parentClock) { m_clock.SetParent(parentClock); }
	void SetTimeDilation(double timeDilation) { m_clock.SetTimeDilation(timeDilation); }

	// Actions
	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();
	void Clear();
	void ClearVertices() { m_debugVerts.clear(); }
	void PreRenderPass();
	void RenderWorld(Camera const& worldCamera);
	void RenderScreen(Camera const& screenCamera);
	void ClearExpiredShapes();
	void ClearDescriptors();
	void ClearModelMatrices();
	void AddShape(DebugShape& newShape);
	void ResetCmdLists();

private:
	void AddVertsForShapes(Camera const& renderCam);
	void AddVertsForDebugShape(DebugShape const& shape);
	void CreatePSOs();
	void CreateRenderObjects();
	void CreateOrUpdateVertexBuffer();
	void CreateModelBuffers();
	void IssueDrawCalls(Camera const& camera);
	bool ContainsAnyWorldShapes() const;
	bool ContainsAnyScreenShapes() const;

	CommandList* GetCopyCmdList();
	Buffer*& GetVertexBuffer();
	Buffer* GetModelBuffer();

private:
	DebugRenderConfig m_config = {};
	Clock m_clock = {};
	bool m_hidden = false;
	bool m_hasAnyWorldShape = false;
	bool m_hasAnyScreenShape = false;
	unsigned int m_currentModelIndex = 0;
	CommandList** m_copyCommandLists = nullptr;
	RenderContext* m_renderContext = nullptr;
	PipelineState* m_debugPSOs[(int)DebugRenderMode::NUM_DEBUG_RENDER_MODES] = {};
	Buffer* m_modelCBO = nullptr;
	Buffer** m_vertexBuffers = nullptr;
	// Intermediate buffers, for copying to default faster GPU memory
	Buffer* m_intmVtxBuffer = nullptr;
	Buffer* m_intmModelBuffer = nullptr;
	Fence* m_copyFence = nullptr;
	Fence* m_renderFence = nullptr;
	std::vector<DebugShape> m_debugShapes[(int)DebugRenderMode::NUM_DEBUG_RENDER_MODES] = {};
	std::vector<Vertex_PCU> m_debugVerts = {};
	std::vector<ModelConstants> m_modelConstants = {};
	BitmapFont* m_font = nullptr;
};

DebugRenderSystem* s_debugRenderSystem = nullptr;

void DebugRenderSystemStartup(const DebugRenderConfig& config)
{
	s_debugRenderSystem = new DebugRenderSystem(config);
	s_debugRenderSystem->Startup();
}

void DebugRenderSystemShutdown()
{
	s_debugRenderSystem->Shutdown();
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
	s_debugRenderSystem->BeginFrame();
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
	s_debugRenderSystem->EndFrame();
}

Fence* DebugRenderGetFence()
{
	return s_debugRenderSystem->GetFence();
}


void DebugAddWorldPoint(const Vec3& pos, float radius, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode, int stacks, int slices)
{
	Mat44 modelMatrix;
	modelMatrix.AppendTranslation3D(pos);

	DebugShapeInfo shapeInfo = {};
	shapeInfo.m_flags = DebugShapeFlagsBit::DebugWorldShape | DebugShapeFlagsBit::DebugWorldShapeValid;
	shapeInfo.m_stacks = (unsigned char)stacks;
	shapeInfo.m_slices = (unsigned char)slices;
	shapeInfo.m_renderMode = mode;
	shapeInfo.m_duration = duration;
	shapeInfo.m_shapeSize = radius;
	shapeInfo.m_startColor = startColor;
	shapeInfo.m_endColor = endColor;
	shapeInfo.m_shapeType = DEBUG_RENDER_WORLD_POINT;

	DebugShape newShape(shapeInfo);
	s_debugRenderSystem->AddShape(newShape);

	if (mode == DebugRenderMode::XRAY) {
		shapeInfo.m_renderMode = DebugRenderMode::USE_DEPTH;
		shapeInfo.m_startColor.a = 120;
		shapeInfo.m_endColor.a = 120;

		DebugShape depthShape(shapeInfo);

		s_debugRenderSystem->AddShape(depthShape);
	}

}

void DebugAddWorldLine(const Vec3& start, const Vec3& end, float radius, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{

	DebugShapeInfo shapeInfo = {};
	shapeInfo.m_flags = DebugShapeFlagsBit::DebugWorldShape | DebugShapeFlagsBit::DebugWorldShapeValid;
	shapeInfo.m_renderMode = mode;
	shapeInfo.m_duration = duration;
	shapeInfo.m_shapeSize = radius;
	shapeInfo.m_startColor = startColor;
	shapeInfo.m_start = start;
	shapeInfo.m_end = end;
	shapeInfo.m_endColor = endColor;
	shapeInfo.m_shapeType = DEBUG_RENDER_WORLD_LINE;

	DebugShape newShape(shapeInfo);
	s_debugRenderSystem->AddShape(newShape);

	if (mode == DebugRenderMode::XRAY) {
		shapeInfo.m_renderMode = DebugRenderMode::USE_DEPTH;

		DebugShape depthShape(shapeInfo);

		s_debugRenderSystem->AddShape(depthShape);
	}

}

void DebugAddWorldWireCylinder(const Vec3& base, const Vec3& top, float radius, float duration, const Rgba8& startColor, const Rgba8& endColor)
{

	DebugShapeInfo shapeInfo = {};
	shapeInfo.m_flags = DebugShapeFlagsBit::DebugWorldShape | DebugShapeFlagsBit::DebugWorldShapeValid;
	shapeInfo.m_renderMode = DebugRenderMode::WIRE;
	shapeInfo.m_duration = duration;
	shapeInfo.m_shapeSize = radius;
	shapeInfo.m_startColor = startColor;
	shapeInfo.m_start = base;
	shapeInfo.m_end = top;
	shapeInfo.m_endColor = endColor;
	shapeInfo.m_shapeType = DEBUG_RENDER_WORLD_WIRE_CYLINDER;

	DebugShape newShape(shapeInfo);
	s_debugRenderSystem->AddShape(newShape);
}

void DebugAddWorldWireSphere(const Vec3& center, float radius, float duration, const Rgba8& startColor, const Rgba8& endColor, int stacks, int slices)
{
	Mat44 modelMatrix;
	modelMatrix.AppendTranslation3D(center);


	DebugShapeInfo shapeInfo = {};
	shapeInfo.m_flags = DebugShapeFlagsBit::DebugWorldShape | DebugShapeFlagsBit::DebugWorldShapeValid;
	shapeInfo.m_stacks = (unsigned char)stacks;
	shapeInfo.m_slices = (unsigned char)slices;
	shapeInfo.m_renderMode = DebugRenderMode::WIRE;
	shapeInfo.m_duration = duration;
	shapeInfo.m_shapeSize = radius;
	shapeInfo.m_startColor = startColor;
	shapeInfo.m_endColor = endColor;
	shapeInfo.m_modelMatrix = modelMatrix;
	shapeInfo.m_shapeType = DEBUG_RENDER_WORLD_WIRE_SPHERE;

	DebugShape newShape(shapeInfo);
	s_debugRenderSystem->AddShape(newShape);
}

void DebugAddWorldArrow(const Vec3& start, const Vec3& end, float radius, float duration, const Rgba8& baseColor, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{

	DebugShapeInfo shapeInfo = {};
	shapeInfo.m_flags = DebugShapeFlagsBit::DebugWorldShape | DebugShapeFlagsBit::DebugWorldShapeValid;
	shapeInfo.m_renderMode = mode;
	shapeInfo.m_duration = duration;
	shapeInfo.m_shapeSize = radius;
	shapeInfo.m_start = start;
	shapeInfo.m_end = end;
	shapeInfo.m_startColor = startColor;
	shapeInfo.m_endColor = endColor;
	shapeInfo.m_shapeType = DEBUG_RENDER_WORLD_POINT;

	DebugShape newShape(shapeInfo);
	s_debugRenderSystem->AddShape(newShape);

	if (mode == DebugRenderMode::XRAY) {
		shapeInfo.m_renderMode = DebugRenderMode::USE_DEPTH;
		shapeInfo.m_startColor.a = 120;
		shapeInfo.m_endColor.a = 120;

		DebugShape depthShape(shapeInfo);

		s_debugRenderSystem->AddShape(depthShape);
	}
}

void DebugAddWorldBox(const AABB3& bounds, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{

	DebugShapeInfo shapeInfo = {};
	shapeInfo.m_flags = DebugShapeFlagsBit::DebugWorldShape | DebugShapeFlagsBit::DebugWorldShapeValid;
	shapeInfo.m_renderMode = mode;
	shapeInfo.m_duration = duration;
	shapeInfo.m_startColor = startColor;
	shapeInfo.m_start = bounds.m_mins;
	shapeInfo.m_end = bounds.m_maxs;
	shapeInfo.m_endColor = endColor;
	shapeInfo.m_shapeType = DEBUG_RENDER_WORLD_BOX;

	DebugShape newShape(shapeInfo);
	s_debugRenderSystem->AddShape(newShape);

	if (mode == DebugRenderMode::XRAY) {
		shapeInfo.m_renderMode = DebugRenderMode::USE_DEPTH;
		shapeInfo.m_startColor.a = 120;
		shapeInfo.m_endColor.a = 120;

		DebugShape depthShape(shapeInfo);

		s_debugRenderSystem->AddShape(depthShape);
	}
}

void DebugAddWorldBasis(const Mat44& basis, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
	Mat44 modelMatrix;
	modelMatrix.AppendTranslation3D(basis.GetTranslation3D());

	constexpr float basisRadius = 0.075f;

	DebugShapeInfo shapeInfo = {};
	shapeInfo.m_flags = DebugShapeFlagsBit::DebugWorldShape | DebugShapeFlagsBit::DebugWorldShapeValid;
	shapeInfo.m_renderMode = mode;
	shapeInfo.m_duration = duration;
	shapeInfo.m_startColor = startColor;
	shapeInfo.m_modelMatrix = modelMatrix;
	shapeInfo.m_endColor = endColor;
	shapeInfo.m_shapeSize = basisRadius;
	shapeInfo.m_shapeType = DEBUG_RENDER_WORLD_BASIS;

	DebugShape newShape(shapeInfo);
	s_debugRenderSystem->AddShape(newShape);

	if (mode == DebugRenderMode::XRAY) {
		shapeInfo.m_renderMode = DebugRenderMode::USE_DEPTH;
		shapeInfo.m_startColor.a = 120;
		shapeInfo.m_endColor.a = 120;

		DebugShape depthShape(shapeInfo);

		s_debugRenderSystem->AddShape(depthShape);
	}
}

void DebugAddWorldText(const std::string& text, const Mat44& transform, float textHeight, const Vec2& alignment, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
	BitmapFont* font = s_debugRenderSystem->GetFont();

	unsigned int flags = DebugShapeFlagsBit::DebugWorldShape;
	flags |= DebugShapeFlagsBit::DebugWorldShapeValid;
	flags |= DebugShapeFlagsBit::DebugWorldShapeText;

	DebugShapeInfo shapeInfo = {};
	shapeInfo.m_flags = flags;
	shapeInfo.m_renderMode = mode;
	shapeInfo.m_duration = duration;
	shapeInfo.m_startColor = startColor;
	shapeInfo.m_modelMatrix = transform;
	shapeInfo.m_endColor = endColor;
	shapeInfo.m_shapeSize = textHeight;
	// Packaging the alignment in the start vec
	shapeInfo.m_start = Vec3(alignment);
	shapeInfo.m_shapeType = DEBUG_RENDER_WORLD_TEXT;
	shapeInfo.m_text = text;

	DebugShape newShape(shapeInfo);
	s_debugRenderSystem->AddShape(newShape);

	if (mode == DebugRenderMode::XRAY) {
		shapeInfo.m_renderMode = DebugRenderMode::USE_DEPTH_NO_CULL;
		shapeInfo.m_startColor.a = 120;
		shapeInfo.m_endColor.a = 120;

		DebugShape depthShape(shapeInfo);

		s_debugRenderSystem->AddShape(depthShape);
	}
}

void DebugAddWorldBillboardText(const std::string& text, const Vec3& origin, float textHeight, const Vec2& alignment, float duration, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
	BitmapFont* font = s_debugRenderSystem->GetFont();


	Mat44 modelMatrix;
	modelMatrix.SetTranslation3D(origin);

	unsigned int flags = DebugShapeFlagsBit::DebugWorldShape;
	flags |= DebugShapeFlagsBit::DebugWorldShapeValid;
	flags |= DebugShapeFlagsBit::DebugWorldShapeText;

	DebugShapeInfo shapeInfo = {};
	shapeInfo.m_flags = flags;
	shapeInfo.m_renderMode = mode;
	shapeInfo.m_duration = duration;
	shapeInfo.m_startColor = startColor;
	shapeInfo.m_modelMatrix = modelMatrix;
	shapeInfo.m_endColor = endColor;
	shapeInfo.m_shapeSize = textHeight;
	// Packaging the alignment in the start vec
	shapeInfo.m_start = Vec3(alignment);
	shapeInfo.m_shapeType = DEBUG_RENDER_WORLD_BILLBOARD_TEXT;
	shapeInfo.m_text = text;

	DebugShape newShape(shapeInfo);
	s_debugRenderSystem->AddShape(newShape);

	if (mode == DebugRenderMode::XRAY) {
		shapeInfo.m_renderMode = DebugRenderMode::USE_DEPTH;
		shapeInfo.m_startColor.a = 120;
		shapeInfo.m_endColor.a = 120;

		DebugShape depthShape(shapeInfo);

		s_debugRenderSystem->AddShape(depthShape);
	}
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

}

DebugRenderSystem::~DebugRenderSystem()
{
}


PipelineState* DebugRenderSystem::GetPSO(DebugRenderMode renderMode)
{
	return m_debugPSOs[(unsigned int)renderMode];
}

void DebugRenderSystem::Startup()
{
	// This is a totally arbitrary reserve size 
	static constexpr int reserveSize = 100;

	for (int debugRenderTypeIndex = 0; debugRenderTypeIndex < (int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugRenderTypeIndex++) {
		std::vector<DebugShape>& debugShapeList = m_debugShapes[debugRenderTypeIndex];
		debugShapeList.reserve(reserveSize);
	}

	CreatePSOs();
	CreateRenderObjects();

	m_font = m_config.m_renderer->CreateOrGetBitmapFont("Data/Images/SquirrelFixedFont");

	unsigned int backBufferCount = m_config.m_renderer->GetBackBufferCount();
	m_vertexBuffers = new Buffer * [backBufferCount];

	// Each debug object requires a model buffer, so a fair estimate we're going for is 4096 rn.
	m_modelCBO = new Buffer[MODEL_BUFFER_COUNT];
	memset(m_vertexBuffers, 0, sizeof(Buffer*) * backBufferCount);
	m_renderContext->CloseAll();


}

void DebugRenderSystem::Shutdown()
{
	m_copyFence->Signal();
	m_copyFence->Wait();

	m_renderFence->Signal();
	m_renderFence->Wait();

	Renderer* renderer = m_config.m_renderer;
	for (unsigned int debugRenderTypeIndex = 0; debugRenderTypeIndex < (unsigned int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugRenderTypeIndex++) {
		PipelineState*& pso = m_debugPSOs[debugRenderTypeIndex];
		delete pso;
		pso = nullptr;
	}

	for (unsigned int bufferIndex = 0; bufferIndex < renderer->GetBackBufferCount(); bufferIndex++) {
		delete m_vertexBuffers[bufferIndex];
		m_vertexBuffers[bufferIndex] = nullptr;

		delete m_copyCommandLists[bufferIndex];
		m_copyCommandLists[bufferIndex] = nullptr;
	}


	delete[] m_copyCommandLists;
	delete[] m_vertexBuffers;
	delete[] m_modelCBO;

	delete m_intmVtxBuffer;
	delete m_intmModelBuffer;
	delete m_renderContext;
	delete m_copyFence;
	delete m_renderFence;

	m_modelCBO = nullptr;
	m_vertexBuffers = nullptr;
	m_intmVtxBuffer = nullptr;
	m_intmModelBuffer = nullptr;
	m_renderContext = nullptr;
	m_copyFence = nullptr;
	m_renderFence = nullptr;

}

void DebugRenderSystem::BeginFrame()
{
	Renderer* renderer = m_config.m_renderer;
	m_renderFence->Signal();
	m_renderFence->Wait();
	ResetCmdLists();

	ClearVertices();
	ClearDescriptors();

	BitmapFont* font = GetFont();
	Texture* defaultTexture = renderer->GetDefaultTexture();
	Texture* fontTexture = &font->GetTexture();

	// Descriptors are copied onto the GPU heap, so we have to place the texture descriptors there, every frame
	D3D12_CPU_DESCRIPTOR_HANDLE nextTexDescriptor = m_renderContext->GetNextCPUDescriptor(PARAM_TEXTURES);
	renderer->CreateShaderResourceView(nextTexDescriptor.ptr, defaultTexture);

	nextTexDescriptor = m_renderContext->GetNextCPUDescriptor(PARAM_TEXTURES);
	renderer->CreateShaderResourceView(nextTexDescriptor.ptr, fontTexture);

}

void DebugRenderSystem::Clear()
{
	for (int debugMode = 0; debugMode < (int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugMode++) {
		m_debugShapes[debugMode].clear();
	}

	// All shapes were emptied, so there is no shape contained
	m_hasAnyWorldShape = false;
}

void DebugRenderSystem::PreRenderPass()
{
	Renderer* renderer = m_config.m_renderer;
	CommandList* copyCmdList = GetCopyCmdList();

	// Create vertex buffer
	CreateOrUpdateVertexBuffer();

	// Create model buffer list with initial Data upload
	CreateModelBuffers();

	copyCmdList->Close();
	renderer->ExecuteCmdLists(CommandListType::COPY, 1, &copyCmdList);

	m_copyFence->SignalGPU();
	m_copyFence->Wait();

	renderer->InsertWaitInQueue(CommandListType::DIRECT, m_copyFence);
}

void DebugRenderSystem::RenderWorld(Camera const& worldCamera)
{
	Renderer* renderer = m_config.m_renderer;
	CommandList* cmdList = m_renderContext->GetCommandList();

	bool hasAnyShapes = ContainsAnyWorldShapes();
	if (hasAnyShapes) {
		// Construct all the shapes
		AddVertsForShapes(worldCamera);

		PreRenderPass();

		// issue render calls, normal draw calls first
		IssueDrawCalls(worldCamera);
	}
	else {
		// Close copy command list, to make sure it's ready for next frame's use, even if no vtx buffer was created
		CommandList* copyCmdList = GetCopyCmdList();
		copyCmdList->Close();
	}

	// Always close and execute every frame, or we risk having the command list in an undefined state
	m_renderContext->Close();
	renderer->ExecuteCmdLists(CommandListType::DIRECT, 1, &cmdList);

	m_renderFence->SignalGPU();
	m_renderFence->Wait();

}

void DebugRenderSystem::RenderScreen(Camera const& screenCamera)
{
	Renderer* renderer = m_config.m_renderer;
	CommandList* cmdList = m_renderContext->GetCommandList();

	bool hasAnyShapes = ContainsAnyWorldShapes();
}

void DebugRenderSystem::ClearExpiredShapes()
{
	bool wasAnyShapeValid = false;
	for (unsigned int debugMode = 0; debugMode < (int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugMode++) {
		std::vector<DebugShape>& shapesContainer = m_debugShapes[debugMode];

		for (unsigned int shapeIndex = 0; shapeIndex < shapesContainer.size(); shapeIndex++) {
			DebugShape& shape = shapesContainer[shapeIndex];
			if (shape.IsShapeValid()) {
				if (shape.CanShapeBeDeleted()) {
					shape.MarkForDeletion();
				}
				else {
					// There are shapes that have persisted through this frame
					wasAnyShapeValid = true;
				}
			}
		}
	}
	m_hasAnyWorldShape = wasAnyShapeValid;
}

void DebugRenderSystem::ClearDescriptors()
{
	m_renderContext->ResetDescriptors();
}

void DebugRenderSystem::ClearModelMatrices()
{
	m_currentModelIndex = 0;
	m_modelConstants.clear();
}

void DebugRenderSystem::AddShape(DebugShape& newShape)
{
	DebugRenderMode renderMode = newShape.GetRenderMode();
	if (renderMode == DebugRenderMode::XRAY) {
		newShape.m_info.m_startColor.a = 120;
		newShape.m_info.m_endColor.a = 120;
	}

	DebugShape* pAddedShape = nullptr;
	std::vector<DebugShape>& shapesContainer = m_debugShapes[(int)renderMode];

	for (unsigned int shapeIndex = 0; shapeIndex < shapesContainer.size(); shapeIndex++) {
		DebugShape& shape = shapesContainer[shapeIndex];
		if (!shape.IsShapeValid()) {
			shape = newShape;
			pAddedShape = &shape;
			break;
		}
	}

	// no slot was found, then pushback to vector
	if (!pAddedShape)
	{
		shapesContainer.push_back(newShape);
		pAddedShape = &shapesContainer.back();
	}

	pAddedShape->StartWatch(GetClock());

	m_hasAnyWorldShape = true;
}

void DebugRenderSystem::ResetCmdLists()
{
	m_renderContext->Reset();
	CommandList* cmdList = GetCopyCmdList();
	cmdList->Reset();
}

void DebugRenderSystem::EndFrame()
{
	ClearExpiredShapes();
	ClearModelMatrices();

	m_renderContext->EndFrame();
	if (m_intmVtxBuffer) {
		delete m_intmVtxBuffer;
		m_intmVtxBuffer = nullptr;
	}

	if (m_intmModelBuffer) {
		delete m_intmModelBuffer;
		m_intmModelBuffer = nullptr;
	}
}

void DebugRenderSystem::AddVertsForShapes(Camera const& renderCam)
{
	unsigned int currentVertexCount = 0;
	// Go through all the shapes, and build add the verts to the vertex buffer
	for (unsigned int debugMode = 0; debugMode < (int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugMode++) {
		std::vector<DebugShape>& shapesContainer = m_debugShapes[debugMode];
		for (unsigned int shapeIndex = 0; shapeIndex < (unsigned int)shapesContainer.size(); shapeIndex++) {
			DebugShape& shape = shapesContainer[shapeIndex];

			if (!shape.IsShapeValid()) continue; // Shape is marked as deleted, so continue

			// Set vertex offset for rendering
			AddVertsForDebugShape(shape);
			shape.m_info.m_vertexStart = currentVertexCount;

			unsigned int shapeVertexCount = m_debugVerts.size() - currentVertexCount;
			currentVertexCount = m_debugVerts.size();
			shape.m_info.m_vertexCount = shapeVertexCount;

			shape.m_modelMatrixOffset = (unsigned int)m_modelConstants.size();
			ModelConstants newModelConstants = {};
			Rgba8 modelColor = shape.GetModelColor();
			modelColor.GetAsFloats(newModelConstants.ModelColor);

			if (shape.IsBillboarded()) {
				Vec3 translation = shape.GetModelMatrix().GetTranslation3D();
				newModelConstants.ModelMatrix = shape.GetBillboardModelMatrix(renderCam);
				newModelConstants.ModelMatrix.SetTranslation3D(translation);

			}
			else {
				newModelConstants.ModelMatrix = shape.GetModelMatrix();
			}

			m_modelConstants.push_back(newModelConstants);
		}
	}
}

void DebugRenderSystem::AddVertsForDebugShape(DebugShape const& shape)
{
	BitmapFont* font = GetFont();

	switch (shape.GetType())
	{
	case DEBUG_RENDER_NUM_TYPES:
	default:
	case INVALID:
		ERROR_AND_DIE("UNKNOWN DEBUG SHAPE");
		break;
	case DEBUG_RENDER_WORLD_WIRE_SPHERE:
	case DEBUG_RENDER_WORLD_POINT:
		AddVertsForSphere(m_debugVerts, shape.GetShapeSize(), shape.GetStacks(), shape.GetSlices(), shape.GetModelColor());
		break;
	case DEBUG_RENDER_WORLD_WIRE_CYLINDER:
	case DEBUG_RENDER_WORLD_LINE:
		AddVertsForCylinder(m_debugVerts, shape.GetStart(), shape.GetEnd(), shape.GetShapeSize(), shape.GetSlices(), shape.GetModelColor());
		break;
	case DEBUG_RENDER_WORLD_ARROW:
		AddVertsForArrow3D(m_debugVerts, shape.GetStart(), shape.GetEnd(), shape.GetShapeSize(), shape.GetSlices(), shape.GetModelColor());
		break;
	case DEBUG_RENDER_WORLD_BOX:
	case DEBUG_RENDER_WORLD_WIRE_BOX:
	{
		AABB3 bounds(shape.GetStart(), shape.GetEnd());
		AddVertsForAABB3D(m_debugVerts, bounds, shape.GetModelColor());
		break;
	}
	case DEBUG_RENDER_WORLD_BASIS:
	{
		Mat44 const& modelMat = shape.GetModelMatrix();
		Vec3 iBasis = modelMat.GetIBasis3D();
		Vec3 jBasis = modelMat.GetJBasis3D();
		Vec3 kBasis = modelMat.GetKBasis3D();

		float radius = shape.GetShapeSize();
		int slices = (int)shape.GetSlices();

		// Hardcoded colors for basis always
		AddVertsForArrow3D(m_debugVerts, Vec3::ZERO, iBasis, radius, slices, Rgba8::RED);
		AddVertsForArrow3D(m_debugVerts, Vec3::ZERO, jBasis, radius, slices, Rgba8::GREEN);
		AddVertsForArrow3D(m_debugVerts, Vec3::ZERO, kBasis, radius, slices, Rgba8::BLUE);
		break;
	}
	case DEBUG_RENDER_WORLD_TEXT:
	case DEBUG_RENDER_WORLD_BILLBOARD_TEXT:
	{
		AABB2 textBox;
		float textHeight = shape.GetShapeSize();
		Vec2 alignment = Vec2(shape.GetStart());
		textBox.SetDimensions(Vec2(font->GetTextWidth(textHeight, shape.m_info.m_text), textHeight));
		font->AddVertsForTextInBox2D(m_debugVerts, textBox, textHeight, shape.m_info.m_text, Rgba8::WHITE, 1.0f, alignment);
		break;
	}
	case DEBUG_RENDER_SCREEN_TEXT:
		break;
	case DEBUG_RENDER_MESSAGE:
		break;
		break;
	}

}

void DebugRenderSystem::CreatePSOs()
{
	Renderer* renderer = m_config.m_renderer;
	// Get Shader
	ShaderPipeline legacyForward = renderer->GetEngineShader(EngineShaderPipelines::LegacyForward);

	// Debug Depth
	PipelineStateDesc psoDesc = {};
	psoDesc.m_blendModes[0] = { BlendMode::OPAQUE };
	psoDesc.m_windingOrder = WindingOrder::COUNTERCLOCKWISE;
	psoDesc.m_cullMode = CullMode::BACK;
	psoDesc.m_fillMode = FillMode::SOLID;
	psoDesc.m_topology = TopologyType::TRIANGLELIST;
	psoDesc.m_depthEnable = true;
	psoDesc.m_depthFunc = DepthFunc::LESSEQUAL;
	psoDesc.m_depthStencilFormat = TextureFormat::D24_UNORM_S8_UINT;
	psoDesc.m_debugName = "DebugDepth";
	psoDesc.m_renderTargetCount = 1;
	psoDesc.m_renderTargetFormats[0] = TextureFormat::R8G8B8A8_UNORM;
	psoDesc.m_type = PipelineType::Graphics;
	psoDesc.m_byteCodes[ShaderType::Vertex] = legacyForward.m_firstShader;
	psoDesc.m_byteCodes[ShaderType::Pixel] = legacyForward.m_pixelShader;

	m_debugPSOs[(int)DebugRenderMode::USE_DEPTH] = renderer->CreatePipelineState(psoDesc);

	// Depth no cull
	psoDesc.m_cullMode = CullMode::NONE;
	m_debugPSOs[(int)DebugRenderMode::USE_DEPTH_NO_CULL] = renderer->CreatePipelineState(psoDesc);

	// WIRE is same as depth, but wire mode
	psoDesc.m_fillMode = FillMode::WIREFRAME;
	m_debugPSOs[(int)DebugRenderMode::WIRE] = renderer->CreatePipelineState(psoDesc);

	// Always
	psoDesc.m_blendModes[0] = BlendMode::ALPHA;
	psoDesc.m_fillMode = FillMode::SOLID;
	psoDesc.m_cullMode = CullMode::NONE;
	psoDesc.m_depthFunc = DepthFunc::ALWAYS;
	psoDesc.m_depthEnable = false;
	m_debugPSOs[(int)DebugRenderMode::ALWAYS] = renderer->CreatePipelineState(psoDesc);

	// XRAY Identical to always, but we do cull back faces
	psoDesc.m_cullMode = CullMode::BACK;
	m_debugPSOs[(int)DebugRenderMode::XRAY] = renderer->CreatePipelineState(psoDesc);

	// TEXT
	m_debugPSOs[(int)DebugRenderMode::SCREENTEXT] = renderer->CreatePipelineState(psoDesc);

}

void DebugRenderSystem::CreateRenderObjects()
{
	Renderer* renderer = m_config.m_renderer;
	unsigned int backbufferCount = renderer->GetBackBufferCount();

	// We need a lot of model cbuffer descriptors
	unsigned int descriptorCounts[4] = { 4096, 1, 1, 1 };
	unsigned int rscDescriptorCounts = descriptorCounts[(int)DescriptorHeapType::CBV_SRV_UAV];

	RenderContextDesc renderCtxDesc = {};
	renderCtxDesc.m_cmdListDesc.m_type = CommandListType::DIRECT;
	renderCtxDesc.m_cmdListDesc.m_debugName = "DebugCmdList";
	renderCtxDesc.m_descriptorCounts = descriptorCounts;
	renderCtxDesc.m_renderer = m_config.m_renderer;
	renderCtxDesc.m_rscDescriptorDistribution[PARAM_CAMERA_BUFFERS] = 2;				// Just 2 needed for world and UI
	renderCtxDesc.m_rscDescriptorDistribution[PARAM_MODEL_BUFFERS] = 4086;
	renderCtxDesc.m_rscDescriptorDistribution[PARAM_DRAW_INFO_BUFFERS] = 0;
	renderCtxDesc.m_rscDescriptorDistribution[PARAM_GAME_BUFFERS] = 0;
	renderCtxDesc.m_rscDescriptorDistribution[PARAM_TEXTURES] = 8;				// At least 2 for default and font tex
	renderCtxDesc.m_rscDescriptorDistribution[PARAM_GAME_UAVS] = 0;

	m_renderContext = new RenderContext(renderCtxDesc);


	CommandListDesc copyCmdListDesc = {};
	std::string copyDebugName = "DbgCopyCmdList";;
	copyCmdListDesc.m_initialState = nullptr;
	copyCmdListDesc.m_type = CommandListType::COPY;
	m_copyCommandLists = new CommandList * [backbufferCount];

	for (unsigned int bufferIndex = 0; bufferIndex < backbufferCount; bufferIndex++) {
		copyDebugName += std::to_string(bufferIndex);
		m_copyCommandLists[bufferIndex] = renderer->CreateCommandList(copyCmdListDesc);
		m_copyCommandLists[bufferIndex]->Close();
	}

	m_copyFence = renderer->CreateFence(CommandListType::COPY);
	m_renderFence = renderer->CreateFence(CommandListType::DIRECT);

	DescriptorHeap* sampleHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::Sampler);
	D3D12_CPU_DESCRIPTOR_HANDLE samplerHandle = sampleHeap->GetNextCPUHandle();
	renderer->CreateSampler(samplerHandle.ptr, SamplerMode::BILINEARWRAP);

}

void DebugRenderSystem::CreateOrUpdateVertexBuffer()
{
	Renderer* renderer = m_config.m_renderer;
	BufferDesc bufferDesc = {};
	bufferDesc.m_type = BufferType::Vertex;
	bufferDesc.m_debugName = "DebugVtxBuff";
	bufferDesc.m_memoryUsage = MemoryUsage::Default;
	bufferDesc.m_data = nullptr; // Manually handling data copying
	bufferDesc.m_size = m_debugVerts.size() * sizeof(Vertex_PCU);
	bufferDesc.m_stride.m_strideBytes = sizeof(Vertex_PCU);

	// Create buffer
	Buffer*& currentVtxBuffer = GetVertexBuffer();
	CommandList* copyCmdList = GetCopyCmdList();
	// Warning, this is borderline dereferencing a nullptr. It only works because of the first condition
	bool shouldCreateVtxBuffer = (currentVtxBuffer == nullptr) || (currentVtxBuffer->GetSize() < bufferDesc.m_size);

	if (shouldCreateVtxBuffer) {
		if (currentVtxBuffer) delete currentVtxBuffer;

		currentVtxBuffer = renderer->CreateBuffer(bufferDesc);
	}

	if (!m_intmVtxBuffer) {
		BufferDesc intermediateDesc = bufferDesc;
		intermediateDesc.m_memoryUsage = MemoryUsage::Dynamic;
		intermediateDesc.m_debugName = "Intermediate Buffer";
		m_intmVtxBuffer = renderer->CreateBuffer(intermediateDesc);
	}

	// update vertex buffer/upload verts
	TransitionBarrier copyBarriers[2] = {};
	copyBarriers[0] = TransitionBarrier(currentVtxBuffer, Common, CopyDest);
	copyBarriers[1] = TransitionBarrier(m_intmVtxBuffer, Common, CopySrc);

	copyCmdList->ResourceBarrier(_countof(copyBarriers), copyBarriers);

	m_intmVtxBuffer->CopyToBuffer(m_debugVerts.data(), bufferDesc.m_size);

	copyCmdList->CopyBuffer(currentVtxBuffer, m_intmVtxBuffer);

}

void DebugRenderSystem::CreateModelBuffers()
{
	Renderer* renderer = m_config.m_renderer;
	CommandList* cmdList = m_renderContext->GetCommandList();

	BufferDesc bufferDesc = {};
	bufferDesc.m_type = BufferType::Constant;
	bufferDesc.m_debugName = "DebugModelBuff";
	bufferDesc.m_memoryUsage = MemoryUsage::Dynamic;
	bufferDesc.m_size = sizeof(ModelConstants);
	bufferDesc.m_stride.m_strideBytes = sizeof(ModelConstants);

	TransitionBarrier* transitionBarriers = new TransitionBarrier[m_modelConstants.size()];

	for (unsigned int modelIndex = 0; modelIndex < (unsigned int)m_modelConstants.size(); modelIndex++) {
		Buffer* currentModelCBO = GetModelBuffer();
		// Release previously used model buffer
		if (currentModelCBO->m_rawRsc) {
			currentModelCBO->ReleaseResource();
		}

		bufferDesc.m_data = &m_modelConstants[modelIndex];

		renderer->CreateBuffer(bufferDesc, &currentModelCBO);
		D3D12_CPU_DESCRIPTOR_HANDLE descriptor = m_renderContext->GetNextCPUDescriptor(PARAM_MODEL_BUFFERS);
		renderer->CreateConstantBufferView(descriptor.ptr, currentModelCBO);
		TransitionBarrier modelBarrier = currentModelCBO->GetTransitionBarrier(ResourceStates::VertexAndCBuffer);
		transitionBarriers[modelIndex] = modelBarrier;
	}

	cmdList->ResourceBarrier(m_modelConstants.size(), transitionBarriers);
	delete[] transitionBarriers;
}

void DebugRenderSystem::IssueDrawCalls(Camera const& camera)
{

	CommandList* cmdList = m_renderContext->GetCommandList();
	Renderer* renderer = m_config.m_renderer;

	Texture* depthTarget = camera.GetDepthTarget();
	Texture* renderTarget = camera.GetRenderTarget();

	DescriptorHeap* resourcesHeap = m_renderContext->GetCPUDescriptorHeap(DescriptorHeapType::CBV_SRV_UAV);
	DescriptorHeap* GPUresourcesHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::CBV_SRV_UAV);
	DescriptorHeap* GPUsamplerHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::Sampler);

	// The camera needs to be added to the camera descriptors
	D3D12_CPU_DESCRIPTOR_HANDLE nextCameraHandle = m_renderContext->GetNextCPUDescriptor(PARAM_CAMERA_BUFFERS);
	renderer->CreateConstantBufferView(nextCameraHandle.ptr, camera.GetCameraBuffer());

	renderer->CopyDescriptorHeap(m_renderContext->GetDescriptorCountForCopy(), GPUresourcesHeap, resourcesHeap);

	DescriptorHeap* rtHeap = m_renderContext->GetDescriptorHeap(DescriptorHeapType::RenderTargetView);
	D3D12_CPU_DESCRIPTOR_HANDLE rtHandle = rtHeap->GetNextCPUHandle();
	renderer->CreateRenderTargetView(rtHandle.ptr, renderTarget);


	m_renderContext->BeginCamera(camera);

	Buffer* vertexBuffer = GetVertexBuffer();

	TransitionBarrier rscBarriers[3] = {};
	// Transition vertex Buffer and model buffer
	rscBarriers[0] = TransitionBarrier(vertexBuffer, ResourceStates::CopyDest, ResourceStates::VertexAndCBuffer);
	rscBarriers[1] = renderTarget->GetTransitionBarrier(ResourceStates::RenderTarget);
	rscBarriers[2] = depthTarget->GetTransitionBarrier(ResourceStates::DepthWrite);

	unsigned int cameraDescriptorStart = m_renderContext->GetDescriptorStart(PARAM_CAMERA_BUFFERS);
	unsigned int modelDescriptorStart = m_renderContext->GetDescriptorStart(PARAM_MODEL_BUFFERS);
	unsigned int textureDescriptorStart = m_renderContext->GetDescriptorStart(PARAM_TEXTURES);

	cmdList->ResourceBarrier(_countof(rscBarriers), rscBarriers);
	cmdList->SetRenderTargets(1, &renderTarget, false, depthTarget);
	cmdList->SetTopology(TopologyType::TRIANGLELIST);
	cmdList->SetVertexBuffers(&vertexBuffer, 1);

	unsigned int drawConstants[16] = {};

	for (unsigned int debugMode = 0; debugMode < (int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugMode++) {
		std::vector<DebugShape>& shapesContainer = m_debugShapes[debugMode];
		PipelineState* pso = GetPSO((DebugRenderMode)debugMode);

		cmdList->BindPipelineState(pso);
		cmdList->SetDescriptorSet(m_renderContext->GetDescriptorSet());
		cmdList->SetDescriptorTable(PARAM_CAMERA_BUFFERS, GPUresourcesHeap->GetGPUHandleAtOffset(cameraDescriptorStart), PipelineType::Graphics);
		cmdList->SetDescriptorTable(PARAM_MODEL_BUFFERS, GPUresourcesHeap->GetGPUHandleAtOffset(modelDescriptorStart), PipelineType::Graphics);
		cmdList->SetDescriptorTable(PARAM_TEXTURES, GPUresourcesHeap->GetGPUHandleAtOffset(textureDescriptorStart), PipelineType::Graphics);
		cmdList->SetDescriptorTable(PARAM_SAMPLERS, GPUsamplerHeap->GetGPUHandleHeapStart(), PipelineType::Graphics);

		for (unsigned int shapeIndex = 0; shapeIndex < shapesContainer.size(); shapeIndex++) {
			DebugShape& shape = shapesContainer[shapeIndex];

			if (!shape.IsShapeValid()) continue; // Shape is marked as deleted, so continue

			drawConstants[0] = 0;	// Camera Index
			drawConstants[1] = shape.m_modelMatrixOffset;		// Matrix Index
			drawConstants[2] = (shape.IsTextType()) ? 1 : 0;	// Texture Index 1 is font texture always

			unsigned int shapeVertexCount = shape.GetVertexCount();
			unsigned int shapeVertexStart = shape.GetVertexStart();
			cmdList->SetGraphicsRootConstants(16, drawConstants);
			cmdList->DrawInstance(shapeVertexCount, 1, shapeVertexStart, 0);
		}
	}

	m_renderContext->EndCamera(camera);
}


bool DebugRenderSystem::ContainsAnyWorldShapes() const
{
	return m_hasAnyWorldShape;
}

bool DebugRenderSystem::ContainsAnyScreenShapes() const
{
	return m_hasAnyScreenShape;
}


CommandList* DebugRenderSystem::GetCopyCmdList()
{
	unsigned int currentBackBuferIndex = m_renderContext->GetBufferIndex();
	return m_copyCommandLists[currentBackBuferIndex];
}

Buffer*& DebugRenderSystem::GetVertexBuffer()
{
	unsigned int currentBackBuferIndex = m_renderContext->GetBufferIndex();
	return m_vertexBuffers[currentBackBuferIndex];
}

Buffer* DebugRenderSystem::GetModelBuffer()
{
	GUARANTEE_OR_DIE(m_currentModelIndex < MODEL_BUFFER_COUNT, "RAN OUT OF MODEL BUFFERS FOR DEBUG RENDER SYSTEM");
	Buffer* newModelCBO = &m_modelCBO[m_currentModelIndex];
	m_currentModelIndex++;
	return newModelCBO;
}
