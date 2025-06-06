#include "Engine/Renderer/Interfaces/Buffer.hpp"
#include "Engine/Renderer/Interfaces/Resource.hpp"

Buffer::~Buffer()
{

}

void Buffer::CopyToBuffer(void* data, size_t size)
{
	if (m_desc.m_memoryUsage != MemoryUsage::Dynamic) {
		ERROR_RECOVERABLE("BUFFER IS NOT MEANT TO BE COPIED TO FROM CPU");
		return;
	}

	void* pMap = nullptr;

	Map(pMap, 0, m_desc.m_size);
	memcpy(pMap, data, size);
	Unmap();
}



BufferView Buffer::GetBufferView() const
{
	BufferView retView = {};
	retView.m_bufferAddr = GetGPUAddress();
	retView.m_stride.m_strideBytes = m_desc.m_stride.m_strideBytes;
	retView.m_sizeBytes = m_desc.m_size;
	retView.m_elemCount = m_elemCount;
	return retView;
}

BufferView Buffer::GetIndexBufferView() const
{
	BufferView retView = {};
	retView.m_bufferAddr = GetGPUAddress();
	retView.m_stride.m_format = m_desc.m_stride.m_format;
	retView.m_sizeBytes = m_desc.m_size;
	return retView;
}

Buffer::Buffer(BufferDesc const& bufferDesc) :
	Resource(bufferDesc.m_debugName),
	m_desc(bufferDesc)
{
	m_elemCount = m_desc.m_size / m_desc.m_stride.m_strideBytes;
}
