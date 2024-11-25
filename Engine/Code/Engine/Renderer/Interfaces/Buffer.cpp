#include "Engine/Renderer/Interfaces/Buffer.hpp"
#include "Engine/Renderer/Interfaces/Resource.hpp"

void Buffer::Map(void*& mapSource)
{
	m_rsc->Map(mapSource, 0, m_desc.m_size);
}

void Buffer::Unmap()
{
	m_rsc->Unmap();
}

BufferView Buffer::GetBufferView() const
{
	BufferView retView = {};
	retView.m_bufferAddr = m_rsc->GetGPUAddress();
	retView.m_stride.m_strideBytes = m_desc.m_stride.m_strideBytes;
	retView.m_sizeBytes = m_desc.m_size;
	retView.m_elemCount = m_elemCount;
	return retView;
}

BufferView Buffer::GetIndexBufferView() const
{
	BufferView retView = {};
	retView.m_bufferAddr = m_rsc->GetGPUAddress();
	retView.m_stride.m_format = m_desc.m_stride.m_format;
	retView.m_sizeBytes = m_desc.m_size;
	return retView;
}

Buffer::Buffer(BufferDesc const& bufferDesc, Resource* bufferRsc) :
	m_desc(bufferDesc),
	m_rsc(bufferRsc)
{
	m_elemCount = m_desc.m_size / m_desc.m_stride.m_strideBytes;
}
