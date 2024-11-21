#include "Engine/Renderer/Interfaces/CommandList.hpp"
#include "Engine/Renderer/Interfaces/PipelineState.hpp"
#include "Engine/Renderer/Interfaces/Resource.hpp"
#include "Engine/Renderer/Interfaces/Buffer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Renderer/D3D12/D3D12TypeConversions.hpp"

CommandList::CommandList(CommandListDesc const& desc) :
	m_desc(desc)
{

}

CommandList& CommandList::Reset(PipelineState* initialState /*= nullptr*/)
{
	m_cmdAllocator->Reset();
	m_cmdList->Reset(m_cmdAllocator, initialState->m_pso);
	
	return *this;
}

CommandList& CommandList::Close()
{
	m_cmdList->Close();

	return *this;
}

CommandList& CommandList::ClearDepthStencilView(Texture* depth, float clearValue, uint8_t stencilClearValue, bool clearStencil)
{
	D3D12_CLEAR_FLAGS clearFlags = D3D12_CLEAR_FLAG_DEPTH;
	if(clearStencil) clearFlags |= D3D12_CLEAR_FLAG_STENCIL;

	ResourceView* dsv = depth->GetResource()->GetDepthStencilView();
	if(!dsv) ERROR_AND_DIE("DEPTH STENCIL VIEW IS NULL FOR RSC: %s", depth->m_debugName);
	m_cmdList->ClearDepthStencilView(dsv->m_descriptor, clearFlags, clearValue, stencilClearValue, 0, NULL);
	return *this;
}

CommandList& CommandList::ClearRenderTargetView(Texture* rt, Rgba8 const& clearValue)
{
	float clearArray[4] = {};
	clearValue.GetAsFloats(clearArray);

	ResourceView* rtv = rt->GetResource()->GetRenderTargetView();

	m_cmdList->ClearRenderTargetView(rtv->m_descriptor, clearArray, 0, NULL);

	return *this;
}

CommandList& CommandList::Dispatch(IntVec3 threads)
{
	m_cmdList->Dispatch(threads.x, threads.y, threads.z);

	return *this;
}

CommandList& CommandList::SetVertexBuffers(Buffer** buffers, unsigned int bufferCount, unsigned int startSlot /* = 0 */)
{
	D3D12_VERTEX_BUFFER_VIEW* vBuffersDesc = new D3D12_VERTEX_BUFFER_VIEW[bufferCount];

	for (int bufferIndex = 0; bufferIndex < bufferCount; bufferIndex++) {
		BufferView bufferView = buffers[bufferIndex]->GetBufferView();
		vBuffersDesc[bufferIndex] = {bufferView.m_bufferAddr, (UINT)bufferView.m_stride.m_strideBytes, (UINT)bufferView.m_sizeBytes};
	}
	
	m_cmdList->IASetVertexBuffers(startSlot, bufferCount,vBuffersDesc);

	delete[] vBuffersDesc;
	return *this;
}

CommandList& CommandList::SetIndexBuffer(Buffer* indexBuffer)
{
	BufferView bufferView = indexBuffer->GetBufferView();
	D3D12_INDEX_BUFFER_VIEW iBufferView = { bufferView.m_bufferAddr, bufferView.m_sizeBytes, LocalToD3D12(bufferView.m_stride.m_format)};
	m_cmdList->IASetIndexBuffer(&iBufferView);
	return *this;
}

CommandList& CommandList::DrawIndexedInstanced(unsigned int instanceIndexCount, unsigned int instanceCount, unsigned int startIndex, unsigned int startVertex, unsigned int startInstance)
{
	m_cmdList->DrawIndexedInstanced(instanceIndexCount, instanceCount, startIndex, startVertex, startInstance);
	return *this;
}

CommandList& CommandList::DrawInstance(unsigned int instanceVertexCount, unsigned int instanceCount, unsigned int startVertex, unsigned int startInstance)
{
	m_cmdList->DrawInstanced(instanceVertexCount, instanceCount, startVertex, startInstance);
	return *this;
}

