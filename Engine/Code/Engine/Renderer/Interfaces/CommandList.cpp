#include "Engine/Renderer/Interfaces/CommandList.hpp"
#include "Engine/Renderer/Interfaces/PipelineState.hpp"
#include "Engine/Renderer/Interfaces/Resource.hpp"
#include "Engine/Renderer/Interfaces/Buffer.hpp"
#include "Engine/Renderer/Interfaces/DescriptorHeap.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Renderer/D3D12/D3D12TypeConversions.hpp"

CommandList::CommandList(CommandListDesc const& desc) :
	m_desc(desc)
{

}

CommandList::~CommandList()
{
	DX_SAFE_RELEASE(m_cmdAllocator);
	DX_SAFE_RELEASE(m_cmdList);
}

CommandList& CommandList::Reset(PipelineState* initialState /*= nullptr*/)
{
	ID3D12PipelineState* pso = (initialState) ? initialState->m_pso : nullptr;
	HRESULT resetStatus = m_cmdAllocator->Reset();
	ThrowIfFailed(resetStatus, "FAILED TO RESET COMMAND ALLOC");
	resetStatus = m_cmdList->Reset(m_cmdAllocator, pso);
	ThrowIfFailed(resetStatus, "FAILED TO RESET COMMAND LIST");

	return *this;
}

CommandList& CommandList::Close()
{
	HRESULT closeStatus = m_cmdList->Close();
	ThrowIfFailed(closeStatus, "FAILED TO CLOSE COMMAND LIST");

	return *this;
}

CommandList& CommandList::CopyResource(Resource* dst, Resource* src)
{
	m_cmdList->CopyResource(dst->m_rawRsc, src->m_rawRsc);

	return *this;
}

CommandList& CommandList::ResourceBarrier(unsigned int count, TransitionBarrier* barriers)
{
	D3D12_RESOURCE_BARRIER* rscBarriers = new D3D12_RESOURCE_BARRIER[count];
	memset(rscBarriers, 0, sizeof(D3D12_RESOURCE_BARRIER) * count);

	unsigned int validBarrierCount = 0;
	for (unsigned int barrierIndex = 0; barrierIndex < count; barrierIndex++) {
		D3D12_RESOURCE_BARRIER& APIBarrier = rscBarriers[validBarrierCount];
		TransitionBarrier const& barrier = barriers[barrierIndex];
		if(barrier.m_before == barrier.m_after) continue;

		APIBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		APIBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		APIBarrier.Transition.pResource = barrier.m_rsc->m_rawRsc;
		APIBarrier.Transition.StateBefore = (D3D12_RESOURCE_STATES)barrier.m_before;
		APIBarrier.Transition.StateAfter = (D3D12_RESOURCE_STATES)barrier.m_after;
		APIBarrier.Transition.Subresource = 0;
		// Change state now that the state will be committed to the cmd list
		barrier.m_rsc->m_currentState = (int)barrier.m_after;

		validBarrierCount++;
	}

	if (validBarrierCount) {
		m_cmdList->ResourceBarrier(validBarrierCount, rscBarriers);
	}
	delete[] rscBarriers;

	return *this;

}

CommandList& CommandList::ClearDepthStencilView(Texture* depth, float clearValue, uint8_t stencilClearValue, bool clearStencil)
{
	D3D12_CLEAR_FLAGS clearFlags = D3D12_CLEAR_FLAG_DEPTH;
	if (clearStencil) clearFlags |= D3D12_CLEAR_FLAG_STENCIL;

	ResourceView* dsv = depth->GetDepthStencilView();
	if (!dsv) ERROR_AND_DIE(Stringf("DEPTH STENCIL VIEW IS NULL FOR RSC: %s", depth->GetDebugName()).c_str());
	m_cmdList->ClearDepthStencilView(dsv->m_descriptor, clearFlags, clearValue, stencilClearValue, 0, NULL);
	return *this;
}

CommandList& CommandList::ClearRenderTarget(Texture* rt, Rgba8 const& clearValue)
{
	float clearArray[4] = {};
	clearValue.GetAsFloats(clearArray);

	ResourceView* rtv = rt->GetRenderTargetView();

	m_cmdList->ClearRenderTargetView(rtv->m_descriptor, clearArray, 0, NULL);

	return *this;
}


CommandList& CommandList::Dispatch(IntVec3 threads)
{
	m_cmdList->Dispatch(threads.x, threads.y, threads.z);

	return *this;
}


CommandList& CommandList::CopyTexture(Texture* dst, Texture* src)
{
	TransitionBarrier resourceBarriers[2] = {};
	resourceBarriers[0] = src->GetTransitionBarrier(ResourceStates::CopySrc);
	resourceBarriers[1] = dst->GetTransitionBarrier(ResourceStates::CopyDest);

	ResourceBarrier(2, resourceBarriers);
	CopyResource(dst, src);

	return *this;
}

CommandList& CommandList::SetVertexBuffers(Buffer* const* buffers, unsigned int bufferCount, unsigned int startSlot /* = 0 */)
{
	D3D12_VERTEX_BUFFER_VIEW* vBuffersDesc = new D3D12_VERTEX_BUFFER_VIEW[bufferCount];

	for (unsigned int bufferIndex = 0; bufferIndex < bufferCount; bufferIndex++) {
		BufferView bufferView = buffers[bufferIndex]->GetBufferView();
		vBuffersDesc[bufferIndex] = { bufferView.m_bufferAddr, (UINT)bufferView.m_sizeBytes, (UINT)bufferView.m_stride.m_strideBytes};
	}

	m_cmdList->IASetVertexBuffers(startSlot, bufferCount, vBuffersDesc);

	delete[] vBuffersDesc;
	return *this;
}

CommandList& CommandList::SetIndexBuffer(Buffer* indexBuffer)
{
	BufferView bufferView = indexBuffer->GetBufferView();
	D3D12_INDEX_BUFFER_VIEW iBufferView = { bufferView.m_bufferAddr, (UINT)bufferView.m_sizeBytes, LocalToD3D12(bufferView.m_stride.m_format) };
	m_cmdList->IASetIndexBuffer(&iBufferView);
	return *this;
}

CommandList& CommandList::CopyBuffer(Buffer* dst, Buffer* src)
{
	TransitionBarrier resourceBarriers[2] = {};
	resourceBarriers[0] = src->GetTransitionBarrier(ResourceStates::CopySrc);
	resourceBarriers[1] = dst->GetTransitionBarrier(ResourceStates::CopyDest);
	ResourceBarrier(2, resourceBarriers);

	m_cmdList->CopyBufferRegion(dst->m_rawRsc, 0, src->m_rawRsc, 0, dst->GetSize());
	return *this;
}

CommandList& CommandList::SetTopology(TopologyType topology)
{
	D3D12_PRIMITIVE_TOPOLOGY topologyType = (D3D12_PRIMITIVE_TOPOLOGY)(topology);
	m_cmdList->IASetPrimitiveTopology(topologyType);
	return *this;
}

CommandList& CommandList::BindPipelineState(PipelineState* pipelineState)
{
	m_cmdList->SetPipelineState(pipelineState->m_pso);
	if (pipelineState->m_desc.m_type == PipelineType::Compute) {
		m_cmdList->SetComputeRootSignature(pipelineState->m_rootSignature);
	}
	else {
		m_cmdList->SetGraphicsRootSignature(pipelineState->m_rootSignature);
	}
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

CommandList& CommandList::SetRenderTargets(unsigned int rtCount, Texture* const* renderTargets, bool singleDescriptor, Texture* depthRenderTarget)
{
	D3D12_CPU_DESCRIPTOR_HANDLE* handleArray = new D3D12_CPU_DESCRIPTOR_HANDLE[rtCount];

	handleArray[0] = renderTargets[0]->GetRenderTargetView()->m_descriptor;

	if (!singleDescriptor) {
		for (unsigned int rtIndex = 1; rtIndex < rtCount; rtIndex++) {
			D3D12_CPU_DESCRIPTOR_HANDLE& handle = handleArray[rtIndex];
			Texture* currentRt = renderTargets[rtIndex];
			ResourceView* rtView = currentRt->GetRenderTargetView();
			handle = rtView->m_descriptor;
		}
	}

	D3D12_CPU_DESCRIPTOR_HANDLE* dsvDescriptor = nullptr;
	if (depthRenderTarget) {
		ResourceView* dsvView = depthRenderTarget->GetDepthStencilView();
		dsvDescriptor = &dsvView->m_descriptor;
	}


	m_cmdList->OMSetRenderTargets(rtCount, handleArray, singleDescriptor, dsvDescriptor);
	return *this;
}

CommandList& CommandList::SetDescriptorHeaps(unsigned int heapCount, DescriptorHeap** descriptorHeaps)
{
	ID3D12DescriptorHeap** heapArray = new ID3D12DescriptorHeap * [heapCount];
	for (unsigned int heapIndex = 0; heapIndex < heapCount; heapIndex++) {
		heapArray[heapIndex] = descriptorHeaps[heapIndex]->m_descriptorHeap;
	}
	m_cmdList->SetDescriptorHeaps(heapCount, heapArray);

	return *this;
}

CommandList& CommandList::SetDescriptorSet(DescriptorSet* dSet)
{
	// We set the descriptor heaps until RTV. RTV and DSV are CPU only
	SetDescriptorHeaps((int)DescriptorHeapType::RenderTargetView, dSet->m_descriptorHeaps);
	return *this;
}

CommandList& CommandList::SetDescriptorTable(unsigned int paramIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseGPUDescriptor, PipelineType pipelineType)
{
	if (pipelineType == PipelineType::Compute) {
		m_cmdList->SetComputeRootDescriptorTable(paramIndex, baseGPUDescriptor);
	}
	else {
		m_cmdList->SetGraphicsRootDescriptorTable(paramIndex, baseGPUDescriptor);
	}

	return *this;
}

CommandList& CommandList::SetDescriptorTable(unsigned int paramIndex, size_t baseGPUDescriptor, PipelineType pipelineType)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle = {};
	handle.ptr = baseGPUDescriptor;
	return SetDescriptorTable(paramIndex, handle, pipelineType);
}

CommandList& CommandList::SetGraphicsRootConstants(unsigned int count, unsigned int* constants)
{
	m_cmdList->SetGraphicsRoot32BitConstants(PARAM_ROOT_CONSTANTS, count, constants, 0);
	return *this;
}

CommandList& CommandList::SetViewport(AABB2 const& viewport, float depthMin /*= 0.0f*/, float depthMax /*= 1.0f*/)
{
	D3D12_VIEWPORT apiViewport = {viewport.m_mins.x, viewport.m_mins.y, viewport.m_maxs.x, viewport.m_maxs.y, depthMin, depthMax};
	m_cmdList->RSSetViewports(1, &apiViewport);
	return *this;
}

CommandList& CommandList::SetViewport(Vec2 const& viewportMin, Vec2 const& viewportMax, float depthMin /*= 0.0f*/, float depthMax /*= 1.0f*/)
{
	D3D12_VIEWPORT apiViewport = { viewportMin.x, viewportMin.y, viewportMax.x, viewportMax.y, depthMin, depthMax};
	m_cmdList->RSSetViewports(1, &apiViewport);
	return *this;
}

CommandList& CommandList::SetScissorRect(AABB2 const& rect)
{
	D3D12_RECT apiRect = { (LONG)rect.m_mins.x, (LONG)rect.m_mins.y, (LONG)rect.m_maxs.x, (LONG)rect.m_maxs.y };
	m_cmdList->RSSetScissorRects(1, &apiRect);
	return *this;
}

CommandList& CommandList::SetScissorRect(Vec2 const& rectMin, Vec2 const& rectMax)
{
	D3D12_RECT apiRect = { (LONG)rectMin.x, (LONG)rectMin.y, (LONG)rectMax.x, (LONG)rectMax.y };
	m_cmdList->RSSetScissorRects(1, &apiRect);
	return *this;
}

CommandList& CommandList::SetBlendFactor(float* blendFactors)
{
	m_cmdList->OMSetBlendFactor(blendFactors);
	return *this;
}

