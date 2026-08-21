#include "Engine/Renderer/D3D12/D3D12TypeConversions.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/Interfaces/Buffer.hpp"
#include "Engine/Renderer/RayTracingCommon.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"

DXGI_FORMAT LocalToD3D12(TextureFormat textureFormat)
{
	switch (textureFormat) {
	case TextureFormat::R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case TextureFormat::D24_UNORM_S8_UINT: return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case TextureFormat::R32G32B32A32_FLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case TextureFormat::R32G32_FLOAT: return DXGI_FORMAT_R32G32_FLOAT;
	case TextureFormat::R24G8_TYPELESS: return DXGI_FORMAT_R24G8_TYPELESS;
	case TextureFormat::R32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
	case TextureFormat::D32_FLOAT: return DXGI_FORMAT_D32_FLOAT;
	case TextureFormat::UNKNOWN: return DXGI_FORMAT_UNKNOWN;
	default: ERROR_AND_DIE("Unsupported format");
	}
}

D3D12_FILL_MODE LocalToD3D12(FillMode fillMode)
{
	switch (fillMode)
	{
	case FillMode::SOLID: return D3D12_FILL_MODE_SOLID;
	case FillMode::WIREFRAME: return D3D12_FILL_MODE_WIREFRAME;
	default: ERROR_AND_DIE("Unsupported format");
	}
}

D3D12_CULL_MODE LocalToD3D12(CullMode cullMode)
{
	switch (cullMode)
	{
	case CullMode::NONE: return D3D12_CULL_MODE_NONE;
	case CullMode::FRONT: return D3D12_CULL_MODE_FRONT;
	case CullMode::BACK: return D3D12_CULL_MODE_BACK;
	default: ERROR_AND_DIE("Unsupported cull mode");
	}
}

BOOL LocalToD3D12(WindingOrder windingOrder)
{
	switch (windingOrder)
	{
	case WindingOrder::CLOCKWISE: return FALSE;
	case WindingOrder::COUNTERCLOCKWISE: return TRUE;
	default: ERROR_AND_DIE("Unsupported winding order");
	}
}

D3D12_COMPARISON_FUNC LocalToD3D12(DepthFunc depthTest)
{
	switch (depthTest) {
	case DepthFunc::NEVER: return D3D12_COMPARISON_FUNC_NEVER;
	case DepthFunc::LESS: return D3D12_COMPARISON_FUNC_LESS;
	case DepthFunc::EQUAL: return D3D12_COMPARISON_FUNC_EQUAL;
	case DepthFunc::LESSEQUAL: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case DepthFunc::GREATER: return D3D12_COMPARISON_FUNC_GREATER;
	case DepthFunc::NOTEQUAL: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case DepthFunc::GREATEREQUAL: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case DepthFunc::ALWAYS: return D3D12_COMPARISON_FUNC_ALWAYS;
	default:
		ERROR_AND_DIE(Stringf("UNSUPPORTED DEPTH TEST %d", (int)depthTest).c_str());
	}

}

D3D12_VERTEX_BUFFER_VIEW LocalToD3D12(BufferView const& bufferView)
{
	return {bufferView.m_bufferAddr, (UINT)bufferView.m_sizeBytes, (UINT)bufferView.m_stride.m_strideBytes};
}

D3D12_DESCRIPTOR_HEAP_TYPE LocalToD3D12(DescriptorHeapType heapType)
{
	switch (heapType)
	{
	case DescriptorHeapType::CBV_SRV_UAV:
		return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	case DescriptorHeapType::Sampler:
		return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	case DescriptorHeapType::RenderTargetView:
		return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	case DescriptorHeapType::DepthStencilView:
		return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	default:
		ERROR_AND_DIE(Stringf("UNRECOGNIZED HEAP TYPE: %d", (heapType)));
		break;
	}
}

D3D12_COMMAND_LIST_TYPE LocalToD3D12(CommandListType cmdListType)
{
	switch (cmdListType)
	{
	case CommandListType::DIRECT:
		return D3D12_COMMAND_LIST_TYPE_DIRECT;
	case CommandListType::COMPUTE:
		return D3D12_COMMAND_LIST_TYPE_COMPUTE;
	case CommandListType::BUNDLE:
		return D3D12_COMMAND_LIST_TYPE_BUNDLE;
	case CommandListType::COPY:
		return D3D12_COMMAND_LIST_TYPE_COPY;
	case CommandListType::VIDEO_DECODE:
		return D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE;
	case CommandListType::VIDEO_PROCESS:
		return D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS;
	case CommandListType::VIDEO_ENCODE:
		return D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE;
	default:
		ERROR_AND_DIE(Stringf("UNRECOGNIZED COMMAND LIST %d", (int)cmdListType).c_str());
		break;
	}
}

D3D12_COMMAND_QUEUE_FLAGS LocalToD3D12(QueueFlags queueFlags)
{
	switch (queueFlags)
	{
	case QueueFlags::DisableGPUTimeOut:
		return D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;
	case QueueFlags::None:
	default:
		return D3D12_COMMAND_QUEUE_FLAG_NONE;
	}
}

D3D12_HEAP_TYPE LocalToD3D12(MemoryUsage memoryUsage)
{
	switch (memoryUsage)
	{
	case MemoryUsage::Default:	return D3D12_HEAP_TYPE_DEFAULT;
	case MemoryUsage::Dynamic:	return D3D12_HEAP_TYPE_UPLOAD;
	case MemoryUsage::Readback: return D3D12_HEAP_TYPE_READBACK;
	default:
		ERROR_AND_DIE("UNRECOGNIZED MEMORY USAGE");
	}
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE LocalToD3D12(TopologyType topology)
{
	switch (topology)
	{
	
	case TopologyType::POINTLIST:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	case TopologyType::TRIANGLELIST:
	case TopologyType::TRIANGLESTRIP:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case TopologyType::LINELIST:
	case TopologyType::LINESTRIP:
	case TopologyType::LINELIST_ADJ:
	case TopologyType::LINESTRIP_ADJ:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case TopologyType::TRIANGLELIST_ADJ:
	case TopologyType::TRIANGLESTRIP_ADJ:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case TopologyType::CONTROL_POINT_PATCHLIST_1:
	case TopologyType::CONTROL_POINT_PATCHLIST_2:
	case TopologyType::CONTROL_POINT_PATCHLIST_3:
	case TopologyType::CONTROL_POINT_PATCHLIST_4:
	case TopologyType::CONTROL_POINT_PATCHLIST_5:
	case TopologyType::CONTROL_POINT_PATCHLIST_6:
	case TopologyType::CONTROL_POINT_PATCHLIST_7:
	case TopologyType::CONTROL_POINT_PATCHLIST_8:
	case TopologyType::CONTROL_POINT_PATCHLIST_9:
	case TopologyType::CONTROL_POINT_PATCHLIST_10:
	case TopologyType::CONTROL_POINT_PATCHLIST_11:
	case TopologyType::CONTROL_POINT_PATCHLIST_12:
	case TopologyType::CONTROL_POINT_PATCHLIST_13:
	case TopologyType::CONTROL_POINT_PATCHLIST_14:
	case TopologyType::CONTROL_POINT_PATCHLIST_15:
	case TopologyType::CONTROL_POINT_PATCHLIST_16:
	case TopologyType::CONTROL_POINT_PATCHLIST_17:
	case TopologyType::CONTROL_POINT_PATCHLIST_18:
	case TopologyType::CONTROL_POINT_PATCHLIST_19:
	case TopologyType::CONTROL_POINT_PATCHLIST_20:
	case TopologyType::CONTROL_POINT_PATCHLIST_21:
	case TopologyType::CONTROL_POINT_PATCHLIST_22:
	case TopologyType::CONTROL_POINT_PATCHLIST_23:
	case TopologyType::CONTROL_POINT_PATCHLIST_24:
	case TopologyType::CONTROL_POINT_PATCHLIST_25:
	case TopologyType::CONTROL_POINT_PATCHLIST_26:
	case TopologyType::CONTROL_POINT_PATCHLIST_27:
	case TopologyType::CONTROL_POINT_PATCHLIST_28:
	case TopologyType::CONTROL_POINT_PATCHLIST_29:
	case TopologyType::CONTROL_POINT_PATCHLIST_30:
	case TopologyType::CONTROL_POINT_PATCHLIST_31:
	case TopologyType::CONTROL_POINT_PATCHLIST_32:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	case TopologyType::UNDEFINED:
	default:
		ERROR_AND_DIE("UNDEFINED TOPOLOGY");
		break;
	}

}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE LocalToD3D12(RtAccelStructType asType)
{
	switch (asType)
	{
	case RtAccelStructType::TopLevel: return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE::D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	case RtAccelStructType::BottomLevel: return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE::D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	default:
		ERROR_AND_DIE("UNKNOWN AS TYPE");
		break;
	}
}

D3D12_RAYTRACING_GEOMETRY_DESC LocalToD3D12(AccelStructs::GeometryTriDesc const& triDesc)
{
	D3D12_RAYTRACING_GEOMETRY_DESC convertedDesc = {};
	convertedDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	convertedDesc.Triangles.IndexBuffer = triDesc.m_pIndexBuffer->GetGPUAddress();
	convertedDesc.Triangles.IndexCount = triDesc.m_indexCount;
	convertedDesc.Triangles.IndexFormat = LocalToD3D12(triDesc.m_indexType);
	convertedDesc.Triangles.Transform3x4 = 0;
	convertedDesc.Triangles.VertexFormat = LocalToD3D12(triDesc.m_vertexFormat);
	convertedDesc.Triangles.VertexCount = triDesc.m_vertexCount;
	convertedDesc.Triangles.VertexBuffer.StartAddress = triDesc.m_pVertexBuffer->GetGPUAddress();
	convertedDesc.Triangles.VertexBuffer.StrideInBytes = triDesc.m_pVertexBuffer->GetSize() / triDesc.m_pVertexBuffer->GetElementCount();

	convertedDesc.Flags = LocalToD3D12(triDesc.m_flags);

	return convertedDesc;
}

DXGI_FORMAT LocalToD3D12(IndexBufferType indexType)
{
	switch (indexType)
	{
	case IndexBufferType::R16_UINT: return DXGI_FORMAT_R16_UINT;
	case IndexBufferType::R32_UINT: return DXGI_FORMAT_R32_UINT;
	default: ERROR_AND_DIE("UNKNOWN INDEX BUFFER TYPE");
	}
}

D3D12_RAYTRACING_GEOMETRY_FLAGS LocalToD3D12(RtGeomFlags flags)
{
	switch (flags)
	{
	case RtGeomFlags::None: return D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
	case RtGeomFlags::Opaque: return D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	case RtGeomFlags::NoDuplicateAnyHitInvocations:return D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
	default:
		ERROR_AND_DIE("UNKNOWN RT GEOM FLAG");
		break;
	}
}

D3D12_DISPATCH_RAYS_DESC LocalToD3D12(AccelStructs::DispatchRaysDesc const& dispatchDesc)
{
	D3D12_DISPATCH_RAYS_DESC convertedDesc = {};

	convertedDesc.Width = dispatchDesc.m_width;
	convertedDesc.Height = dispatchDesc.m_height;
	convertedDesc.Depth = dispatchDesc.m_depth;

	if (dispatchDesc.m_callableShaderTable) {
		convertedDesc.CallableShaderTable.StartAddress = dispatchDesc.m_callableShaderTable->GetGPUAddress();
		convertedDesc.CallableShaderTable.SizeInBytes = dispatchDesc.m_callableShaderTable->GetSize();
		convertedDesc.CallableShaderTable.StrideInBytes = dispatchDesc.m_callableShaderTable->GetStride();
	}

	if (dispatchDesc.m_hitGroupTable) {
		convertedDesc.HitGroupTable.StartAddress = dispatchDesc.m_hitGroupTable->GetGPUAddress();
		convertedDesc.HitGroupTable.SizeInBytes = dispatchDesc.m_hitGroupTable->GetSize();
		convertedDesc.HitGroupTable.StrideInBytes = dispatchDesc.m_hitGroupTable->GetStride();
	}

	if (dispatchDesc.m_missShaderTable) {
		convertedDesc.MissShaderTable.StartAddress = dispatchDesc.m_missShaderTable->GetGPUAddress();
		convertedDesc.MissShaderTable.SizeInBytes = dispatchDesc.m_missShaderTable->GetSize();
		convertedDesc.MissShaderTable.StrideInBytes = dispatchDesc.m_missShaderTable->GetStride();
	}

	if (dispatchDesc.m_rayGenShaderRecord) {
		convertedDesc.RayGenerationShaderRecord.StartAddress = dispatchDesc.m_rayGenShaderRecord->GetGPUAddress();
		convertedDesc.RayGenerationShaderRecord.SizeInBytes = dispatchDesc.m_rayGenShaderRecord->GetSize();
	}

	return convertedDesc;
}

D3D12_RAYTRACING_INSTANCE_DESC LocalToD3D12(AccelStructs::InstanceDesc const& instanceDesc)
{
	D3D12_RAYTRACING_INSTANCE_DESC apiInstanceDesc = {};

	// Only 12 of the 16 values are used, no w is used.
	instanceDesc.m_transform.GetAsFloatArrayNonHomogeneous3D(apiInstanceDesc.Transform);
	apiInstanceDesc.Flags = LocalToD3D12(instanceDesc.m_flags);
	apiInstanceDesc.AccelerationStructure = instanceDesc.m_pBLASBuffer->GetGPUAddress();
	apiInstanceDesc.InstanceID = instanceDesc.m_instanceID;
	apiInstanceDesc.InstanceMask = instanceDesc.m_instanceMask;
	return apiInstanceDesc;
}

D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS LocalToD3D12(AccelStructs::AccelStructInputs const& buildInputs)
{
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};

	inputs.Type = LocalToD3D12(buildInputs.m_type);
	inputs.DescsLayout = LocalToD3D12(buildInputs.m_layout);
	inputs.Flags = LocalToD3D12(buildInputs.m_buildFlags);
	inputs.InstanceDescs = buildInputs.m_instanceDescAddress;
	inputs.NumDescs = buildInputs.m_structCount;

	return inputs;
}

D3D12_ELEMENTS_LAYOUT LocalToD3D12(RtElementsLayout layout)
{
	switch (layout)
	{
	case RtElementsLayout::Array: return D3D12_ELEMENTS_LAYOUT_ARRAY;
	case RtElementsLayout::ArrayOfPointers: return D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS;
	default: ERROR_AND_DIE("UNKNOWN RT ELEMENTS LAYOUT");
	}
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS LocalToD3D12(RtBuildFlags buildFlags)
{
	switch (buildFlags)
	{
	case RtBuildFlags::None:		     return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	case RtBuildFlags::AllowUpdate:		 return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	case RtBuildFlags::AllowCompaction:  return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
	case RtBuildFlags::PreferFastTrace:  return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	case RtBuildFlags::PreferFastBuild:  return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
	case RtBuildFlags::MinimizeMemory:	 return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
	case RtBuildFlags::PerformUpdate:	 return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
	case RtBuildFlags::AllowOMMUpdate:	 return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_OMM_UPDATE;
	case RtBuildFlags::AllowDisableOmms: return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_DISABLE_OMMS;
	case RtBuildFlags::NUM_BUILD_FLAGS:	 return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	default:
		ERROR_AND_DIE("UNKNOWN RT BUILD FLAG");
		break;
	}
}

DXGI_FORMAT LocalToColourD3D12(TextureFormat textureFormat)
{
	switch (textureFormat) {
	case TextureFormat::R24G8_TYPELESS: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	case TextureFormat::D32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
	default: return LocalToD3D12(textureFormat);
	}
}


D3D12_RESOURCE_FLAGS LocalToD3D12(ResourceBindFlag flags)
{
	D3D12_RESOURCE_FLAGS result = D3D12_RESOURCE_FLAG_NONE;

	if (flags & ResourceBindFlagBit::RESOURCE_BIND_SHADER_RESOURCE_BIT) {
		result |= D3D12_RESOURCE_FLAG_NONE;
	}

	if (flags & ResourceBindFlagBit::RESOURCE_BIND_RENDER_TARGET_BIT) {
		result |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}

	if (flags & ResourceBindFlagBit::RESOURCE_BIND_DEPTH_STENCIL_BIT) {
		result |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	}

	if (flags & ResourceBindFlagBit::RESOURCE_BIND_UNORDERED_ACCESS_VIEW_BIT) {
		result |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	return result;
}
