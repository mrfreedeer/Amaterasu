#include <d3d12.h>
#include <dxgiformat.h>
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Interfaces/DescriptorHeap.hpp"
#include "Engine/Renderer/Interfaces/CommandList.hpp"
#include "Engine/Renderer/Interfaces/CommandQueue.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"

struct BufferView;
typedef unsigned int        UINT;

namespace AccelStructs {
	struct GeometryTriDesc;
}

DXGI_FORMAT LocalToD3D12(TextureFormat textureFormat);
DXGI_FORMAT LocalToColourD3D12(TextureFormat textureFormat);
D3D12_RESOURCE_FLAGS LocalToD3D12(ResourceBindFlag flags);
D3D12_DESCRIPTOR_HEAP_TYPE LocalToD3D12(DescriptorHeapType dHeapType);
D3D12_FILL_MODE LocalToD3D12(FillMode fillMode);
D3D12_CULL_MODE LocalToD3D12(CullMode cullMode);
D3D12_COMPARISON_FUNC LocalToD3D12(DepthFunc depthTest);
BOOL LocalToD3D12(WindingOrder windingOrder);
D3D12_VERTEX_BUFFER_VIEW LocalToD3D12(BufferView const& bufferView);
D3D12_COMMAND_LIST_TYPE LocalToD3D12(CommandListType cmdListType);
D3D12_COMMAND_QUEUE_FLAGS LocalToD3D12(QueueFlags queueFlags);
D3D12_HEAP_TYPE LocalToD3D12(MemoryUsage memoryUsage);
D3D12_PRIMITIVE_TOPOLOGY_TYPE LocalToD3D12(TopologyType topology);

// Raytracing specific conversions
D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE LocalToD3D12(RtAccelStructType asType);
D3D12_RAYTRACING_GEOMETRY_DESC LocalToD3D12(AccelStructs::GeometryTriDesc const& triDesc);
DXGI_FORMAT LocalToD3D12(IndexBufferType indexType);
D3D12_RAYTRACING_GEOMETRY_FLAGS LocalToD3D12(RtBuildFlags flags);
