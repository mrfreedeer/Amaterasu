#pragma once
#include <stdint.h>


struct PipelineState;
struct IntVec3;
struct Rgba8;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList6;
class Resource;
class Texture;
class Buffer;
class DescriptorHeap;

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
	friend class CommandQueue;
	CommandList(CommandListDesc const& desc);
	~CommandList();
public:
	CommandList& Reset(PipelineState* initialState = nullptr);
	CommandList& Close();
	
	// Textures
	CommandList& ClearDepthStencilView(Texture* depth, float clearValue, uint8_t stencilClearValue = 0, bool clearStencil = false);
	CommandList& ClearRenderTargetView(Texture* rt, Rgba8 const& clearValue);
	CommandList& Dispatch(IntVec3 threads);
	CommandList& SetVertexBuffers(Buffer** buffers, unsigned int bufferCount, unsigned int startSlot = 0);
	CommandList& SetIndexBuffer(Buffer* indexBuffer);
	CommandList& DrawIndexedInstanced(unsigned int instanceIndexCount, unsigned int instanceCount, unsigned int startIndex, unsigned int startVertex, unsigned int startInstance);
	CommandList& DrawInstance(unsigned int instanceVertexCount, unsigned int instanceCount, unsigned int startVertex, unsigned int startInstance);
	CommandList& SetRenderTargets(unsigned int rtCount, Texture** renderTargets, bool singleDescriptor, Texture* depthRenderTarget);
	CommandList& SetDescriptorHeaps(unsigned int heapCount, DescriptorHeap** descriptorHeaps);


private:

private:
	CommandListDesc m_desc = {};
	ID3D12CommandAllocator* m_cmdAllocator = nullptr;
	ID3D12GraphicsCommandList6* m_cmdList = nullptr;
};

