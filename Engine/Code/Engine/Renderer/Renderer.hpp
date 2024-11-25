#pragma once

struct DescriptorHeapDesc;
struct CommandListDesc;
struct ID3D12Device8;
struct IDXGIFactory4;
struct ID3D12Object;

class DescriptorHeap;
class CommandList;
class Texture;
class Buffer;

struct RendererConfig {

};

class Renderer {
public:
	// All public methods to return instance to Renderer for chaining purposes
	// Creation methods, which require the device, are all here 
	Renderer(RendererConfig const& config);

	/// <summary>
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
	DescriptorHeap* CreateDescriptorHeap(DescriptorHeapDesc const& desc);
	CommandList* CreateCommandList(CommandListDesc const& desc);
	ResourceView* CreateRenderTargetView(size_t handle, Texture* renderTarget);
	ResourceView* CreateShaderResourceView(size_t handle, Buffer* buffer);
	ResourceView* CreateShaderResourceView(size_t handle, Texture* texture);
	ResourceView* CreateDepthStencilView(size_t handle, Texture* depthTexture);
	ResourceView* CreateConstantBufferView(size_t handle, Buffer* cBuffer);


private:

	void EnableDebugLayer();
	void CreateDevice();
	void SetDebugName(ID3D12Object* object, char const* name);
	void SetDebugName(IDXGIObject* object, char const* name);
	// ImGui
	void InitializeImGui();
	void ShutdownImGui();
	void BeginFrameImGui();
	void EndFrameImGui();

private:
	RendererConfig m_config = {};
	ID3D12Device8* m_device = nullptr;
	IDXGIFactory4* m_DXGIFactory = nullptr;

};