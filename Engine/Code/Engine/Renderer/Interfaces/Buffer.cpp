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
	return { m_rsc->GetGPUAddress(), m_desc.m_stride, m_desc.m_size };
}

Buffer::Buffer(BufferDesc const& bufferDesc, Resource* bufferRsc) :
	m_desc(bufferDesc),
	m_rsc(bufferRsc)
{

}
