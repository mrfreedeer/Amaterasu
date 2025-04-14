#include "Engine/Renderer/Interfaces/CommandQueue.hpp"
#include "Engine/Renderer/Interfaces/Fence.hpp"
#include <Engine/Renderer/GraphicsCommon.hpp>
#include <d3d12.h>

CommandQueue::CommandQueue(CommandQueueDesc const& desc) :
	m_desc(desc)
{

}

CommandQueue::~CommandQueue()
{
	DX_SAFE_RELEASE(m_queue);
}

CommandQueue& CommandQueue::ExecuteCommandLists(unsigned int count, CommandList** cmdLists)
{
	ID3D12CommandList** rawCmdLists = new ID3D12CommandList*[count];
	for (unsigned int cmdListIndex = 0; cmdListIndex < count; cmdListIndex++) {
		rawCmdLists[cmdListIndex] = cmdLists[cmdListIndex]->m_cmdList;
	}

	m_queue->ExecuteCommandLists(count, rawCmdLists);	

	delete[] rawCmdLists;

	return *this;
}

CommandQueue& CommandQueue::Signal(Fence* fence, unsigned int newFenceValue)
{
	ThrowIfFailed(m_queue->Signal(fence->m_fence, newFenceValue), "FAILED TO SIGNAL FENCE");
	return *this;

}

CommandQueue& CommandQueue::Wait(Fence* fence, unsigned int waitValue)
{
	m_queue->Wait(fence->m_fence, waitValue);
	return *this;
}

