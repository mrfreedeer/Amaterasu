
#pragma once
#include "Engine/Math/Mat44.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"

class Buffer;
struct AABB3;

namespace AccelStructs {

	struct GeometryTriDesc {
		// A 3x4 matrix will be actually used, as this is what the API expects
		Mat44 m_transform;
		IndexBufferType m_indexType = IndexBufferType::UNKNOWN;
		TextureFormat m_vertexFormat = TextureFormat::INVALID;
		unsigned int m_indexCount = 0;
		unsigned int m_vertexCount = 0;
		Buffer* m_pIndexBuffer = nullptr;
		Buffer* m_pVertexBuffer = nullptr;
		RtGeomFlags m_flags = RtGeomFlags::None;
	};

	struct GeeometryAABBDesc {
		unsigned int m_count = 0;
		AABB3* m_aabbs = nullptr;
	};

	// PLACEHOLDER. THIS NEEDS TO BE IMPLEMENTED
	struct GeometryLinkageDesc {

	};

	struct GeometryOMMTriDesc {
		GeometryTriDesc* m_pTriDesc = nullptr;
		GeometryLinkageDesc* m_pLinkageDesc = nullptr;
	};

	struct AccelStructInputs {
		RtAccelStructType m_type = RtAccelStructType::BottomLevel;
		RtBuildFlags m_buildFlags = RtBuildFlags::None;
		unsigned int m_structCount = 0;
		RtElementsLayout m_layout = RtElementsLayout::Array;
		union {
			unsigned int m_instanceDescAddress = 0;
			void* m_instanceDescPointer;
		};
	};

	struct PrebuildInfo {
		unsigned int m_resultDataMaxSizeBytes = 0;
		unsigned int m_scratchDataSizeBytes = 0;
		unsigned int m_updateScratchDataSizeBytes = 0;
		AccelStructInputs m_inputs = {};
	};

	struct BuildDesc {
		GeometryTriDesc* m_triDesc;
		GeeometryAABBDesc* m_aabbDesc;
		GeometryOMMTriDesc* m_ommTriDesc;

		unsigned int m_structCount = 0;

		RtBuildFlags m_buildFlags = RtBuildFlags::None;
		RtAccelStructType m_type = RtAccelStructType::BottomLevel;
		AccelStructInputs m_inputs = {};
	};

	struct DispatchRaysDesc {
		unsigned int m_width = 0;
		unsigned int m_height = 0;
		unsigned int m_depth = 0;

		Buffer* m_rayGenShaderRecord = nullptr;
		Buffer* m_missShaderTable = nullptr;
		Buffer* m_hitGroupTable = nullptr;
		Buffer* m_callableShaderTable = nullptr;
	};

	struct ShaderTableSet {
		void* m_shaderIdentifiers[RTShaderSubType::NUM_RT_SHADER_SUB_TYPES] = { nullptr };
		Buffer* m_shaderTables[RTShaderSubType::NUM_RT_SHADER_SUB_TYPES] = { nullptr };
		unsigned int m_shaderRecordCount[RTShaderSubType::NUM_RT_SHADER_SUB_TYPES] = {0};
		unsigned int m_shaderRecordStrideBytes[RTShaderSubType::NUM_RT_SHADER_SUB_TYPES] = {0};
		unsigned int m_shaderIdentifierSizeBytes = 0;
		std::string m_debugName = "";

	};

	struct InstanceDesc {
		Mat44 m_transform = Mat44();
		unsigned int m_instanceID = 0;
		unsigned int m_instanceMask = 0xFF;
		unsigned int m_instanceContributionToHitGroupIndex = 0;
		RtGeomFlags m_flags = RtGeomFlags::None;
		Buffer* m_pBLASBuffer = nullptr;
	};

}