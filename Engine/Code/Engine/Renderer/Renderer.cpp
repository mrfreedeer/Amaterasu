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

#include "Engine/Renderer/D3D12/D3D12TypeConversions.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"


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

DescriptorHeap* Renderer::CreateDescriptorHeap(DescriptorHeapDesc const& desc)
{
	DescriptorHeap* newHeap = new DescriptorHeap(desc);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = desc.m_numDescriptors;
	heapDesc.Type = LocalToD3D12(desc.m_type);
	heapDesc.Flags = (D3D12_DESCRIPTOR_HEAP_FLAGS)static_cast<uint8_t>(desc.m_flags);

	ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&newHeap->m_descriptorHeap)), "FAILED TO CREATE DESCRIPTOR HEAP");
	newHeap->m_handleSize = m_device->GetDescriptorHandleIncrementSize(heapDesc.Type);

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

