#pragma once
struct ID3D12Fence1;

class CommandQueue;

class Fence {
friend class Renderer;
friend class CommandQueue;
public:

	/// <summary>
	/// Fence to wait for work to be done, can be used in buffering as well
	/// </summary>
	/// <param name="fenceManager"></param>
	/// <param name="bufferCount"></param>
	/// <param name="initialValue"></param>
	Fence(CommandQueue* fenceManager, unsigned int initialValue = 1, unsigned int bufferCount = 2);
	~Fence();

	unsigned int Signal();
	unsigned int SignalGPU();
	void Wait();
	unsigned int GetFenceValue() const { return m_fenceValues[m_currentIndex]; }

private:
	ID3D12Fence1* m_fence = nullptr;
	CommandQueue* m_commandQueue = nullptr;
	void* m_fenceEvent = nullptr;
	unsigned int* m_fenceValues = nullptr;
	unsigned int m_currentIndex = 0;
	unsigned int m_bufferCount = 0;

};