#pragma once
#include <stdint.h>
#include "Engine/Renderer/GraphicsCommon.hpp"


struct PipelineState;
struct IntVec3;
struct Rgba8;
class Camera;
class Resource;
class Texture;
class Buffer;

struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList6;
class DescriptorHeap;
struct DescriptorSet;

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;

struct CommandListDesc {
	CommandListType m_type = CommandListType::DIRECT;
	PipelineState* m_initialState = nullptr;
};


class CommandList {
	friend class Renderer;
	friend class CommandQueue;
	CommandList(CommandListDesc const& desc);
public:
	~CommandList();
	CommandList& Reset(PipelineState* initialState = nullptr);
	CommandList& Close();
	
	// Textures
	CommandList& ClearDepthStencilView(Texture* depth, float clearValue, uint8_t stencilClearValue = 0, bool clearStencil = false);
	CommandList& ClearRenderTarget(Texture* rt, Rgba8 const& clearValue);
	CommandList& SetRenderTargets(unsigned int rtCount, Texture* const* renderTargets, bool singleDescriptor, Texture* depthRenderTarget);
	CommandList& Dispatch(IntVec3 threads);

	// Buffers
	CommandList& SetVertexBuffers(Buffer** buffers, unsigned int bufferCount, unsigned int startSlot = 0);
	CommandList& SetIndexBuffer(Buffer* indexBuffer);
	CommandList& CopyBuffer(Buffer* src, Buffer* dest);

	// Draws/State
	CommandList& BindPipelineState(PipelineState* pipelineState);
	CommandList& DrawIndexedInstanced(unsigned int instanceIndexCount, unsigned int instanceCount, unsigned int startIndex, unsigned int startVertex, unsigned int startInstance);
	CommandList& DrawInstance(unsigned int instanceVertexCount, unsigned int instanceCount, unsigned int startVertex, unsigned int startInstance);
	CommandList& SetDescriptorHeaps(unsigned int heapCount, DescriptorHeap** descriptorHeaps);
	CommandList& SetDescriptorSet(DescriptorSet* dSet);
	CommandList& SetDescriptorTable(unsigned int paramIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseGPUDescriptor, PipelineType pipelineType);
	/// <summary>
	/// Set blend factor
	/// </summary>
	/// <param name="blendFactors"> array of 4 floats</param>
	/// <returns></returns>
	CommandList& SetBlendFactor(float* blendFactors);
private:

private:
	CommandListDesc m_desc = {};
	ID3D12CommandAllocator* m_cmdAllocator = nullptr;
	ID3D12GraphicsCommandList6* m_cmdList = nullptr;
	Camera const* m_currentCamera = nullptr;
};

