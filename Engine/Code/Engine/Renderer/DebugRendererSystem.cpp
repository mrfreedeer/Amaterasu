#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/DebugShape.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Renderer/Billboard.hpp"
#include "Engine/Renderer/Interfaces/Buffer.hpp"
#include "Engine/Renderer/Interfaces/Fence.hpp"
#include "Engine/Renderer/Interfaces/PipelineState.hpp"
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
	void Shutdown();
	void Clear();
	void ClearVertices() { m_debugVerts.clear();}
	void RenderWorld(Camera const& worldCamera);
	void RenderScreen(Camera const& screenCamera);
	void ClearExpiredShapes();
	void ClearModelMatrices() { m_modelMatrices.clear(); }
	void AddShape(DebugShape const& newShape);
	void ResetCmdLists();
	void EndFrame();

private:
	void AddVertsForShapes();
	void AddVertsForDebugShape(DebugShape const& shape);
	void CreatePSOs();
	void CreateRenderObjects();
	void CreateOrUpdateVertexBuffer();
	CommandList* GetCopyCmdList();
	Buffer*& GetVertexBuffer();

private:
	DebugRenderConfig m_config = {};
	Clock m_clock = {};
	bool m_hidden = false;
	unsigned int m_vertexCounts[(int)DebugRenderMode::NUM_DEBUG_RENDER_MODES] = {};
	CommandList** m_copyCommandLists = nullptr;
	RenderContext* m_renderContext = nullptr;
	PipelineState* m_debugPSOs[(int)DebugRenderMode::NUM_DEBUG_RENDER_MODES] = {};
	Buffer** m_modelCBO = nullptr;
	Buffer** m_vertexBuffers = nullptr;
	Buffer* m_intermediateBuffer = nullptr;
	Fence* m_copyFence = nullptr;
	std::vector<DebugShape> m_debugShapes[(int)DebugRenderMode::NUM_DEBUG_RENDER_MODES] = {};
	std::vector<Vertex_PCU> m_debugVerts = {};
	std::vector<Mat44> m_modelMatrices = {};
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
	s_debugRenderSystem->ClearModelMatrices();
	s_debugRenderSystem->EndFrame();
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
	shapeInfo.m_start			= start;
	shapeInfo.m_end				= end;
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
	shapeInfo.m_start			= base;
	shapeInfo.m_end				= top;
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
	shapeInfo.m_modelMatrix = modelMatrix;
	shapeInfo.m_shapeType = DEBUG_RENDER_WORLD_WIRE_SPHERE;
}

void DebugAddWorldArrow(const Vec3& start, const Vec3& end, float radius, float duration, const Rgba8& baseColor, const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
	unsigned int vertexCount = CalcVertCountForArrow3D();
	unsigned int vertexStart = s_debugRenderSystem->GetVertexCount(mode);

	DebugShapeInfo shapeInfo = {};
	shapeInfo.m_startVertex = vertexStart;
	shapeInfo.m_vertexCount = vertexCount;
	shapeInfo.m_flags = DebugShapeFlagsBit::DebugWorldShape | DebugShapeFlagsBit::DebugWorldShapeValid;
	shapeInfo.m_renderMode = mode;
	shapeInfo.m_duration = duration;
	shapeInfo.m_radius = radius;
	shapeInfo.m_start = start;
	shapeInfo.m_end = end;
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
	unsigned int vertexCount = CalcVertCountForAABB3D();
	unsigned int vertexStart = s_debugRenderSystem->GetVertexCount(mode);

	DebugShapeInfo shapeInfo = {};
	shapeInfo.m_startVertex = vertexStart;
	shapeInfo.m_vertexCount = vertexCount;
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
	shapeInfo.m_modelMatrix = modelMatrix;
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
	
}

DebugRenderSystem::~DebugRenderSystem()
{
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

	m_vertexBuffers = new Buffer*[m_config.m_renderer->GetBackBufferCount()];

	
}

void DebugRenderSystem::Shutdown()
{
	Renderer* renderer = m_config.m_renderer;
	for (unsigned int debugRenderTypeIndex = 0; debugRenderTypeIndex < (unsigned int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugRenderTypeIndex++) {
		PipelineState*& pso = m_debugPSOs[debugRenderTypeIndex];
		delete pso;
		pso	= nullptr;
	}

	for (unsigned int bufferIndex = 0; bufferIndex < renderer->GetBackBufferCount(); bufferIndex++) {
		delete m_vertexBuffers[bufferIndex];
		m_vertexBuffers[bufferIndex] = nullptr;

		delete m_copyCommandLists[bufferIndex];
		m_copyCommandLists[bufferIndex] = nullptr;

	}

	delete[] m_copyCommandLists;
	delete[] m_vertexBuffers;
	delete m_intermediateBuffer;
	m_intermediateBuffer = nullptr;

	delete m_renderContext;
	m_renderContext = nullptr;

	delete m_modelCBO;
	m_modelCBO = nullptr;
	delete[] m_vertexBuffers;
}

void DebugRenderSystem::Clear()
{
	for (int debugMode = 0; debugMode < (int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugMode++) {
		m_debugShapes[debugMode].clear();
	}
}

void DebugRenderSystem::RenderWorld(Camera const& worldCamera)
{
	// Construct all the shapes
	AddVertsForShapes();

	// Create vertex buffer
	CreateOrUpdateVertexBuffer();

	// Create model buffer list with initial Data upload

	// issue render calls, normal draw calls first

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

void DebugRenderSystem::ResetCmdLists()
{
	m_renderContext->Reset();
	CommandList* cmdList = GetCopyCmdList();
	cmdList->Reset();
}

void DebugRenderSystem::EndFrame()
{
	m_renderContext->EndFrame();
}

void DebugRenderSystem::AddVertsForShapes()
{
	// Go through all the shapes, and build add the verts to the vertex buffer
	for (unsigned int debugMode = 0; debugMode < (int)DebugRenderMode::NUM_DEBUG_RENDER_MODES; debugMode++) {
		std::vector<DebugShape>& shapesContainer = m_debugShapes[debugMode];
		for (unsigned int shapeIndex = 0; shapeIndex < shapesContainer.size(); shapeIndex++) {
			DebugShape& shape = shapesContainer[shapeIndex];
			if(!shape.IsShapeValid()) continue; // Shape is marked as deleted, so continue

			AddVertsForDebugShape(shape);
			m_modelMatrices.push_back(shape.GetModelMatrix());
		}
	}
}

void DebugRenderSystem::AddVertsForDebugShape(DebugShape const& shape)
{
	switch (shape.GetType())
	{
	case DEBUG_RENDER_NUM_TYPES:
	default:
	case INVALID:
		ERROR_AND_DIE("UNKNOWN DEBUG SHAPE");
		break;
	case DEBUG_RENDER_WORLD_WIRE_SPHERE:
	case DEBUG_RENDER_WORLD_POINT:
		AddVertsForSphere(m_debugVerts, shape.GetRadius(), shape.GetStacks(), shape.GetSlices(), shape.GetModelColor());
		break;
	case DEBUG_RENDER_WORLD_WIRE_CYLINDER:
	case DEBUG_RENDER_WORLD_LINE:
		AddVertsForCylinder(m_debugVerts, shape.GetStart(), shape.GetEnd(), shape.GetRadius(), shape.GetSlices(), shape.GetModelColor());
		break;
	case DEBUG_RENDER_WORLD_ARROW:
		AddVertsForArrow3D(m_debugVerts, shape.GetStart(), shape.GetEnd(), shape.GetRadius(), shape.GetSlices(), shape.GetModelColor());
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

		AddVertsForArrow3D(m_debugVerts, Vec3::ZERO, iBasis, shape.GetRadius(), shape.GetSlices(), shape.GetModelColor());
		AddVertsForArrow3D(m_debugVerts, Vec3::ZERO, jBasis, shape.GetRadius(), shape.GetSlices(), shape.GetModelColor());
		AddVertsForArrow3D(m_debugVerts, Vec3::ZERO, kBasis, shape.GetRadius(), shape.GetSlices(), shape.GetModelColor());
		break;
	}
	case DEBUG_RENDER_WORLD_TEXT:
		break;
	case DEBUG_RENDER_WORLD_BILLBOARD_TEXT:
		break;
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
	ShaderPipeline debugShaderPipeline = renderer->GetEngineShader(EngineShaderPipelines::Debug);

	// Debug Depth
	PipelineStateDesc psoDesc = {};
	psoDesc.m_blendModes[0] = {BlendMode::OPAQUE};
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
	psoDesc.m_byteCodes[ShaderType::Vertex] = debugShaderPipeline.m_firstShader;
	psoDesc.m_byteCodes[ShaderType::Pixel] = debugShaderPipeline.m_pixelShader;

	m_debugPSOs[(int)DebugRenderMode::USEDEPTH] = renderer->CreatePipelineState(psoDesc);

	// WIRE is same as depth, but wire mode
	psoDesc.m_fillMode = FillMode::WIREFRAME;
	m_debugPSOs[(int)DebugRenderMode::WIRE] = renderer->CreatePipelineState(psoDesc);

	// Always
	psoDesc.m_blendModes[0] = BlendMode::ALPHA;
	psoDesc.m_cullMode = CullMode::NONE;
	psoDesc.m_depthFunc = DepthFunc::ALWAYS;
	psoDesc.m_depthEnable = false;
	m_debugPSOs[(int)DebugRenderMode::ALWAYS] = renderer->CreatePipelineState(psoDesc);

	// XRAY Identical to always, but we do cull back faces
	psoDesc.m_cullMode = CullMode::BACK;
	m_debugPSOs[(int)DebugRenderMode::XRAY] = renderer->CreatePipelineState(psoDesc);

	// TEXT!!
}

void DebugRenderSystem::CreateRenderObjects()
{
	Renderer* renderer = m_config.m_renderer;
	unsigned int backbufferCount = renderer->GetBackBufferCount();

	unsigned int descriptorCounts[4] = { 17, 1, 1, 1 };
	unsigned int rscDescriptorCounts = descriptorCounts[(int)DescriptorHeapType::CBV_SRV_UAV];
	unsigned int equalDistribution = (rscDescriptorCounts - 2) / 5;

	RenderContextDesc renderCtxDesc = {};
	renderCtxDesc.m_cmdListDesc.m_type = CommandListType::DIRECT;
	renderCtxDesc.m_cmdListDesc.m_debugName = "DebugCmdList";
	renderCtxDesc.m_descriptorCounts = descriptorCounts;
	renderCtxDesc.m_renderer = m_config.m_renderer;
	renderCtxDesc.m_rscDescriptorDistribution[PARAM_CAMERA_BUFFERS] = 2;		// Just 2 needed for world and UI
	renderCtxDesc.m_rscDescriptorDistribution[PARAM_MODEL_BUFFERS] = equalDistribution;
	renderCtxDesc.m_rscDescriptorDistribution[PARAM_DRAW_INFO_BUFFERS] = equalDistribution;
	renderCtxDesc.m_rscDescriptorDistribution[PARAM_GAME_BUFFERS] = equalDistribution;
	renderCtxDesc.m_rscDescriptorDistribution[PARAM_TEXTURES] = equalDistribution;
	renderCtxDesc.m_rscDescriptorDistribution[PARAM_GAME_UAVS] = equalDistribution;

	m_renderContext = new RenderContext(renderCtxDesc);


	CommandListDesc copyCmdListDesc = {};
	std::string copyDebugName = "DbgCopyCmdList";;
	copyCmdListDesc.m_initialState = nullptr;
	copyCmdListDesc.m_type = CommandListType::COPY;
	m_copyCommandLists = new CommandList*[backbufferCount];

	for (unsigned int bufferIndex = 0; bufferIndex < backbufferCount; bufferIndex++) {
		copyDebugName += bufferIndex;
		m_copyCommandLists[bufferIndex] = renderer->CreateCommandList(copyCmdListDesc);
	}

	m_copyFence = renderer->CreateFence(CommandListType::COPY);
}

void DebugRenderSystem::CreateOrUpdateVertexBuffer()
{
	Renderer* renderer = m_config.m_renderer;
	BufferDesc bufferDesc = {};
	bufferDesc.m_type = BufferType::Vertex;
	bufferDesc.m_debugName = "DebugVtxBuff";
	bufferDesc.m_memoryUsage = MemoryUsage::Dynamic;
	bufferDesc.m_data = nullptr; // Manually handling data copying
	bufferDesc.m_size = m_debugVerts.size() * sizeof(Vertex_PCU);
	bufferDesc.m_stride.m_strideBytes = sizeof(Vertex_PCU);
	
	unsigned int currentBufferIndex = m_renderContext->GetBufferIndex();

	// Create buffer
	Buffer*& currentVtxBuffer = GetVertexBuffer();
	CommandList* copyCmdList = GetCopyCmdList();
	// Warning, this is borderline dereferencing a nullptr. It only works because of the first condition
	bool shouldCreateVtxBuffer = (currentVtxBuffer == nullptr) || (currentVtxBuffer->GetSize() < bufferDesc.m_size);

	if (shouldCreateVtxBuffer) {
		if(currentVtxBuffer) delete currentVtxBuffer;

		currentVtxBuffer = renderer->CreateBuffer(bufferDesc);
	}

	if (!m_intermediateBuffer) {
		BufferDesc intermediateDesc = bufferDesc;
		intermediateDesc.m_memoryUsage = MemoryUsage::Dynamic;
		intermediateDesc.m_debugName = "Intermediate Buffer";
		m_intermediateBuffer = renderer->CreateBuffer(intermediateDesc);
	}

	// update vertex buffer/upload verts
	TransitionBarrier copyBarriers[2] = {};
	copyBarriers[0] = TransitionBarrier(currentVtxBuffer, Common, CopyDest);
	copyBarriers[1] = TransitionBarrier(m_intermediateBuffer, Common, CopySrc);

	copyCmdList->ResourceBarrier(_countof(copyBarriers), copyBarriers);

	m_intermediateBuffer->CopyToBuffer(m_debugVerts.data(), bufferDesc.m_size);

	copyCmdList->CopyBuffer(currentVtxBuffer, m_intermediateBuffer);

	copyCmdList->Close();
	
	m_copyFence->SignalGPU();
	m_copyFence->Wait();
	renderer->InsertWaitInQueue(CommandListType::DIRECT, m_copyFence);

	renderer->ExecuteCmdLists(CommandListType::COPY, 1, &copyCmdList);

	
}

CommandList* DebugRenderSystem::GetCopyCmdList()
{
	unsigned int currentBackBuferIndex = m_config.m_renderer->GetCurrentBufferIndex();
	return m_copyCommandLists[currentBackBuferIndex];
}

Buffer*& DebugRenderSystem::GetVertexBuffer()
{
	unsigned int currentBackBuferIndex = m_config.m_renderer->GetCurrentBufferIndex();
	return m_vertexBuffers[currentBackBuferIndex];
}
