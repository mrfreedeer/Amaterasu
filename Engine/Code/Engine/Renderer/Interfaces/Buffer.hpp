#pragma once
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"

class Resource;

struct BufferDesc {
	void* m_data = nullptr;
	char const* m_debugName = nullptr;
	size_t m_size = 0;
	size_t m_stride = 0;
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
};

class Buffer {

public:
	void Map(void*& mapSource);
	void Unmap();
	size_t GetSize() const { return m_desc.m_size; }
	MemoryUsage GetMemoryUsage() const { return m_desc.m_memoryUsage; }
	BufferView GetBufferView() const;
private:
	Buffer(BufferDesc const& bufferDesc, Resource* bufferRsc);

private:
	Resource* m_rsc = nullptr;
	BufferDesc m_desc = {};
};