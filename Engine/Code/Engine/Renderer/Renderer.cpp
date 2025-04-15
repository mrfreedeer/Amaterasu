#include "Engine/Renderer/Renderer.hpp"
#include <Engine/Renderer/D3D12/lib/d3dcommon.h>
#include <dxgidebug.h>
#include <dxgi1_6.h>
#include <d3d12shader.h>
#include "Game/EngineBuildPreferences.hpp"
#include "Engine/Renderer/Interfaces/Resource.hpp"
#include "Engine/Renderer/Interfaces/Buffer.hpp"
#include "Engine/Renderer/Interfaces/CommandQueue.hpp"
#include "Engine/Renderer/Interfaces/Fence.hpp"
#include "Engine/Renderer/Interfaces/PipelineState.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/D3D12/D3D12TypeConversions.hpp"

#include "Engine/Core/Image.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"
#include "Engine/Renderer/ConstantBuffers.hpp"
#include "Engine/Window/Window.hpp"
#include <ThirdParty/ImGUI/imgui_impl_win32.h>
#include <ThirdParty/ImGUI/imgui_impl_dx12.h>
#include <Engine/Renderer/D3D12/lib/d3dx12_resource_helpers.h>
#include <Engine/Core/FileUtils.hpp>
#include <Engine/Renderer/D3D12/lib/dxcapi.h>
#include <Engine/Renderer/D3D12/lib/d3dx12_root_signature.h>


#pragma comment (lib, "Engine/Renderer/D3D12/lib/dxcompiler.lib")
#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "dxguid.lib")
#pragma message("ENGINE_DIR == " ENGINE_DIR)

ModelConstants defaultModelConstants = {};

void SetBlendModeSpecs(BlendMode const* blendModes, D3D12_BLEND_DESC& blendDesc) {
	/*
		WARNING!!!!!
		IF THE RT IS SINGLE CHANNEL I.E: FLOATR_32,
		PICKING ALPHA/ADDITIVE WILL GET THE SRC ALPHA (DOES NOT EXIST)
		MAKES ALL WRITES TO RENDER TARGET INVALID!!!!
	*/
	blendDesc.IndependentBlendEnable = TRUE;
	for (int rtIndex = 0; rtIndex < 8; rtIndex++) {
		BlendMode currentRtBlendMode = blendModes[rtIndex];
		D3D12_RENDER_TARGET_BLEND_DESC& rtBlendDesc = blendDesc.RenderTarget[rtIndex];
		rtBlendDesc.BlendEnable = true;
		rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
		rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
		rtBlendDesc.DestBlendAlpha = D3D12_BLEND_ONE;
		rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;

		switch (currentRtBlendMode) {
		case BlendMode::ALPHA:
			rtBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			rtBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			break;
		case BlendMode::ADDITIVE:
			rtBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			rtBlendDesc.DestBlend = D3D12_BLEND_ONE;
			break;
		case BlendMode::OPAQUE:
			rtBlendDesc.SrcBlend = D3D12_BLEND_ONE;
			rtBlendDesc.DestBlend = D3D12_BLEND_ZERO;
			break;
		default:
			ERROR_AND_DIE(Stringf("Unknown / unsupported blend mode #%i", currentRtBlendMode));
			break;
		}
	}
}


DXGI_FORMAT GetFormatForComponent(D3D_REGISTER_COMPONENT_TYPE componentType, char const* semanticnName, BYTE mask) {
	// determine DXGI format
	if (mask == 1)
	{
		if (componentType == D3D_REGISTER_COMPONENT_UINT32) return DXGI_FORMAT_R32_UINT;
		else if (componentType == D3D_REGISTER_COMPONENT_SINT32) return DXGI_FORMAT_R32_SINT;
		else if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) return DXGI_FORMAT_R32_FLOAT;
	}
	else if (mask <= 3)
	{
		if (componentType == D3D_REGISTER_COMPONENT_UINT32) return DXGI_FORMAT_R32G32_UINT;
		else if (componentType == D3D_REGISTER_COMPONENT_SINT32) return DXGI_FORMAT_R32G32_SINT;
		else if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) return DXGI_FORMAT_R32G32_FLOAT;
	}
	else if (mask <= 7)
	{
		if (componentType == D3D_REGISTER_COMPONENT_UINT32) return DXGI_FORMAT_R32G32B32_UINT;
		else if (componentType == D3D_REGISTER_COMPONENT_SINT32) return DXGI_FORMAT_R32G32B32_SINT;
		else if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) return DXGI_FORMAT_R32G32B32_FLOAT;
	}
	else if (mask <= 15)
	{
		if (AreStringsEqualCaseInsensitive(semanticnName, "COLOR")) {
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		else {
			if (componentType == D3D_REGISTER_COMPONENT_UINT32) return DXGI_FORMAT_R32G32B32A32_UINT;
			else if (componentType == D3D_REGISTER_COMPONENT_SINT32) return DXGI_FORMAT_R32G32B32A32_SINT;
			else if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) return DXGI_FORMAT_R32G32B32A32_FLOAT;
		}
	}

	return DXGI_FORMAT_UNKNOWN;
}


void CreateInputLayoutFromVS(Shader* vs, std::vector<D3D12_SIGNATURE_PARAMETER_DESC>& elementsDescs, std::vector<std::string>& semanticNames)
{
	IDxcUtils* pUtils;
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));

	DxcBuffer reflectionSource;
	reflectionSource.Encoding = DXC_CP_ACP;
	reflectionSource.Ptr = vs->m_reflection.data();
	reflectionSource.Size = vs->m_reflection.size();

	ID3D12ShaderReflection* pShaderReflection = NULL;
	HRESULT reflectOp = pUtils->CreateReflection(&reflectionSource, IID_PPV_ARGS(&pShaderReflection));
	ThrowIfFailed(reflectOp, "FAILED TO GET REFLECTION OUT OF SHADER");

	//// Get shader info
	D3D12_SHADER_DESC shaderDesc;
	pShaderReflection->GetDesc(&shaderDesc);


	// Read input layout description from shader info
	for (UINT i = 0; i < shaderDesc.InputParameters; i++)
	{
		D3D12_SIGNATURE_PARAMETER_DESC paramDesc;
		pShaderReflection->GetInputParameterDesc(i, &paramDesc);
		semanticNames.push_back(std::string(paramDesc.SemanticName));
		//save element desc
		//
		elementsDescs.emplace_back(D3D12_SIGNATURE_PARAMETER_DESC{
		paramDesc.SemanticName,			// Name of the semantic
		paramDesc.SemanticIndex,		// Index of the semantic
		paramDesc.Register,				// Number of member variables
		paramDesc.SystemValueType,		// A predefined system value, or D3D_NAME_UNDEFINED if not applicable
		paramDesc.ComponentType,		// Scalar type (e.g. uint, float, etc.)
		paramDesc.Mask,					// Mask to indicate which components of the register
										// are used (combination of D3D10_COMPONENT_MASK values)
		paramDesc.ReadWriteMask,		// Mask to indicate whether a given component is 
										// never written (if this is an output signature) or
										// always read (if this is an input signature).
										// (combination of D3D_MASK_* values)
		paramDesc.Stream,			// Stream index
		paramDesc.MinPrecision			// Minimum desired interpolation precision
			});
	}
	DX_SAFE_RELEASE(pShaderReflection);
	DX_SAFE_RELEASE(pUtils);
}

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

Renderer::Renderer(RendererConfig const& config) : m_config(config)
{

}

Renderer& Renderer::Startup()
{
	CommandQueueDesc queueDesc = {};
	queueDesc.m_flags = QueueFlags::None;
	queueDesc.m_listType = CommandListType::DIRECT;
	queueDesc.m_debugName = "GraphicsQ";

	EnableDebugLayer();
	CreateDevice();
	m_graphicsQueue = CreateCommandQueue(queueDesc);

	queueDesc.m_listType = CommandListType::COPY;
	queueDesc.m_debugName = "CopyQ";
	m_copyQueue = CreateCommandQueue(queueDesc);

	CreateSwapChain();
	CreateDefaultRootSignature();

	CommandListDesc rscCmdDesc = {};
	rscCmdDesc.m_initialState = nullptr;
	rscCmdDesc.m_type = CommandListType::DIRECT;
	rscCmdDesc.m_debugName = "RscCmdList";

	m_rscCmdList = CreateCommandList(rscCmdDesc);

	AddBackBufferToTextures();

	// Create default texture to enable sampling in any shader
	m_defaultTexture = new Texture();
	Image whiteTexelImg(IntVec2(1, 1), Rgba8::WHITE);
	whiteTexelImg.m_imageFilePath = "DefaultTexture";
	m_defaultTexture = CreateTextureFromImage(whiteTexelImg);

	LoadEngineShaders();

	BufferDesc modelBufDesc = {};
	modelBufDesc.m_debugName = "DefaultModelBufferUpload";
	modelBufDesc.m_data = &defaultModelConstants;
	modelBufDesc.m_memoryUsage = MemoryUsage::Dynamic;
	modelBufDesc.m_size = sizeof(ModelConstants);
	modelBufDesc.m_stride.m_strideBytes = sizeof(ModelConstants);

	m_defaultModelBufferUpload = CreateBuffer(modelBufDesc);
	modelBufDesc.m_memoryUsage = MemoryUsage::Default;
	modelBufDesc.m_data = nullptr;

	m_defaultModelBuffer = CreateBuffer(modelBufDesc);

	m_rscCmdList->CopyBuffer(m_defaultModelBufferUpload, m_defaultModelBuffer);
	InitializeImGui();

	Fence* initializationFence = CreateFence(CommandListType::DIRECT);

	m_rscCmdList->Close();
	ExecuteCmdLists(CommandListType::DIRECT, 1, &m_rscCmdList);
	initializationFence->Signal();
	initializationFence->Wait(initializationFence->GetFenceValue());

	delete initializationFence;
	m_rscCmdList->Reset();


	return *this;
}

Renderer& Renderer::Shutdown()
{
	ShutdownImGui();

	delete m_rscCmdList;
	m_rscCmdList = nullptr;

	DX_SAFE_RELEASE(m_DXGIFactory);
	DX_SAFE_RELEASE(m_swapChain);
	DX_SAFE_RELEASE(m_defaultRootSig);

	delete m_ImGuiSrvDescHeap;
	m_ImGuiSrvDescHeap = nullptr;

	delete m_graphicsQueue;
	m_graphicsQueue = nullptr;

	DX_SAFE_RELEASE(m_device);

	m_currentBackBuffer = 0;

	// Game Resources
	for (int textureInd = 0; textureInd < m_loadedTextures.size(); textureInd++) {
		Texture* tex = m_loadedTextures[textureInd];
		DestroyTexture(tex);
	}
	m_loadedTextures.clear();

	for (int shaderInd = 0; shaderInd < m_loadedShaders.size(); shaderInd++) {
		Shader* shader = m_loadedShaders[shaderInd];
		delete shader;
	}
	m_loadedShaders.clear();

	for (int fontInd = 0; fontInd < m_loadedFonts.size(); fontInd++) {
		BitmapFont* font = m_loadedFonts[fontInd];
		delete font;
	}
	m_loadedFonts.clear();

	return *this;
}

Renderer& Renderer::BeginFrame()
{
	if (m_defaultModelBufferUpload) delete m_defaultModelBufferUpload;
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

void Renderer::CreateSwapChain()
{
	Window* window = Window::GetWindowContext();
	IntVec2 windowDims = window->GetClientDimensions();

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
		m_graphicsQueue->m_queue,        // Swap chain needs the queue so that it can force a flush on it.
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

void Renderer::CreateDefaultRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE1 descRange[6];
	descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, (UINT)-1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);		// Unbounded model buffers
	descRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, (UINT)-1, 0, 1, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);		// Unbounded camera buffers
	descRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 128, 0, 2);																// Game CBuffers
	descRange[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)-1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);		// Unbounded textures
	descRange[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 128, 0, 0);																// Game UAVs
	descRange[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 8, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);															// There might be max a sampler per RT (I've only used 1 ever)

	CD3DX12_ROOT_PARAMETER1 rootParams[6];
	rootParams[0].InitAsDescriptorTable(1, &descRange[0]);
	rootParams[1].InitAsDescriptorTable(1, &descRange[1]);
	rootParams[2].InitAsDescriptorTable(1, &descRange[2]);
	rootParams[3].InitAsDescriptorTable(1, &descRange[3]);
	rootParams[4].InitAsDescriptorTable(1, &descRange[4]);
	rootParams[5].InitAsDescriptorTable(1, &descRange[5]);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignDesc(6, rootParams);
	rootSignDesc.Desc_1_2.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	ID3DBlob* pSerializedRootSig = nullptr;
	ID3DBlob* pErrorBlob = nullptr;
	HRESULT serializeRes = D3D12SerializeVersionedRootSignature(&rootSignDesc, &pSerializedRootSig, &pErrorBlob);

	if (pErrorBlob) {
		DebuggerPrintf((char const*)pErrorBlob->GetBufferPointer());
	}

	ThrowIfFailed(serializeRes, "FAILED TO SERIALIZE DEFAULT ROOT SIGNATURE");
	HRESULT rootSigCreateRes = m_device->CreateRootSignature(0, pSerializedRootSig->GetBufferPointer(), pSerializedRootSig->GetBufferSize(), IID_PPV_ARGS(&m_defaultRootSig));
	SetDebugName(m_defaultRootSig, "DefaultRootSig");

	ThrowIfFailed(rootSigCreateRes, "FAILED TO CREATE DEFAULT ROOT SIGNATURE");
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
	if (!object) return;
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
	TextureDesc ci{};
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

void Renderer::DestroyTexture(Texture* texture)
{
	if (texture) {
		Resource* texResource = texture->m_rsc;
		Resource* uploadRsc = texture->m_uploadRsc;
		delete texture;
		if (texResource) {
			delete texResource;
		}

		if (uploadRsc) {
			delete uploadRsc;
		}
	}
}

void Renderer::LoadEngineShaders()
{
	ShaderPipelineDesc legacyDefaultDesc = {};
	legacyDefaultDesc.m_name = "LegacyDefault";
	legacyDefaultDesc.m_path = ENGINE_DIR "Renderer/Shaders/DefaultFwdLegacy.hlsl";
	legacyDefaultDesc.m_firstShaderType = ShaderType::Vertex;

	m_engineShaders[(int)EngineShaderPipelines::LegacyForward] = CreateOrGetShaderPipeline(legacyDefaultDesc);
}

Shader* Renderer::CreateShader(ShaderDesc const& shaderDesc)
{
	Shader* newShader = new Shader();

	newShader->m_name = shaderDesc.m_name;
	newShader->m_path = shaderDesc.m_path;
	newShader->m_type = shaderDesc.m_type;

	CompileShader(newShader);

	m_loadedShaders.push_back(newShader);
	return newShader;
}

PipelineState* Renderer::CreateGraphicsPSO(PipelineStateDesc const& desc)
{
	std::vector<D3D12_SIGNATURE_PARAMETER_DESC> reflectInputDesc;
	std::vector<std::string> nameStrings;
	Shader* vs = desc.m_byteCodes[ShaderType::Vertex];

	CreateInputLayoutFromVS(vs, reflectInputDesc, nameStrings);

	// Const Char* copying from D3D12_SIGNATURE_PARAMETER_DESC is quite problematic
	// so the string is kept alive
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutDesc;
	inputLayoutDesc.resize(reflectInputDesc.size());

	for (int inputIndex = 0; inputIndex < reflectInputDesc.size(); inputIndex++) {
		D3D12_INPUT_ELEMENT_DESC& elementDesc = inputLayoutDesc[inputIndex];
		D3D12_SIGNATURE_PARAMETER_DESC& paramDesc = reflectInputDesc[inputIndex];
		std::string& currentString = nameStrings[inputIndex];

		paramDesc.SemanticName = currentString.c_str();
		elementDesc.Format = GetFormatForComponent(paramDesc.ComponentType, currentString.c_str(), paramDesc.Mask);
		elementDesc.SemanticName = currentString.c_str();
		elementDesc.SemanticIndex = paramDesc.SemanticIndex;
		elementDesc.InputSlot = 0;
		elementDesc.AlignedByteOffset = (inputIndex == 0) ? 0 : D3D12_APPEND_ALIGNED_ELEMENT;
		elementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;
	}

	D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(
		LocalToD3D12(desc.m_fillMode),			// Fill mode
		LocalToD3D12(desc.m_cullMode),			// Cull mode
		LocalToD3D12(desc.m_windingOrder),		// Winding order
		D3D12_DEFAULT_DEPTH_BIAS,					// Depth bias
		D3D12_DEFAULT_DEPTH_BIAS_CLAMP,				// Bias clamp
		D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,		// Slope scaled bias
		TRUE,										// Depth Clip enable
		FALSE,										// Multi sample (MSAA)
		FALSE,										// Anti aliased line enable
		0,											// Force sample count
		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF	// Conservative Rasterization
	);

	D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	SetBlendModeSpecs(desc.m_blendModes, blendDesc);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	Shader* ps = desc.m_byteCodes[ShaderType::Pixel];
	Shader* gs = desc.m_byteCodes[ShaderType::Geometry];
	Shader* hs = desc.m_byteCodes[ShaderType::Hull];
	Shader* ds = desc.m_byteCodes[ShaderType::Domain];

	psoDesc.InputLayout.NumElements = (UINT)reflectInputDesc.size();
	psoDesc.InputLayout.pInputElementDescs = inputLayoutDesc.data();

	ID3D12RootSignature* pRootSignature = nullptr;
	if (vs->m_rootSignature.size() > 0) {
		m_device->CreateRootSignature(0, vs->m_rootSignature.data(), vs->m_rootSignature.size(), IID_PPV_ARGS(&pRootSignature));
	}
	else { // Use default if no specific root sig is used in shader
		pRootSignature = m_defaultRootSig;
	}

	psoDesc.pRootSignature = pRootSignature;
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(ps->m_byteCode.data(), ps->m_byteCode.size());

	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vs->m_byteCode.data(), vs->m_byteCode.size());
	if (gs) {
		psoDesc.GS = CD3DX12_SHADER_BYTECODE(gs->m_byteCode.data(), gs->m_byteCode.size());
	}

	if (hs) {
		psoDesc.HS = CD3DX12_SHADER_BYTECODE(hs->m_byteCode.data(), hs->m_byteCode.size());
	}

	if (ds) {
		psoDesc.DS = CD3DX12_SHADER_BYTECODE(ds->m_byteCode.data(), ds->m_byteCode.size());
	}

	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.BlendState = blendDesc;
	psoDesc.DepthStencilState.DepthEnable = desc.m_depthEnable;
	psoDesc.DepthStencilState.DepthFunc = LocalToD3D12(desc.m_depthFunc);
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	psoDesc.DSVFormat = LocalToD3D12(desc.m_depthStencilFormat);
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = // a stencil operation structure, does not really matter since stencil testing is turned off
	{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	psoDesc.DepthStencilState.FrontFace = defaultStencilOp; // both front and back facing polygons get the same treatment
	psoDesc.DepthStencilState.BackFace = defaultStencilOp;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = (UINT)desc.m_sampleCount;
	psoDesc.PrimitiveTopologyType = LocalToD3D12(desc.m_topology);
	psoDesc.NumRenderTargets = (UINT)desc.m_renderTargetCount;
	for (unsigned int rtIndex = 0; rtIndex < desc.m_renderTargetCount; rtIndex++) {
		psoDesc.RTVFormats[rtIndex] = LocalToColourD3D12(desc.m_renderTargetFormats[rtIndex]);
	}

	ID3D12PipelineState* pso = nullptr;
	m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
	PipelineState* newPso = new PipelineState(pso);
	newPso->m_desc = desc;
	newPso->m_rootSignature = pRootSignature;

	return newPso;
}

PipelineState* Renderer::CreateMeshPSO(PipelineStateDesc const& desc)
{
	UNUSED(desc);
	return nullptr;
}

PipelineState* Renderer::CreateComputePSO(PipelineStateDesc const& desc)
{
	UNUSED(desc);
	return nullptr;
}



CommandQueue* Renderer::GetCommandQueue(CommandListType listType)
{
	CommandQueue* queueToReturn = nullptr;

	switch (listType)
	{
	case CommandListType::DIRECT:
		queueToReturn = m_graphicsQueue;
		break;
	case CommandListType::COPY:
		queueToReturn = m_copyQueue;
		break;
	case CommandListType::BUNDLE:
	case CommandListType::COMPUTE:
	case CommandListType::VIDEO_DECODE:
	case CommandListType::VIDEO_PROCESS:
	case CommandListType::VIDEO_ENCODE:
	case CommandListType::NUM_COMMAND_LIST_TYPES:
	default:
		"UNKNOWN QUEUE TYPE";
		break;
	}

	return queueToReturn;
}

BitmapFont* Renderer::CreateBitmapFont(char const* sourcePath)
{
	std::string filename = sourcePath;
	filename += ".png";

	Texture* bitmapTexture = CreateOrGetTextureFromFile(filename.c_str());
	BitmapFont* newBitmapFont = new BitmapFont(filename.c_str(), *bitmapTexture);

	m_loadedFonts.push_back(newBitmapFont);
	return newBitmapFont;
}

void Renderer::CompileShader(Shader* shader)
{
	IDxcUtils* pUtils = nullptr;
	IDxcCompiler3* pCompiler = nullptr;
	IDxcIncludeHandler* pIncludeHandler = nullptr;

	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler));

	pUtils->CreateDefaultIncludeHandler(&pIncludeHandler);

	char const* entryPoint = (shader->m_entryPoint) ? shader->m_entryPoint : Shader::GetDefaultEntryPoint(shader->m_type);

	size_t shaderNameLength = strlen(shader->m_name) + 1;
	size_t entryPointLength = strlen(entryPoint) + 1;
	size_t srcLength = strlen(shader->m_path) + 1;
	wchar_t* wShaderName = new wchar_t[shaderNameLength];
	wchar_t* wEntryPoint = new wchar_t[entryPointLength];
	wchar_t* wSrc = new wchar_t[srcLength];
	wchar_t const* wTarget = Shader::GetTarget(shader->m_type);

	size_t outNameLength = 0;
	size_t outEntryPointLength = 0;
	size_t outSrcLength = 0;

	mbstowcs_s(&outNameLength, wShaderName, shaderNameLength, shader->m_name, shaderNameLength - 1);
	mbstowcs_s(&outEntryPointLength, wEntryPoint, entryPointLength, entryPoint, entryPointLength - 1);
	mbstowcs_s(&outSrcLength, wSrc, srcLength, shader->m_path, srcLength - 1);

	LPCWSTR compileArgs[] =
	{
		L"-E", wEntryPoint,              // Entry point.
		L"-T", wTarget,            // Target.
		#if defined(_DEBUG)
		DXC_ARG_DEBUG,
		DXC_ARG_SKIP_OPTIMIZATIONS,
		L"-Qembed_debug"
		#endif
	};

	IDxcBlobEncoding* pSource = nullptr;
	pUtils->LoadFile(wSrc, nullptr, &pSource);
	DxcBuffer source;
	source.Ptr = pSource->GetBufferPointer();
	source.Size = pSource->GetBufferSize();
	source.Encoding = DXC_CP_ACP;

	// Compile
	IDxcResult* pResults = nullptr;
	pCompiler->Compile(&source, compileArgs, _countof(compileArgs), pIncludeHandler, IID_PPV_ARGS(&pResults));

	// Get errors
	// If UTF16 is used, pErrors is always null with current settings
	IDxcBlobUtf8* pErrors = nullptr;
	pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), NULL);
	if (pErrors != nullptr && pErrors->GetStringLength() != 0) {

		DebuggerPrintf("\n-------------- SHADER COMPILATION ERRORS --------------\n");
		DebuggerPrintf(pErrors->GetStringPointer());
		DebuggerPrintf("-------------------------------------------------------\n");

		ERROR_AND_DIE("COMPILATION FAILED! SEE OUTPUT FOR MORE DETAILS");
	}

	// If compilation failed, then die
	HRESULT hrStatus;
	pResults->GetStatus(&hrStatus);
	ThrowIfFailed(hrStatus, "COMPILATION FAILED");


	IDxcBlob* pShader = nullptr;
	pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), nullptr);

	shader->m_byteCode.resize(pShader->GetBufferSize());
	memcpy(shader->m_byteCode.data(), pShader->GetBufferPointer(), pShader->GetBufferSize());

	IDxcBlob* pReflection = nullptr;
	pResults->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&pReflection), nullptr);

	shader->m_reflection.resize(pReflection->GetBufferSize());
	memcpy(shader->m_reflection.data(), pReflection->GetBufferPointer(), pReflection->GetBufferSize());

	IDxcBlob* pRootSignature = nullptr;
	pResults->GetOutput(DXC_OUT_ROOT_SIGNATURE, IID_PPV_ARGS(&pRootSignature), nullptr);

	if (pRootSignature) {
		shader->m_rootSignature.resize(pRootSignature->GetBufferSize());
		memcpy(shader->m_rootSignature.data(), pRootSignature->GetBufferPointer(), pRootSignature->GetBufferSize());
	}

	// Release all temp assets
	DX_SAFE_RELEASE(pUtils);
	DX_SAFE_RELEASE(pCompiler);
	DX_SAFE_RELEASE(pIncludeHandler);
	DX_SAFE_RELEASE(pResults);
	DX_SAFE_RELEASE(pShader);
	DX_SAFE_RELEASE(pReflection);
	DX_SAFE_RELEASE(pRootSignature);

	delete[] wShaderName;
	wShaderName = nullptr;

	delete[] wEntryPoint;
	wEntryPoint = nullptr;

	delete[] wSrc;
	wSrc = nullptr;
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

DescriptorHeap* Renderer::CreateDescriptorHeap(DescriptorHeapDesc& desc, char const* debugName)
{

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = desc.m_numDescriptors;
	heapDesc.Type = LocalToD3D12(desc.m_type);
	heapDesc.Flags = (D3D12_DESCRIPTOR_HEAP_FLAGS)static_cast<uint8_t>(desc.m_flags);

	ID3D12DescriptorHeap* descriptorHeap = nullptr;
	char const* errorString = Stringf("FAILED TO CRATE DESCRIPTOR HEAP %s", debugName).c_str();
	ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap)), errorString);

	desc.m_heap = descriptorHeap;
	DescriptorHeap* newHeap = new DescriptorHeap(desc);
	newHeap->m_handleSize = m_device->GetDescriptorHandleIncrementSize(heapDesc.Type);

	if (debugName) {
		SetDebugName(newHeap->m_descriptorHeap, debugName);
	}

	return newHeap;

}

DescriptorSet* Renderer::CreateDescriptorSet(unsigned int* descriptorCounts, bool isShaderVisible, char const* debugName /*= nullptr*/)
{
	DescriptorHeapFlags flag = (isShaderVisible) ? DescriptorHeapFlags::ShaderVisible : DescriptorHeapFlags::None;
	DescriptorSet* newSet = new DescriptorSet();
	for (unsigned int heapType = 0; heapType < (unsigned int)DescriptorHeapType::NUM_TYPES; heapType++) {
		DescriptorHeapDesc desc = {};
		DescriptorHeapType currentType = (DescriptorHeapType)heapType;
		std::string debugNameStr = debugName;
		debugNameStr += "| ";
		debugNameStr += EnumToString(currentType);

		desc.m_debugName = debugNameStr.c_str();
		desc.m_flags = (heapType < (int)DescriptorHeapType::RenderTargetView) ? flag : DescriptorHeapFlags::None;
		desc.m_numDescriptors = descriptorCounts[heapType];
		desc.m_type = currentType;

		newSet->m_descriptorHeaps[heapType] = CreateDescriptorHeap(desc);
	}

	return newSet;
}

CommandList* Renderer::CreateCommandList(CommandListDesc const& desc)
{
	CommandList* newCmdList = new CommandList(desc);

	D3D12_COMMAND_LIST_TYPE cmdListType = LocalToD3D12(desc.m_type);


	m_device->CreateCommandAllocator(cmdListType, IID_PPV_ARGS(&newCmdList->m_cmdAllocator));
	m_device->CreateCommandList1(0, cmdListType, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&newCmdList->m_cmdList));

	SetDebugName(newCmdList->m_cmdList, desc.m_debugName);

	newCmdList->Reset();

	return newCmdList;
}

CommandQueue* Renderer::CreateCommandQueue(CommandQueueDesc const& desc)
{
	CommandQueue* newCmdQueue = new CommandQueue(desc);

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = LocalToD3D12(desc.m_flags);
	cmdQueueDesc.Type = LocalToD3D12(desc.m_listType);

	m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&newCmdQueue->m_queue));

	SetDebugName(newCmdQueue->m_queue, desc.m_debugName);
	return newCmdQueue;
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
		newRTV->m_valid = true;

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
		newSRV->m_valid = true;

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
	srvDesc.Buffer.NumElements = (UINT)viewDesc.m_elemCount;
	srvDesc.Buffer.StructureByteStride = (UINT)viewDesc.m_stride.m_strideBytes;
	srvDesc.Format = LocalToD3D12(TextureFormat::UNKNOWN);
	srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 1, 2, 3);
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

	Resource* rsc = buffer->m_rsc;
	ResourceView* newSRV = new ResourceView();
	newSRV->m_descriptor = { handle };
	newSRV->m_owner = rsc;
	newSRV->m_valid = true;

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
		newDSV->m_valid = true;


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
	newCBV->m_valid = true;

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

Texture* Renderer::CreateTexture(TextureDesc& creationInfo)
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
		newTexture->m_uploadRsc = new Resource(Stringf("UploadRsc: %s", creationInfo.m_name.c_str()).c_str());
		newTexture->m_uploadRsc->m_rawRsc = textureUploadHeap;
	}
	newTexture->m_rsc = handle;
	SetDebugName(newTexture->m_rsc->m_rawRsc, creationInfo.m_name.c_str());
	m_loadedTextures.push_back(newTexture);

	return newTexture;
}

Texture* Renderer::GetActiveBackBuffer()
{
	return m_backBuffers[m_currentBackBuffer];
}

Texture* Renderer::GetBackUpBackBuffer()
{
	unsigned int otherInd = (m_currentBackBuffer + 1) % m_config.m_backBuffersCount;
	return m_backBuffers[otherInd];
}

Texture* Renderer::GetDefaultTexture()
{
	return m_defaultTexture;
}

Sampler* Renderer::CreateSampler(size_t handle, SamplerMode samplerMode)
{
	Sampler* newSampler = new Sampler();

	D3D12_SAMPLER_DESC samplerDesc = {};
	switch (samplerMode)
	{
	case SamplerMode::POINTCLAMP:
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

		break;
	case SamplerMode::POINTWRAP:
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

		break;
	case SamplerMode::BILINEARCLAMP:
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

		break;
	case SamplerMode::BILINEARWRAP:
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		break;

	case SamplerMode::SHADOWMAPS:
		samplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		samplerDesc.BorderColor[0] = 0.0f;
		samplerDesc.BorderColor[1] = 0.0f;
		samplerDesc.BorderColor[2] = 0.0f;
		samplerDesc.BorderColor[3] = 0.0f;

		break;
	default:
		break;
	}
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

	D3D12_CPU_DESCRIPTOR_HANDLE d3d12Handle = { handle };

	m_device->CreateSampler(&samplerDesc, d3d12Handle);

	return newSampler;
}

Buffer* Renderer::CreateBuffer(BufferDesc const& desc)
{
	Resource* bufferRsc = new Resource(desc.m_debugName);

	D3D12_HEAP_PROPERTIES heapType = CD3DX12_HEAP_PROPERTIES(LocalToD3D12(desc.m_memoryUsage));
	D3D12_RESOURCE_DESC1 rscDesc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, desc.m_size, 1, 1, 1,
		DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };

	D3D12_RESOURCE_STATES initialState = (desc.m_memoryUsage == MemoryUsage::Dynamic) ? D3D12_RESOURCE_STATE_COPY_SOURCE : D3D12_RESOURCE_STATE_COMMON;
	m_device->CreateCommittedResource2(&heapType,
		D3D12_HEAP_FLAG_NONE,
		&rscDesc,
		initialState,
		NULL,
		NULL,
		IID_PPV_ARGS(&bufferRsc->m_rawRsc));


	Buffer* newBuffer = new Buffer(desc, bufferRsc);

	// If there is initial data and is upload buffer, then copy to it
	if (desc.m_data && desc.m_memoryUsage == MemoryUsage::Dynamic) {
		newBuffer->CopyToBuffer(desc.m_data, desc.m_size);
	}

	return newBuffer;
}

Shader* Renderer::CreateOrGetShader(ShaderDesc const& desc)
{
	for (int shaderInd = 0; shaderInd < m_loadedShaders.size(); shaderInd++) {
		Shader* shader = m_loadedShaders[shaderInd];
		if ((strcmp(desc.m_path, shader->m_path) == 0) && (desc.m_type == shader->m_type)) {
			return shader;
		}
	}

	return CreateShader(desc);
}

ShaderPipeline Renderer::CreateOrGetShaderPipeline(ShaderPipelineDesc const& desc)
{
	GUARANTEE_OR_DIE(desc.m_firstShaderType != ShaderType::InvalidShader, "FIRST SHADER TYPE IS INVALID");

	ShaderDesc firstShaderDsc = {};
	std::string firstShaderName = desc.m_name;
	std::string secShaderName = desc.m_name;
	firstShaderName += "| " + std::string(EnumToString(desc.m_firstShaderType));
	secShaderName += "| PS";

	firstShaderDsc.m_path = desc.m_path;
	firstShaderDsc.m_type = desc.m_firstShaderType;
	firstShaderDsc.m_entryPoint = desc.m_firstEntryPoint;
	firstShaderDsc.m_name = firstShaderName.c_str();

	ShaderDesc secShaderDesc = {};
	secShaderDesc.m_path = desc.m_path;
	secShaderDesc.m_type = ShaderType::Pixel;
	secShaderDesc.m_name = secShaderName.c_str();
	secShaderDesc.m_entryPoint = desc.m_secEntryPoint;


	ShaderPipeline newPipeline = {};
	newPipeline.m_firstShader = CreateOrGetShader(firstShaderDsc);
	newPipeline.m_pixelShader = CreateOrGetShader(secShaderDesc);

	return newPipeline;
}

PipelineState* Renderer::CreatePipelineState(PipelineStateDesc const& desc)
{
	switch (desc.m_type)
	{
	case PipelineType::Graphics:	return CreateGraphicsPSO(desc);
	case PipelineType::Mesh:		return CreateMeshPSO(desc);
	case PipelineType::Compute:		return CreateComputePSO(desc);
	default:						return nullptr;
	}
}

ShaderPipeline Renderer::GetEngineShader(EngineShaderPipelines shader)
{
	return m_engineShaders[(int)shader];
}

BitmapFont* Renderer::CreateOrGetBitmapFont(char const* sourcePath)
{
	for (int loadedFontIndex = 0; loadedFontIndex < m_loadedFonts.size(); loadedFontIndex++) {
		BitmapFont*& bitmapFont = m_loadedFonts[loadedFontIndex];
		if (strcmp(bitmapFont->m_fontFilePathNameWithNoExtension.c_str(), sourcePath) == 0) {
			return bitmapFont;
		}
	}

	return CreateBitmapFont(sourcePath);
}

Fence* Renderer::CreateFence(CommandListType managerType, unsigned int initialValue /* = 0*/)
{
	CommandQueue* fenceManager = GetCommandQueue(managerType);

	Fence* outFence = new Fence(fenceManager, initialValue);

	ThrowIfFailed(m_device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&outFence->m_fence)), "FAILED CREATING FENCE");

	// Create an event handle to use for frame synchronization.
	outFence->m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (outFence->m_fenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()), "FAILED GETTING LAST ERROR");
	}

	outFence->m_commandQueue = fenceManager;

	return outFence;
}

Renderer& Renderer::CopyDescriptorHeap(unsigned int numDescriptors, DescriptorHeap* src, DescriptorHeap* dest, unsigned int offsetStart, unsigned int offsetEnd)
{
	D3D12_DESCRIPTOR_HEAP_TYPE heapType = LocalToD3D12(src->GetType());
	m_device->CopyDescriptorsSimple(numDescriptors, src->GetCPUHandleAtOffset(offsetStart), dest->GetCPUHandleAtOffset(offsetEnd), heapType);
	return *this;
}

Renderer& Renderer::ExecuteCmdLists(CommandListType type, unsigned int count, CommandList** cmdLists)
{
	CommandQueue* usedQueue = GetCommandQueue(type);

	usedQueue->ExecuteCommandLists(count, cmdLists);

	return *this;
}

Renderer& Renderer::AddBackBufferToTextures()
{
	m_backBuffers = new Texture * [m_config.m_backBuffersCount];

	// Handle size is vendor specific
	// Get a helper handle to the descriptor and create the RTVs 

	for (unsigned int frameBufferInd = 0; frameBufferInd < m_config.m_backBuffersCount; frameBufferInd++) {
		ID3D12Resource2* bufferTex = nullptr;
		Texture*& backBuffer = m_backBuffers[frameBufferInd];
		HRESULT fetchBuffer = m_swapChain->GetBuffer(frameBufferInd, IID_PPV_ARGS(&bufferTex));
		if (!SUCCEEDED(fetchBuffer)) {
			ERROR_AND_DIE("COULD NOT GET FRAME BUFFER");
		}

		D3D12_RESOURCE_DESC bufferTexDesc = bufferTex->GetDesc();

		TextureDesc backBufferTexInfo = {};
		backBufferTexInfo.m_bindFlags = ResourceBindFlagBit::RESOURCE_BIND_RENDER_TARGET_BIT;
		backBufferTexInfo.m_dimensions = IntVec2((int)bufferTexDesc.Width, (int)bufferTexDesc.Height);
		backBufferTexInfo.m_format = TextureFormat::R8G8B8A8_UNORM;
		backBufferTexInfo.m_name = Stringf("BackBuffer %d", frameBufferInd);
		backBufferTexInfo.m_owner = this;
		backBufferTexInfo.m_handle = new Resource(backBufferTexInfo.m_name.c_str());
		backBufferTexInfo.m_handle->m_rawRsc = bufferTex;

		backBuffer = CreateTexture(backBufferTexInfo);
		DX_SAFE_RELEASE(bufferTex);
	}

	return *this;
}


Renderer& Renderer::WaitForOtherQueue(CommandListType waitingType, Fence* otherQFence)
{
	CommandQueue* waitingQueue = nullptr;
	switch (waitingType)
	{
	case CommandListType::DIRECT:
		waitingQueue = m_graphicsQueue;
		break;
	case CommandListType::COPY:
		waitingQueue = m_copyQueue;
		break;
	case CommandListType::COMPUTE:
	case CommandListType::VIDEO_DECODE:
	case CommandListType::VIDEO_PROCESS:
	case CommandListType::VIDEO_ENCODE:
	case CommandListType::NUM_COMMAND_LIST_TYPES:
	case CommandListType::BUNDLE:
	default:
		ERROR_AND_DIE("UNRECOGNIZED QUEUE TYPE");
		break;
	}

	waitingQueue->Wait(otherQFence, otherQFence->GetFenceValue());

	return *this;
}

LiveObjectReporter::~LiveObjectReporter()
{
#if defined(_DEBUG) 
	ID3D12Debug3* debugController = nullptr;

	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {

		IDXGIDebug1* debug;
		DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug));
		debugController->Release();
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
		debug->Release();
	}
	else {
		ERROR_AND_DIE("COULD NOT ENABLE DX12 LIVE REPORTING");
	}

#endif
}

RenderContext::RenderContext(RenderContextDesc const& config) :
	m_config(config)
{
	Renderer* renderer = m_config.m_renderer;
	unsigned int backBufferCount = renderer->GetBackBufferCount();
	m_cmdList = new CommandList * [backBufferCount];

	for (unsigned int listInd = 0; listInd < backBufferCount; listInd++) {
		m_cmdList[listInd] = renderer->CreateCommandList(m_config.m_cmdListDesc);
	}

	m_descriptorSet = renderer->CreateDescriptorSet(config.m_descriptorCounts, true, "CTX");
}

RenderContext::~RenderContext()
{
	Renderer* renderer = m_config.m_renderer;
	unsigned int backBufferCount = renderer->GetBackBufferCount();
	for (unsigned int listInd = 0; listInd < backBufferCount; listInd++) {
		delete m_cmdList[listInd];
	}

	delete[] m_cmdList;
	m_cmdList = nullptr;

	delete m_descriptorSet;
	m_descriptorSet = nullptr;
}

RenderContext& RenderContext::BeginCamera(Camera const& camera)
{
	DescriptorHeap* rtvHeap = m_descriptorSet->GetDescriptorHeap(DescriptorHeapType::RenderTargetView);
	DescriptorHeap* dsvHeap = m_descriptorSet->GetDescriptorHeap(DescriptorHeapType::DepthStencilView);
	Renderer* renderer = m_config.m_renderer;

	m_currentCamera = &camera;
	// There are max 8 color targets bound at a time
	int rtCount = 0;
	for (int colorTargetInd = 0; colorTargetInd < 8; colorTargetInd++) {
		Texture* tex = camera.m_colorTargets[colorTargetInd];
		if (!tex) continue;
		if (!tex->GetRenderTargetView()) {
			renderer->CreateRenderTargetView(rtvHeap->GetNextCPUHandle().ptr, tex);
		}
		rtCount++;
	}

	if (camera.m_depthTarget) {
		if (!camera.m_depthTarget->GetDepthStencilView()->m_valid) {
			renderer->CreateDepthStencilView(dsvHeap->GetNextCPUHandle().ptr, camera.m_depthTarget);
		}
	}

	CommandList* cmdList = GetCommandList();
	cmdList->SetRenderTargets(rtCount, camera.m_colorTargets, true, camera.m_depthTarget);

	return *this;
}

RenderContext& RenderContext::EndCamera(Camera const& camera)
{
	GUARANTEE_OR_DIE(m_currentCamera == &camera, "THE CAMERA WAS SWITCHED MID RENDER PASS")
		return *this;
}

RenderContext& RenderContext::Execute()
{
	CommandList* cmdList = GetCommandList();
	m_config.m_renderer->ExecuteCmdLists(CommandListType::DIRECT, 1, &cmdList);

	return *this;
}

