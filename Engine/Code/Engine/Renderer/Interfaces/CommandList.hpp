#pragma once
#include <stdint.h>
#include "Engine/Renderer/GraphicsCommon.hpp"


struct PipelineState;
struct IntVec3;
struct Rgba8;
struct AABB2;
struct TransitionBarrier;
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
	std::string m_debugName = "Unnamed Cmd List";
};


class CommandList {
	friend class Renderer;
	friend class CommandQueue;
	CommandList(CommandListDesc const& desc);
public:
	~CommandList();
	CommandList& Reset(PipelineState* initialState = nullptr);
	CommandList& Close();
	
	// All resources	
	CommandList& CopyResource(Resource* dst, Resource* src);
	CommandList& ResourceBarrier(unsigned int count, TransitionBarrier* barriers);

	// Textures
	CommandList& ClearDepthStencilView(Texture* depth, float clearValue, uint8_t stencilClearValue = 0, bool clearStencil = false);
	CommandList& ClearRenderTarget(Texture* rt, Rgba8 const& clearValue);
	CommandList& SetRenderTargets(unsigned int rtCount, Texture* const* renderTargets, bool singleDescriptor, Texture* depthRenderTarget);
	CommandList& Dispatch(IntVec3 threads);
	CommandList& CopyTexture(Texture* dst, Texture* src);

	// Buffers
	CommandList& SetVertexBuffers(Buffer* const* buffers, unsigned int bufferCount, unsigned int startSlot = 0);
	CommandList& SetIndexBuffer(Buffer* indexBuffer);
	CommandList& CopyBuffer(Buffer* dst, Buffer* src);

	// Draws/State
	CommandList& SetTopology(TopologyType topology);
	CommandList& BindPipelineState(PipelineState* pipelineState);
	CommandList& DrawIndexedInstanced(unsigned int instanceIndexCount, unsigned int instanceCount, unsigned int startIndex, unsigned int startVertex, unsigned int startInstance);
	CommandList& DrawInstance(unsigned int instanceVertexCount, unsigned int instanceCount, unsigned int startVertex, unsigned int startInstance);
	CommandList& SetDescriptorHeaps(unsigned int heapCount, DescriptorHeap** descriptorHeaps);
	CommandList& SetDescriptorSet(DescriptorSet* dSet);
	CommandList& SetDescriptorTable(unsigned int paramIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseGPUDescriptor, PipelineType pipelineType);
	CommandList& SetDescriptorTable(unsigned int paramIndex, size_t baseGPUDescriptor, PipelineType pipelineType);
	CommandList& SetGraphicsRootConstants(unsigned int count, unsigned int* constants);
	CommandList& SetViewport(AABB2 const& viewport, float depthMin = 0.0f, float depthMax = 1.0f);
	CommandList& SetViewport(Vec2 const& viewportMin, Vec2 const& viewportMax, float depthMin = 0.0f, float depthMax = 1.0f);
	CommandList& SetScissorRect(AABB2 const& rect);
	CommandList& SetScissorRect(Vec2 const& rectMin, Vec2 const& rectMax);
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

