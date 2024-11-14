#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Interfaces/DescriptorHeap.hpp"
#include "Engine/Renderer/Interfaces/CommandList.hpp"
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

