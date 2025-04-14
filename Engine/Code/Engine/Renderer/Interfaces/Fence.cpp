#include "Engine/Renderer/Interfaces/Fence.hpp"
#include "Engine/Renderer/Interfaces/CommandQueue.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"
#include <d3d12.h>

Fence::Fence(CommandQueue* commandQueue, unsigned int initialValue /*= 0*/):
	m_fenceValue(initialValue),
	m_commandQueue(commandQueue)
{
	

}

Fence::~Fence()
{
	DX_SAFE_RELEASE(m_fence);
}

unsigned int Fence::Signal()
{
	m_fenceValue++;
	m_commandQueue->Signal(this, m_fenceValue);
	return m_fenceValue;
}

void Fence::Wait(unsigned int waitValue)
{
	if (m_fence->GetCompletedValue() < waitValue)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent), "FAILED TO SET EVENT ON COMPLETION");
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

