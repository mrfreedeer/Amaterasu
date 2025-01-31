#include "Engine/Renderer/Interfaces/DescriptorHeap.hpp"
#include <d3d12.h>
#include <lib/d3dx12.h>


DescriptorHeap::DescriptorHeap(DescriptorHeapDesc const& desc):
	m_config(desc),
	m_remainingDescriptors(desc.m_numDescriptors)
{
	// The assumption is that Renderer will initialize the descriptor heap properly
	GUARANTEE_OR_DIE(m_descriptorHeap == nullptr, "Descriptor Heap is unitiliazed");
	m_currentDescriptor = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
}


DescriptorHeap::~DescriptorHeap()
{
	DX_SAFE_RELEASE(m_descriptorHeap);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetNextCPUHandle()
{
	if ((!m_descriptorHeap) || (m_remainingDescriptors <= 0)) ERROR_AND_DIE("RAN OUT OF DESCRIPTORS");

	D3D12_CPU_DESCRIPTOR_HANDLE returnHandle = {};
	returnHandle.ptr = m_currentDescriptor;
	m_currentDescriptor += m_handleSize;
	m_remainingDescriptors--;

	return returnHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUHandleAtOffset(size_t offset)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE handleToReturn(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), (UINT)offset, (UINT)m_handleSize);

	return handleToReturn;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUHandleAtOffset(size_t offset)
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE handleToReturn(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), (UINT)offset, (UINT)m_handleSize);

	return handleToReturn;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUHandleHeapStart()
{
	return m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUHandleHeapStart()
{
	return m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
}
