#include "Engine/Renderer/Renderer.hpp"
#include <Engine/Renderer/D3D12/lib/d3d12.h>
#include <Engine/Renderer/D3D12/lib/d3dcommon.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <d3d12shader.h>
#include <dxgidebug.h>
#include "Game/EngineBuildPreferences.hpp"
#include "Engine/Renderer/Interfaces/DescriptorHeap.hpp"
#include "Engine/Renderer/Interfaces/CommandList.hpp"
#include "Engine/Renderer/Interfaces/Resource.hpp"
#include "Engine/Renderer/Interfaces/Buffer.hpp"
#include "Engine/Core/Image.hpp"

#include "Engine/Renderer/D3D12/D3D12TypeConversions.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"
#include "Engine/Window/Window.hpp"
#include <ThirdParty/ImGUI/imgui_impl_win32.h>
#include <ThirdParty/ImGUI/imgui_impl_dx12.h>
#include <Engine/Renderer/D3D12/lib/d3dx12_resource_helpers.h>


#pragma comment (lib, "Engine/Renderer/D3D12/lib/dxcompiler.lib")
#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "dxguid.lib")
#pragma message("ENGINE_DIR == " ENGINE_DIR)


void GetHardwareAdapter(
	IDXGIFactory1* pFactory,
	IDXGIAdapter1** ppAdapter,
	bool requestHighPerformanceAdapter)
{
	*ppAdapter = nullptr;

	ComPtr<IDXGIAdapter1> adapter;

	ComPtr<IDXGIFactory6> factory6;
	if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
	{
		for (
			UINT adapterIndex = 0;
			SUCCEEDED(factory6->EnumAdapterByGpuPreference(
				adapterIndex,
				requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
				IID_PPV_ARGS(&adapter)));
			++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	if (adapter.Get() == nullptr)
	{
		for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	*ppAdapter = adapter.Detach();
}

Renderer::Renderer(RendererConfig const& config)
{

}

Renderer& Renderer::Startup()
{
	EnableDebugLayer();
	CreateDevice();
	CreateCommandQueue();
	CreateSwapChain();

	CommandListDesc rscCmdDesc = {};
	rscCmdDesc.m_initialState = nullptr;
	rscCmdDesc.m_type = CommandListType::DIRECT;

	m_rscCmdList = CreateCommandList(rscCmdDesc);

	return *this;
}

Renderer& Renderer::Shutdown()
{
	ShutdownImGui();
	

	return *this;
}

Renderer& Renderer::BeginFrame()
{
	BeginFrameImGui();
	return *this;
}

void Renderer::CreateDevice()
{
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	// Enable additional debug layers.
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_DXGIFactory)), "FAILED TO CREATE DXGI FACTORY");


	ComPtr<IDXGIAdapter1> hardwareAdapter;
	GetHardwareAdapter(m_DXGIFactory, &hardwareAdapter, false);

	ThrowIfFailed(D3D12CreateDevice(
		hardwareAdapter.Get(),
		D3D_FEATURE_LEVEL_12_2,
		IID_PPV_ARGS(&m_device)
	), "FAILED TO CREATE DEVICE");

	SetDebugName(m_DXGIFactory, "DXGI FACTORY");
	SetDebugName(m_device, "DEVICE");

#if defined(_DEBUG)
	ID3D12InfoQueue* d3dInfoQueue;
	m_device->QueryInterface(&d3dInfoQueue);
	if (d3dInfoQueue)
	{
		D3D12_MESSAGE_ID hide[] =
		{
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE
		};
		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = _countof(hide);
		filter.DenyList.pIDList = hide;
		d3dInfoQueue->AddStorageFilterEntries(&filter);
	}
#endif
}

void Renderer::CreateCommandQueue()
{
	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)), "FAILED TO CREATE COMMANDQUEUE");
}

void Renderer::CreateSwapChain()
{
	Window* window = Window::GetWindowContext();
	IntVec2 windowDims = window->GetClientDimensions();

	m_viewport = D3D12_VIEWPORT{0.0f, 0.0f, float(windowDims.x), float(windowDims.y), 0.0f, 1.0f};
	m_scissorRect = D3D12_RECT{0, 0, LONG(windowDims.x), LONG(windowDims.y)};

	HWND windowHandle = (HWND)window->m_osWindowHandle;
	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Width = windowDims.x;
	swapChainDesc.Height = windowDims.y;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(m_DXGIFactory->CreateSwapChainForHwnd(
		m_commandQueue,        // Swap chain needs the queue so that it can force a flush on it.
		windowHandle,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	), "Failed to create swap chain");

	// This sample does not support full screen transitions.
	ThrowIfFailed(m_DXGIFactory->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER), "Failed to make window association");

	ThrowIfFailed(swapChain->QueryInterface(&m_swapChain), "Failed to get Swap Chain");
	m_currentBackBuffer = m_swapChain->GetCurrentBackBufferIndex();
}

void Renderer::SetDebugName(IDXGIObject* object, char const* name)
{
	if (!object) return;
#if defined(ENGINE_DEBUG_RENDER)
	object->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(name), name);
#else
	UNUSED(object);
	UNUSED(name);
#endif
}

void Renderer::SetDebugName(ID3D12Object* object, char const* name)
{
#if defined(ENGINE_DEBUG_RENDER)
	object->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(name), name);
#else
	UNUSED(object);
	UNUSED(name);
#endif
}

void Renderer::InitializeImGui()
{
#if defined(ENGINE_USE_IMGUI)

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	Window* window = Window::GetWindowContext();


	{
		DescriptorHeapDesc desc = {};
		desc.m_flags = DescriptorHeapFlags::ShaderVisible;
		desc.m_numDescriptors = 1;
		desc.m_type = DescriptorHeapType::CBV_SRV_UAV;
	
		m_ImGuiSrvDescHeap = CreateDescriptorHeap(desc, "ImGuiDescriptorHeap");
	}

	HWND windowHandle = (HWND)window->m_osWindowHandle;
	ImGui_ImplWin32_Init(windowHandle);
	ImGui_ImplDX12_Init(m_device, m_config.m_backBuffersCount,
		DXGI_FORMAT_R8G8B8A8_UNORM, m_ImGuiSrvDescHeap->m_descriptorHeap,
		m_ImGuiSrvDescHeap->GetCPUHandleHeapStart(),
		m_ImGuiSrvDescHeap->GetGPUHandleHeapStart());
#endif
}

void Renderer::ShutdownImGui()
{
#if defined(ENGINE_USE_IMGUI)

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	delete m_ImGuiSrvDescHeap;
	m_ImGuiSrvDescHeap = nullptr;
#endif
}

void Renderer::BeginFrameImGui()
{
#if defined(ENGINE_USE_IMGUI)
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//ImGui::ShowDemoWindow();
#endif
}

Texture* Renderer::GetTextureForFileName(char const* imageFilePath)
{
	Texture* textureToGet = nullptr;

	for (Texture*& loadedTexture : m_loadedTextures) {
		if (loadedTexture->GetSource() == imageFilePath) {
			return loadedTexture;
		}
	}
	return textureToGet;
}

Texture* Renderer::CreateTextureFromImage(Image const& image)
{
	TextureCreateInfo ci{};
	ci.m_owner = this;
	ci.m_name = image.GetImageFilePath();
	ci.m_source = image.GetImageFilePath().c_str();
	ci.m_dimensions = image.GetDimensions();
	ci.m_initialData = image.GetRawData();
	ci.m_stride = sizeof(Rgba8);

	Texture* newTexture = CreateTexture(ci);
	SetDebugName(newTexture->m_rsc->m_rawRsc, ci.m_source);

	return newTexture;
}

Texture* Renderer::CreateTextureFromFile(char const* imageFilePath)
{
	Image loadedImage(imageFilePath);
	Texture* newTexture = CreateTextureFromImage(loadedImage);

	return newTexture;
}

Renderer& Renderer::RenderImGui(CommandList& cmdList, Texture* renderTarget)
{
#if defined(ENGINE_USE_IMGUI)
	
	ResourceView* rtView = renderTarget->GetRenderTargetView();
	D3D12_CPU_DESCRIPTOR_HANDLE currentRTVHandle = rtView->m_descriptor;

	ImGui::Render();
	cmdList.SetRenderTargets(1, &renderTarget, true, nullptr);
	cmdList.SetDescriptorHeaps(1, &m_ImGuiSrvDescHeap);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList.m_cmdList);
#endif

	return *this;
}

void Renderer::EnableDebugLayer()
{
#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
		}
	}
#endif
}

Renderer& Renderer::EndFrame()
{
	
	return *this;
}

DescriptorHeap* Renderer::CreateDescriptorHeap(DescriptorHeapDesc const& desc, char const* debugName)
{
	DescriptorHeap* newHeap = new DescriptorHeap(desc);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = desc.m_numDescriptors;
	heapDesc.Type = LocalToD3D12(desc.m_type);
	heapDesc.Flags = (D3D12_DESCRIPTOR_HEAP_FLAGS)static_cast<uint8_t>(desc.m_flags);

	char const* errorString = Stringf("FAILED TO CRATE DESCRIPTOR HEAP %s", debugName).c_str();
	ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&newHeap->m_descriptorHeap)), errorString);
	newHeap->m_handleSize = m_device->GetDescriptorHandleIncrementSize(heapDesc.Type);

	if (debugName) {
		SetDebugName(newHeap->m_descriptorHeap, debugName);
	}

	return newHeap;

}

CommandList* Renderer::CreateCommandList(CommandListDesc const& desc)
{
	CommandList* newCmdList = new CommandList(desc);

	D3D12_COMMAND_LIST_TYPE cmdListType = LocalToD3D12(desc.m_type);


	m_device->CreateCommandAllocator(cmdListType, IID_PPV_ARGS(&newCmdList->m_cmdList));
	m_device->CreateCommandList1(0, cmdListType, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&newCmdList->m_cmdList));

	return newCmdList;
}

ResourceView* Renderer::CreateRenderTargetView(size_t handle, Texture* renderTarget)
{
	if (renderTarget->IsRenderTargetCompatible())
	{
		Resource* rtResource = renderTarget->m_rsc;
		D3D12_RENDER_TARGET_VIEW_DESC viewDesc = {};
		viewDesc.Format = LocalToColourD3D12(renderTarget->GetFormat());
		viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		ResourceView* newRTV = new ResourceView();
		newRTV->m_descriptor = { handle };
		newRTV->m_owner = rtResource;

		rtResource->m_rtv = newRTV;

		m_device->CreateRenderTargetView(rtResource->m_rawRsc, &viewDesc, newRTV->m_descriptor);
	}
	return renderTarget->GetRenderTargetView();
}

ResourceView* Renderer::CreateShaderResourceView(size_t handle, Texture* texture)
{
	if (texture->IsShaderResourceCompatible()) {

		Resource* texResource = texture->m_rsc;
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = LocalToD3D12(texture->GetFormat());
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

		ResourceView* newSRV = new ResourceView();
		newSRV->m_descriptor = { handle };
		newSRV->m_owner = texResource;

		texResource->m_srv = newSRV;

		m_device->CreateShaderResourceView(texResource->m_rawRsc, &srvDesc, newSRV->m_descriptor);
	}

	return texture->GetShaderResourceView();
}

ResourceView* Renderer::CreateShaderResourceView(size_t handle, Buffer* buffer)
{
	BufferView viewDesc = buffer->GetBufferView();
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	srvDesc.Buffer.NumElements = viewDesc.m_elemCount;
	srvDesc.Buffer.StructureByteStride = (UINT)viewDesc.m_stride.m_strideBytes;
	srvDesc.Format = LocalToD3D12(TextureFormat::UNKNOWN);
	srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 1, 2, 3);
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

	Resource* rsc = buffer->m_rsc;
	ResourceView* newSRV = new ResourceView();
	newSRV->m_descriptor = { handle };
	newSRV->m_owner = rsc;
	rsc->m_srv = newSRV;

	m_device->CreateShaderResourceView(rsc->m_rawRsc, &srvDesc, newSRV->m_descriptor);

	return buffer->GetShaderResourceView();
}

ResourceView* Renderer::CreateDepthStencilView(size_t handle, Texture* depthTexture)
{
	if (depthTexture->IsDepthStencilCompatible()) {
		Resource* texResource = depthTexture->m_rsc;
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = LocalToD3D12(depthTexture->GetFormat());
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

		ResourceView* newDSV = new ResourceView();
		newDSV->m_descriptor = { handle };
		newDSV->m_owner = texResource;

		texResource->m_dsv = newDSV;

		m_device->CreateDepthStencilView(texResource->m_rawRsc, &dsvDesc, newDSV->m_descriptor);
	}
	return depthTexture->GetDepthStencilView();
}

ResourceView* Renderer::CreateConstantBufferView(size_t handle, Buffer* cBuffer)
{
	BufferView viewDesc = cBuffer->GetBufferView();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cBufferView = D3D12_CONSTANT_BUFFER_VIEW_DESC();
	cBufferView.BufferLocation = cBuffer->GetGPUAddress();
	cBufferView.SizeInBytes = (UINT)viewDesc.m_sizeBytes;

	Resource* rsc = cBuffer->m_rsc;
	ResourceView* newCBV = new ResourceView();
	newCBV->m_descriptor = { handle };
	newCBV->m_owner = rsc;
	rsc->m_cbv = newCBV;

	m_device->CreateConstantBufferView(&cBufferView, newCBV->m_descriptor);

	return cBuffer->GetConstantBufferView();
}

Texture* Renderer::CreateOrGetTextureFromFile(char const* imageFilePath)
{
	Texture* existingTexture = GetTextureForFileName(imageFilePath);
	if (existingTexture)
	{
		return existingTexture;
	}

	// Never seen this texture before!  Let's load it.
	Texture* newTexture = CreateTextureFromFile(imageFilePath);
	return newTexture;
}

Texture* Renderer::CreateTexture(TextureCreateInfo& creationInfo)
{
	Resource*& handle = creationInfo.m_handle;
	ID3D12Resource2* textureUploadHeap = nullptr;

	if (handle) {
		handle->m_rawRsc->AddRef();
	}
	else {
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.Width = (UINT64)creationInfo.m_dimensions.x;
		textureDesc.Height = (UINT64)creationInfo.m_dimensions.y;
		textureDesc.MipLevels = 1;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.Format = LocalToD3D12(creationInfo.m_format);
		textureDesc.Flags = LocalToD3D12(creationInfo.m_bindFlags);
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		CD3DX12_HEAP_PROPERTIES heapType(D3D12_HEAP_TYPE_DEFAULT);
		D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON;

		handle = new Resource(creationInfo.m_name.c_str());
		handle->m_currentState = initialResourceState;

		D3D12_CLEAR_VALUE clearValueTex = {};
		D3D12_CLEAR_VALUE* clearValue = NULL;

		// If it can be bound as RT then it needs the clear colour, otherwise it's null
		if (creationInfo.m_bindFlags & RESOURCE_BIND_RENDER_TARGET_BIT) {
			creationInfo.m_clearColour.GetAsFloats(clearValueTex.Color);
			clearValueTex.Format = LocalToD3D12(creationInfo.m_clearFormat);
			clearValue = &clearValueTex;
		}
		else if (creationInfo.m_bindFlags & RESOURCE_BIND_DEPTH_STENCIL_BIT) {
			creationInfo.m_clearColour.GetAsFloats(clearValueTex.Color);
			clearValueTex.Format = LocalToD3D12(creationInfo.m_clearFormat);
			clearValue = &clearValueTex;
		}

		HRESULT textureCreateHR = m_device->CreateCommittedResource(
			&heapType,
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			initialResourceState,
			clearValue,
			IID_PPV_ARGS(&handle->m_rawRsc)
		);

		if (creationInfo.m_initialData) {
			UINT64  const uploadBufferSize = GetRequiredIntermediateSize(handle->m_rawRsc, 0, 1);
			CD3DX12_RESOURCE_DESC uploadHeapDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
			CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);

			HRESULT createUploadHeap = m_device->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&uploadHeapDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&textureUploadHeap));

			ThrowIfFailed(createUploadHeap, "FAILED TO CREATE TEXTURE UPLOAD HEAP");
			SetDebugName(textureUploadHeap, "Texture Upload Heap");

			D3D12_SUBRESOURCE_DATA imageData = {};
			imageData.pData = creationInfo.m_initialData;
			imageData.RowPitch = creationInfo.m_stride * creationInfo.m_dimensions.x;
			imageData.SlicePitch = creationInfo.m_stride * creationInfo.m_dimensions.y * creationInfo.m_dimensions.x;

			UpdateSubresources(m_rscCmdList->m_cmdList, handle->m_rawRsc, textureUploadHeap, 0, 0, 1, &imageData);
		}

		std::string const errorMsg = Stringf("COULD NOT CREATE TEXTURE WITH NAME %s", creationInfo.m_name.c_str());
		ThrowIfFailed(textureCreateHR, errorMsg.c_str());
		handle->m_currentState = D3D12_RESOURCE_STATE_COPY_DEST;
	}
	Texture* newTexture = new Texture(creationInfo);
	if (textureUploadHeap) {
		newTexture->m_uploadRsc = new Resource(Stringf("UploadRsc: %s", creationInfo.m_name).c_str());
		newTexture->m_uploadRsc->m_rawRsc = textureUploadHeap;
	}
	newTexture->m_rsc = handle;
	SetDebugName(newTexture->m_rsc->m_rawRsc, creationInfo.m_name.c_str());

	m_loadedTextures.push_back(newTexture);

	return newTexture;
}

