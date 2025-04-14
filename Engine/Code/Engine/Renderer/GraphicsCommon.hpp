#pragma  once
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/Mat44.hpp"
#include <wrl.h>

#undef OPAQUE

#define DX_SAFE_RELEASE(dxObject)			\
{											\
	if (( dxObject) != nullptr)				\
	{										\
		(dxObject)->Release();				\
		(dxObject) = nullptr;				\
	}										\
}

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

inline void ThrowIfFailed(long hr, char const* errorMsg) {
	if (hr < 0) {
		ERROR_AND_DIE(errorMsg);
	}
}

// Default shaders offered by the Engine
enum EngineShaderPipelines {
	LegacyForward,
	NUM_ENGINE_SHADERS
};


enum ShaderType : unsigned int {
	InvalidShader = UINT_MAX,
	Vertex = 0,
	Pixel,
	Geometry,
	Mesh,
	Hull,
	Domain,
	Compute,
	NUM_SHADER_TYPES
};
static const char* ShaderTypeStrings[] = { "VS", "PS", "GS", "MS", "HS", "DS", "CS"};



enum class PipelineType {
	Graphics,
	Mesh,
	Compute
};


enum class TextureFormat : int {
	INVALID = -1,
	R8G8B8A8_UNORM,
	R32G32B32A32_FLOAT,
	R32G32_FLOAT,
	D24_UNORM_S8_UINT,
	R24G8_TYPELESS,
	R32_FLOAT,
	D32_FLOAT,
	UNKNOWN
};

static const char* BlendModeStrings[] = {"ALPHA", "ADDITIVE", "OPAQUE"};
enum class BlendMode
{
	ALPHA = 0,
	ADDITIVE,
	OPAQUE,
	NUM_BLEND_MODES
};

static const char* CullModeStrings[] = { "NONE", "FRONT", "BACK" };
enum class CullMode {
	NONE = 0,
	FRONT,
	BACK,
	NUM_CULL_MODES
};

static const char* FillModeStrings[] = { "SOLID", "WIREFRAME" };
enum class FillMode {
	SOLID = 0,
	WIREFRAME,
	NUM_FILL_MODES
};

static const char* WindingOrderStrings[] = { "CLOCKWISE", "COUNTERCLOCKWISE" };
enum class WindingOrder {
	CLOCKWISE = 0,
	COUNTERCLOCKWISE,
	NUM_WINDING_ORDERS
};


static const char* DepthFuncStrings[] = { "NEVER", "LESS", "EQUAL", "LESSEQUAL", "GREATER", "NOTEQUAL", "GREATEREQUAL", "ALWAYS"};
enum class DepthFunc // Transformed directly to DX11 (if standard changes, unexpected behavior might result) check when changing to > DX11
{
	NEVER = 0,
	LESS,
	EQUAL,
	LESSEQUAL,
	GREATER,
	NOTEQUAL,
	GREATEREQUAL,
	ALWAYS,
	NUM_DEPTH_TESTS
};

static const char* SamplerModeStrings[] = { "POINTCLAMP", "POINTWRAP", "BILINEARCLAMP", "BILINEARWRAP", "SHADOWMAPS" };
enum class SamplerMode
{
	POINTCLAMP,
	POINTWRAP,
	BILINEARCLAMP,
	BILINEARWRAP,
	SHADOWMAPS,
};

static const char* TopologyTypeStrings[] = { "UNDEFINED", "POINT", "LINE", "TRIANGLE", "PATCH" };
enum class TopologyType {// Transformed directly to DX12 (if standard changes, unexpected behavior might result) check when changing to > DX12
	TOPOLOGY_TYPE_UNDEFINED = 0,
	TOPOLOGY_TYPE_POINT = 1,
	TOPOLOGY_TYPE_LINE = 2,
	TOPOLOGY_TYPE_TRIANGLE = 3,
	TOPOLOGY_TYPE_PATCH = 4,
	NUM_TOPOLOGIES
};

enum class CommandListType {
	DIRECT = 0,
	BUNDLE = 1,
	COMPUTE = 2,
	COPY = 3,
	VIDEO_DECODE = 4,
	VIDEO_PROCESS = 5,
	VIDEO_ENCODE = 6,
	NUM_COMMAND_LIST_TYPES
};

enum class MemoryUsage {
	Default,		// Buffer made to read from multiple times but CPU cannot write to. 
	Dynamic,		// Buffer that can be read/write from both GPU and CPU. 
	Readback		// Buffer that can be read back from GPU
};

enum class DescriptorHeapFlags : uint8_t {
	None = 0,
	ShaderVisible
};

enum class DescriptorHeapType : uint8_t {
	CBV_SRV_UAV = 0,
	Sampler,
	RenderTargetView,
	DepthStencilView,
	NUM_TYPES,
	UNDEFINED = 255,
};

// ENUM for getting descriptor table param index
// Unbounded model buffers
// Unbounded camera buffers
// Game CBuffers
// Unbounded textures
// Game UAVs
// There might be max a sampler per RT (I've only used 1 ever)
enum RootParameterIndex : unsigned int {
	PARAM_MODEL_BUFFERS = 0,
	PARAM_CAMERA_BUFFERS,
	PARAM_GAME_BUFFERS,
	PARAM_TEXTURES,
	PARAM_GAME_UAVS,
	PARAM_SAMPLERS

};

static const char* DesciptorHeapStrings[] = { "CBV_SRV_UAV", "Sampler", "RTV", "DSV"};

constexpr char const* EnumToString(BlendMode blendMode) {
	return BlendModeStrings[(int)blendMode];
}

constexpr char const* EnumToString(CullMode cullMode) {
	return CullModeStrings[(int)cullMode];
}

constexpr char const* EnumToString(FillMode fillMode) {
	return FillModeStrings[(int)fillMode];
}

constexpr char const* EnumToString(WindingOrder windingOrder) {
	return WindingOrderStrings[(int)windingOrder];
}

constexpr char const* EnumToString(DepthFunc depthFunc) {
	return DepthFuncStrings[(int)depthFunc];
}

constexpr char const* EnumToString(SamplerMode samplerMode) {
	return SamplerModeStrings[(int)samplerMode];
}

constexpr char const* EnumToString(TopologyType topologyType) {
	return TopologyTypeStrings[(int)topologyType];
}

constexpr char const* EnumToString(ShaderType shaderType) {
	return ShaderTypeStrings[shaderType];
}
constexpr char const* EnumToString(DescriptorHeapType heapType) {
	return DesciptorHeapStrings[(int)heapType];
}