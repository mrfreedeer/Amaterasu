#pragma once
#include <stdint.h>
#include "Engine/Renderer/Interfaces/CommandList.hpp"
struct ID3D12CommandQueue;
class Fence;

enum class QueueFlags: uint8_t {
	None,
	DisableGPUTimeOut
};

struct CommandQueueDesc {
	QueueFlags m_flags = QueueFlags::None;
	CommandListType m_listType = CommandListType::DIRECT;
	char const* m_debugName = "UnnamedQueue";
};

class CommandQueue {
	friend class Renderer;
public:
	CommandQueue(CommandQueueDesc const& desc);
	~CommandQueue();

	CommandQueue& ExecuteCommandLists(unsigned int count, CommandList** cmdLists);
	CommandQueue& Signal(Fence* fence, unsigned int newFenceValue);
	CommandQueue& Wait(Fence* fence, unsigned int waitValue);

private:
	CommandQueueDesc m_desc = {};
	ID3D12CommandQueue* m_queue = nullptr;
};