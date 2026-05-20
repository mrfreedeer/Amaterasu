
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

	struct PrebuildInfo {
		unsigned int m_resultDataMaxSizeBytes = 0;
		unsigned int m_scratchDataSizeBytes = 0;
		unsigned int m_updateScratchDataSizeBytes = 0;
	};

	struct BuildDesc {
		GeometryTriDesc* m_triDesc;
		GeeometryAABBDesc* m_aabbDesc;
		GeometryOMMTriDesc* m_ommTriDesc;

		unsigned int m_structCount = 0;

		RtBuildFlags m_buildFlags = RtBuildFlags::None;
		RtAccelStructType m_type = RtAccelStructType::BottomLevel;
	};

}