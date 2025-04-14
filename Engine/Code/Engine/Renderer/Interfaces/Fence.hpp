#pragma once
struct ID3D12Fence1;

class CommandQueue;

class Fence {
friend class Renderer;
friend class CommandQueue;
public:
	Fence(CommandQueue* fenceManager, unsigned int initialValue = 0);
	~Fence();

	unsigned int Signal();
	void Wait(unsigned int waitValue);
	unsigned int GetFenceValue() const { return m_fenceValue; }
private:
	ID3D12Fence1* m_fence = nullptr;
	CommandQueue* m_commandQueue = nullptr;
	void* m_fenceEvent = nullptr;
	unsigned int m_fenceValue = 0;

};