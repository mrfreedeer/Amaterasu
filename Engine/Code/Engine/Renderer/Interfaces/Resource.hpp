#pragma once
#include <vector>
#include "Engine/Renderer/GraphicsCommon.hpp"
#include <d3d12.h>

class Resource;

struct ResourceView {
	Resource* m_owner = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_descriptor = {};
};

class Resource {
	friend class Renderer;
	friend class CommandList;
public:
	ResourceView* GetDepthStencilView() const { return m_dsv; }
	ResourceView* GetShaderResourceView() const { return m_srv; }
	ResourceView* GetConstantBufferView() const { return m_cbv; }
	ResourceView* GetRenderTargetView() const { return m_rtv; }
	ResourceView* GetUnorderedAccessView() const { return m_uav; }
	size_t GetGPUAddress() const { return m_rawRsc->GetGPUVirtualAddress();}
	void Map(void*& mapSource, size_t beginRange, size_t endRange);
	void Unmap();

private:
	Resource(char const* debugName): m_debugName(debugName) {}
	~Resource();
private:
	ID3D12Resource2* m_rawRsc = nullptr;
	ResourceView* m_dsv = nullptr;
	ResourceView* m_uav = nullptr;
	ResourceView* m_srv = nullptr;
	ResourceView* m_rtv = nullptr;
	ResourceView* m_cbv = nullptr;
	char const* m_debugName = nullptr;
	int m_currentState = 0;
};