#pragma once

struct PipelineState;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList6;

enum class CommandListType {
	DIRECT = 0,
	BUNDLE = 1,
	COMPUTE = 2,
	COPY = 3,
	VIDEO_DECODE = 4,
	VIDEO_PROCESS = 5,
	VIDEO_ENCODE,
	NONE
};


struct CommandListDesc {
	CommandListType m_type = CommandListType::DIRECT;
	PipelineState* m_initialState = nullptr;
};


class CommandList {
	friend class Renderer;
public:

private:
	CommandList(CommandListDesc const& desc);

private:
	CommandListDesc m_desc = {};
	ID3D12CommandAllocator* m_cmdAllocator = nullptr;
	ID3D12GraphicsCommandList6* m_cmdList = nullptr;
};

