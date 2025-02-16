#pragma once
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/Interfaces/Resource.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"


struct BufferDesc {
	void* m_data = nullptr;
	char const* m_debugName = nullptr;
	size_t m_size = 0;
	union StrideDesc
	{
		TextureFormat m_format = TextureFormat::INVALID;
		size_t m_strideBytes;
	} m_stride;
	MemoryUsage m_memoryUsage = MemoryUsage::Default;
};

struct BufferView {
	size_t m_bufferAddr = 0;
	union StrideDesc
	{
		TextureFormat m_format = TextureFormat::INVALID;
		size_t m_strideBytes;
	} m_stride;
	size_t m_sizeBytes = 0;
	size_t m_elemCount = 0;
};

class Buffer {
	friend class Renderer;
	friend class CommandList;
public:
	~Buffer();
	void CopyToBuffer(void* data, size_t size);
	void Map(void*& mapSource, size_t beginRange = 0, size_t endRange = 0);
	void Unmap();
	/// <summary>
	/// Get size in bytes
	/// </summary>
	size_t GetSize() const { return m_desc.m_size; }
	size_t GetElementCount() const { return m_elemCount; }
	MemoryUsage GetMemoryUsage() const { return m_desc.m_memoryUsage; }
	BufferView GetBufferView() const;
	BufferView GetIndexBufferView() const;
	ResourceView* GetShaderResourceView() const { return m_rsc->GetShaderResourceView(); }
	ResourceView* GetConstantBufferView() const { return m_rsc->GetConstantBufferView(); }
	size_t GetGPUAddress() const { return m_rsc->GetGPUAddress(); }
private:
	Buffer(BufferDesc const& bufferDesc, Resource* bufferRsc);

private:
	Resource* m_rsc = nullptr;
	BufferDesc m_desc = {};
	size_t m_elemCount = 0;
};