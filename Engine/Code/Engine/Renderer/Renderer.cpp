#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Interfaces/DescriptorHeap.hpp"
#include "Engine/Renderer/Interfaces/CommandList.hpp"
#include "Engine/Renderer/Interfaces/Resource.hpp"
#include "Engine/Renderer/Interfaces/Buffer.hpp"
#include "Engine/Renderer/D3D12/lib/d3d12.h"
#include "Engine/Renderer/D3D12/D3D12TypeConversions.hpp"

Renderer::Renderer(RendererConfig const& config)
{

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

