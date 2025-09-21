#pragma once
#include "Engine/Renderer/GraphicsCommon.hpp"

struct ID3D12DescriptorHeap;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;

class Renderer;


struct DescriptorHeapDesc {
	unsigned int m_numDescriptors = 0;
	DescriptorHeapType m_type = DescriptorHeapType::UNDEFINED;
	DescriptorHeapFlags m_flags = DescriptorHeapFlags::None;
	ID3D12DescriptorHeap* m_heap = nullptr;
	std::string m_debugName = "Unknown Heap";
};

class DescriptorHeap {
	// Renderer owns device, so only it can create DescriptorHeaps
	friend class Renderer;
	friend class CommandList;
public:
	~DescriptorHeap();
	D3D12_CPU_DESCRIPTOR_HANDLE GetNextCPUHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandleAtOffset(size_t offset);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleAtOffset(size_t offset);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandleHeapStart();
	size_t GetDescriptorCount() const;
	DescriptorHeapType GetType() const { return m_config.m_type; }
private:
	DescriptorHeap(DescriptorHeapDesc const& desc);

private:
	DescriptorHeapDesc m_config = {};
	ID3D12DescriptorHeap* m_descriptorHeap = nullptr;
	size_t m_remainingDescriptors = 0;
	size_t m_currentDescriptor = 0;
	size_t m_handleSize = 0;
};

// Container for a set of descriptors
struct DescriptorSet {
	~DescriptorSet();
	DescriptorHeap* m_descriptorHeaps[(int)DescriptorHeapType::NUM_TYPES] = {};
	DescriptorHeap* GetDescriptorHeap(DescriptorHeapType heapType) { return m_descriptorHeaps[(int)heapType]; }
};