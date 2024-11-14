#pragma once

struct DescriptorHeapDesc;
struct CommandListDesc;


class DescriptorHeap;
class CommandList;

struct RendererConfig {

};

class Renderer {
public:
	// All public methods to return instance to Renderer for chaining purposes
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
private:
	// ImGui
	void InitializeImGui();
	void ShutdownImGui();
	void BeginFrameImGui();
	void EndFrameImGui();

private:
	RendererConfig m_config = {};
	ID3D12Device11* m_device = nullptr;

};