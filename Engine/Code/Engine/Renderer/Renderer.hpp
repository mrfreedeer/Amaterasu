#pragma once
#include <Engine/Renderer/D3D12/lib/d3d12.h>
#include "Engine/Renderer/Interfaces/DescriptorHeap.hpp"
#include "Engine/Renderer/Interfaces/CommandList.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"
#include <vector>

struct DescriptorHeapDesc;
struct CommandListDesc;
struct CommandQueueDesc;
struct ResourceView;
struct TextureDesc;
struct ShaderDesc;
struct BufferDesc;
struct Shader;
struct PipelineStateDesc;
struct Sampler;

struct IDXGIFactory4;
struct IDXGIObject;
struct IDXGISwapChain4;


class DescriptorHeap;
class CommandList;
class CommandQueue;
class Texture;
class Buffer;
class Camera;
class Image;
class Fence;
class Window;
class BitmapFont;

struct RendererConfig {
	unsigned int m_backBuffersCount = 0;
	Window* m_window = nullptr;
};

/// <summary>
/// Object that reports all live objects
/// NEEDS TO BE THE LAST DESTROYED THING, OTHERWISE IT REPORTS FALSE POSITIVES
/// </summary>
struct LiveObjectReporter {
	~LiveObjectReporter();
};
class Renderer {
public:
	// All public methods to return instance to Renderer for chaining purposes
	// Creation methods, which require the device, are all here 
	Renderer(RendererConfig const& config);
	~Renderer() {};
	/// <summary>+
	/// Handles initialization of all the rendering system, including creation and 
	/// initialization of all API's objects
	/// </summary>
	/// <returns></returns>
	Renderer& Startup();
	/// <summary>
	/// Destroy common API's objects, and shuts down the renderer. 
	/// Games are still responsible for destroying resources like descriptor heaps.
	/// Resources that implement singleton (i.e: textures) are destroyed by the renderer.
	/// </summary>
	/// <returns></returns>
	Renderer& Shutdown();
	Renderer& BeginFrame();
	Renderer& EndFrame();

	/// <summary>
	/// Creates descriptor heap according to provided description
	/// </summary>
	/// <param name="desc"></param>
	/// <returns></returns>
	DescriptorHeap* CreateDescriptorHeap(DescriptorHeapDesc& desc, char const* debugName = nullptr);
	DescriptorSet* CreateDescriptorSet(unsigned int* descriptorCounts, bool isShaderVisible, char const* debugName = nullptr);
	CommandList* CreateCommandList(CommandListDesc const& desc);
	CommandQueue* CreateCommandQueue(CommandQueueDesc const& desc);
	Fence* CreateFence(CommandListType managerType, unsigned int initialValue = 0);
	Renderer& CopyDescriptorHeap(unsigned int numDescriptors, DescriptorHeap* src, DescriptorHeap* dest, unsigned int offsetStart = 0, unsigned int offsetEnd = 0);
	Renderer& CopyDescriptor(size_t src, DescriptorHeap* dest, unsigned int offsetEnd = 0);
	Renderer& ExecuteCmdLists(CommandListType type, unsigned int count, CommandList** cmdLists);

	//-------------------------- Resource views --------------------------
	ResourceView* CreateRenderTargetView(size_t handle, Texture* renderTarget);
	ResourceView* CreateShaderResourceView(size_t handle, Buffer* buffer);
	ResourceView* CreateShaderResourceView(size_t handle, Texture* texture);
	ResourceView* CreateDepthStencilView(size_t handle, Texture* depthTexture);
	ResourceView* CreateConstantBufferView(size_t handle, Buffer* cBuffer);

	//----------------------------- Textures -----------------------------
	//************************************
	// Method:    CreateOrGetTextureFromFile
	// FullName:  Renderer::CreateOrGetTextureFromFile
	// Access:    public 
	// Returns:   Texture*
	// Qualifier:
	// Parameter: char const * imageFilePath
	// Parameter: CommandList * cmdList Optional parameter, cmd list used for copying texture to defaul memory
	//************************************
	Texture* CreateOrGetTextureFromFile(char const* imageFilePath, CommandList* cmdList = nullptr);
	Texture* CreateTexture(TextureDesc& creationInfo);
	Texture* GetActiveBackBuffer();
	Texture* GetBackUpBackBuffer();
	Texture* GetDefaultTexture();

	//----------------------------- Sampler -----------------------------
	Sampler* CreateSampler(size_t handle, SamplerMode samplerMode);

	//------------------------------ Buffers -----------------------------
	Buffer* CreateBuffer(BufferDesc const& desc);
	/// <summary>
	/// Utility function to create intermediate and default buffer. 
	/// There should be data, to copy into intermediate buffer
	/// </summary>
	/// <param name="desc"></param>
	/// <param name="out_intermediate"></param>
	/// <param name="out_default"></param>
	/// <returns></returns>
	Buffer* CreateDefaultBuffer(BufferDesc const& desc, Buffer** out_intermediate);
	Buffer* GetDefaultModelBuffer() const { return m_defaultModelBuffer; }

	//--------------------------- Shaders/PSO ----------------------------
	Shader* CreateOrGetShader(ShaderDesc const& desc);
	ShaderPipeline CreateOrGetShaderPipeline(ShaderPipelineDesc const& desc);
	PipelineState* CreatePipelineState(PipelineStateDesc const& desc);
	ShaderPipeline GetEngineShader(EngineShaderPipelines shader);

	BitmapFont* CreateOrGetBitmapFont(char const* sourcePath);
	Renderer& AddBackBufferToTextures();
	/// <summary>
	/// Set the state of the resources on the vector back to common. This must be handle by the game if at all
	/// </summary>
	/// <param name="resources"></param>
	/// <returns></returns>
	Renderer& HandleStateDecay(std::vector<Resource*> resources);

	/// <summary>
	/// Use this for waiting on a fence that is executing on another queue
	/// </summary>
	Renderer& InsertWaitInQueue(CommandListType waitingType, Fence* otherQFence);
	Renderer& RenderImGui(CommandList& cmdList, Texture* renderTarget);
	unsigned int GetCurrentBufferIndex() const { return m_currentBackBuffer; }
	unsigned int GetBackBufferCount() const { return m_config.m_backBuffersCount; }
	Renderer& UploadPendingResources();

	Renderer& Present(unsigned int syncInterval = 0, unsigned int flags = 0);

private:
	void EnableDebugLayer();
	void CreateDevice();
	void CreateSwapChain();
	/// <summary>
	/// Create default root signature, designed for bindless usage by default
	/// </summary>
	void CreateDefaultRootSignature();
	void SetDebugName(ID3D12Object* object, char const* name);
	void SetDebugName(IDXGIObject* object, char const* name);
	// ImGui
	void InitializeImGui();
	void ShutdownImGui();
	void BeginFrameImGui();

	// Textures
	Texture* GetTextureForFileName(char const* imageFilePath);
	Texture* CreateTextureFromImage(Image const& image, CommandList* cmdList = nullptr);
	Texture* CreateTextureFromFile(char const* imageFilePath, CommandList* cmdList = nullptr);
	void DestroyTexture(Texture* texture);

	// Shaders & PSO
	void LoadEngineShaders();
	Shader* CreateShader(ShaderDesc const& shaderDesc);
	PipelineState* CreateGraphicsPSO(PipelineStateDesc const& desc);
	PipelineState* CreateMeshPSO(PipelineStateDesc const& desc);
	PipelineState* CreateComputePSO(PipelineStateDesc const& desc);

	// CommandQueue
	CommandQueue* GetCommandQueue(CommandListType listType);

	BitmapFont* CreateBitmapFont(char const* sourcePath);
	void CompileShader(Shader* shader);

	size_t AlignToCBufferStride(size_t size) const;

private:
	// LiveObjectReporter must be first ALWAYS!!!!!
	LiveObjectReporter m_liveObjectReporter;
	RendererConfig m_config = {};
	ID3D12Device8* m_device = nullptr;
	IDXGIFactory4* m_DXGIFactory = nullptr;
	IDXGISwapChain4* m_swapChain = nullptr;
	ID3D12RootSignature* m_defaultRootSig = nullptr;
	DescriptorHeap* m_ImGuiSrvDescHeap = nullptr;
	CommandQueue* m_graphicsQueue = nullptr;
	CommandQueue* m_copyQueue = nullptr;
	Fence* m_internalFence = nullptr;

	unsigned int m_currentBackBuffer = 0;

	Texture* m_defaultTexture = nullptr;
	Texture** m_backBuffers = nullptr;

	// Game Resources
	std::vector<Texture*> m_loadedTextures;
	std::vector<BitmapFont*> m_loadedFonts;
	std::vector<Shader*> m_loadedShaders;

	// Internal Command Lists (For uploading singleton resources like textures)
	CommandList* m_rscCmdList = nullptr;
	ShaderPipeline m_engineShaders[(int)EngineShaderPipelines::NUM_ENGINE_SHADERS] = {};

	// Identity Matrix model buffer
	Buffer* m_defaultModelBuffer = nullptr;
};

struct RenderContextDesc {
	Renderer* m_renderer = nullptr;
	// All descriptor heaps will be considered shader visible
	unsigned int* m_descriptorCounts = nullptr;
	CommandListDesc m_cmdListDesc = {};
};
class RenderContext {
public:
	RenderContext(RenderContextDesc const& config);
	~RenderContext();

	RenderContext& BeginFrame() { };
	RenderContext& BeginCamera(Camera const& camera);
	RenderContext& EndCamera(Camera const& camera);
	RenderContext& EndFrame();
	RenderContext& Execute();
	CommandList* GetCommandList();
	DescriptorHeap* GetDescriptorHeap(DescriptorHeapType heapType) { return m_descriptorSet->GetDescriptorHeap(heapType); }
	DescriptorSet* GetDescriptorSet() { return m_descriptorSet; }
	unsigned int GetBufferIndex() const { return m_currentIndex; }

	RenderContext& Close();
	RenderContext& CloseAll();
	RenderContext& Reset();
private:
	RenderContextDesc m_config = {};
	D3D12_VIEWPORT m_viewport = {};
	D3D12_RECT m_scissorRect = {};
	CommandList** m_cmdList = nullptr;
	DescriptorSet* m_descriptorSet = nullptr;
	Camera const* m_currentCamera = nullptr;
	unsigned int m_currentIndex = 0;
	unsigned int m_backBufferCount = 0;
};