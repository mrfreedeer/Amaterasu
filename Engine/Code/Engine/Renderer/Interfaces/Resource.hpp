#pragma once
#include <vector>
#include "Engine/Renderer/GraphicsCommon.hpp"
#include <d3d12.h>

class Resource;

struct ResourceView {
	Resource* m_owner = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_descriptor = {};
	// Flag to know whether this is a valid descriptor
	bool m_valid = false;
};

struct TransitionBarrier {
	ResourceStates m_before = ResourceStates::Common;
	ResourceStates m_after = ResourceStates::Common;
	Resource* m_rsc = nullptr;
};

class Resource {
	friend class Renderer;
	friend class CommandList;
public:
	virtual ~Resource();
	virtual ResourceView* GetDepthStencilView() const { return m_dsv; }
	virtual ResourceView* GetShaderResourceView() const { return m_srv; }
	virtual ResourceView* GetConstantBufferView() const { return m_cbv; }
	virtual ResourceView* GetRenderTargetView() const { return m_rtv; }
	virtual ResourceView* GetUnorderedAccessView() const { return m_uav; }
	virtual size_t GetGPUAddress() const { return m_rawRsc->GetGPUVirtualAddress(); }
	virtual void Map(void*& mapSource, size_t beginRange, size_t endRange);
	virtual void Unmap();

	virtual TransitionBarrier GetTransitionBarrier(ResourceStates afterState);

protected:
	Resource(char const* debugName) : m_debugName(debugName) {}
protected:
	ID3D12Resource2* m_rawRsc = nullptr;
	ResourceView* m_dsv = nullptr;
	ResourceView* m_uav = nullptr;
	ResourceView* m_srv = nullptr;
	ResourceView* m_rtv = nullptr;
	ResourceView* m_cbv = nullptr;
	char const* m_debugName = nullptr;
	int m_currentState = 0;
};