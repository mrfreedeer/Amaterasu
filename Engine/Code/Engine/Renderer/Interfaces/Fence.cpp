#include "Engine/Renderer/Interfaces/Fence.hpp"
#include "Engine/Renderer/Interfaces/CommandQueue.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"
#include <d3d12.h>

Fence::Fence(CommandQueue* commandQueue, unsigned int initialValue /*= 0*/, unsigned int bufferCount /*= 1*/) :
	m_commandQueue(commandQueue),
	m_bufferCount(bufferCount)
{
	m_fenceValues = new unsigned int[m_bufferCount];
	memset(m_fenceValues, 0, sizeof(unsigned int) * m_bufferCount);
	m_fenceValues[0] = initialValue;


}

Fence::~Fence()
{
	CloseHandle(m_fenceEvent);
	delete[] m_fenceValues;
	DX_SAFE_RELEASE(m_fence);
}

unsigned int Fence::Signal()
{
	unsigned int currentValue = m_fenceValues[m_currentIndex];
	m_fence->Signal(currentValue);
	return currentValue;
}

unsigned int Fence::SignalGPU()
{
	unsigned int currentValue = m_fenceValues[m_currentIndex];
	m_commandQueue->Signal(this, currentValue);
	return currentValue;
}

void Fence::Wait()
{
	unsigned int currentValue = m_fenceValues[m_currentIndex];
	m_currentIndex = (m_currentIndex + 1) % m_bufferCount;

	unsigned int completedValue = (unsigned int)m_fence->GetCompletedValue();
	if (completedValue < currentValue)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(currentValue, m_fenceEvent), "FAILED TO SET EVENT ON COMPLETION");
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_fenceValues[m_currentIndex] = currentValue + 1;

}

unsigned int Fence::GetCompletedValue()
{
	return (unsigned int)m_fence->GetCompletedValue();
}

