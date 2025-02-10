#pragma once
#include "Engine/Renderer/GraphicsCommon.hpp"

struct ID3D12DescriptorHeap;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;

class Renderer;

enum class DescriptorHeapFlags : uint8_t {
	None = 0,
	ShaderVisible
};

enum class DescriptorHeapType : uint8_t {
	CBV_SRV_UAV = 0,
	Sampler,
	RenderTargetView,
	DepthStencilView,
	NUM_TYPES,
	UNDEFINED = 255,
};


struct DescriptorHeapDesc {
	unsigned int m_numDescriptors = 0;
	DescriptorHeapType m_type = DescriptorHeapType::UNDEFINED;
	DescriptorHeapFlags m_flags = DescriptorHeapFlags::None;
	char const* m_debugName = nullptr;
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
private:
	DescriptorHeap(DescriptorHeapDesc const& desc);

private:
	DescriptorHeapDesc m_config = {};
	ID3D12DescriptorHeap* m_descriptorHeap = nullptr;
	size_t m_remainingDescriptors = 0;
	size_t m_currentDescriptor = 0;
	size_t m_handleSize = 0;
};
