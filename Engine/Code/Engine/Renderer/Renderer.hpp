#pragma once
#include <Engine/Renderer/D3D12/lib/d3d12.h>

struct DescriptorHeapDesc;
struct CommandListDesc;
struct CommandQueueDesc;
struct ResourceView;
struct TextureCreateInfo;

struct IDXGIFactory4;
struct IDXGIObject;
struct IDXGISwapChain4;


class DescriptorHeap;
class CommandList;
class CommandQueue;
class Texture;
class Buffer;
class Image;
class Fence;
class BitmapFont;

struct RendererConfig {
	unsigned int m_backBuffersCount = 0;
};

class Renderer {
public:
	// All public methods to return instance to Renderer for chaining purposes
	// Creation methods, which require the device, are all here 
	Renderer(RendererConfig const& config);

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
	DescriptorHeap* CreateDescriptorHeap(DescriptorHeapDesc const& desc, char const* debugName = nullptr);
	CommandList* CreateCommandList(CommandListDesc const& desc);
	CommandQueue* CreateCommandQueue(CommandQueueDesc const& desc);
	ResourceView* CreateRenderTargetView(size_t handle, Texture* renderTarget);
	ResourceView* CreateShaderResourceView(size_t handle, Buffer* buffer);
	ResourceView* CreateShaderResourceView(size_t handle, Texture* texture);
	ResourceView* CreateDepthStencilView(size_t handle, Texture* depthTexture);
	ResourceView* CreateConstantBufferView(size_t handle, Buffer* cBuffer);
	Texture* CreateOrGetTextureFromFile(char const* imageFilePath);
	Texture* CreateTexture(TextureCreateInfo& creationInfo);
	Fence* CreateFence(CommandQueue* fenceManager, unsigned int initialValue = 0);

	/// <summary>
	/// When textures are created, they still need to be uploaded to the GPU
	/// The exact moment this happens, is left for the user to decide
	/// </summary>
	/// <returns></returns>
	Renderer& UploadTexturesToGPU();

	Renderer& RenderImGui(CommandList& cmdList, Texture* renderTarget);
private:

	void EnableDebugLayer();
	void CreateDevice();
	void CreateSwapChain();
	void SetDebugName(ID3D12Object* object, char const* name);
	void SetDebugName(IDXGIObject* object, char const* name);
	// ImGui
	void InitializeImGui();
	void ShutdownImGui();
	void BeginFrameImGui();

	// Textures
	Texture* GetTextureForFileName(char const* imageFilePath);
	Texture* CreateTextureFromImage(Image const& image);
	Texture* CreateTextureFromFile(char const* imageFilePath);

private:
	RendererConfig m_config = {};
	ID3D12Device8* m_device = nullptr;
	IDXGIFactory4* m_DXGIFactory = nullptr;
	D3D12_VIEWPORT m_viewport = {};
	D3D12_RECT m_scissorRect = {};
	IDXGISwapChain4* m_swapChain = nullptr;
	DescriptorHeap* m_ImGuiSrvDescHeap = nullptr;
	CommandQueue* m_commandQueue = nullptr;

	unsigned int m_currentBackBuffer = 0;

	// Game Resources
	std::vector<Texture*> m_loadedTextures;
	std::vector<BitmapFont*> m_loadedFonts;

	// Internal Command Lists (For uploading singleton resources like textures)
	CommandList* m_rscCmdList = nullptr;
};