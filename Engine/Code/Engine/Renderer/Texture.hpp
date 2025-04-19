#pragma once
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"
#include "Engine/Renderer/Interfaces/Resource.hpp"
#include <vector>
#include <string>

class Resource;
struct ResourceView;
class Renderer;

enum ResourceBindFlagBit : unsigned int {
	RESOURCE_BIND_NONE = 0,
	RESOURCE_BIND_SHADER_RESOURCE_BIT = (1 << 0),
	RESOURCE_BIND_RENDER_TARGET_BIT = (1 << 1),
	RESOURCE_BIND_DEPTH_STENCIL_BIT = (1 << 2),
	RESOURCE_BIND_UNORDERED_ACCESS_VIEW_BIT = (1 << 3),
	RESOURCE_BIND_CONSTANT_BUFFER_VIEW_BIT = (1 << 4),
	RESOURCE_BIND_ALL_BUFFER_VIEWS = (0b1001), // SRV AND UAV
	RESOURCE_BIND_ALL_TEXTURE_VIEWS = (0b1011), // SRV, RTV AND UAV
};

typedef unsigned int ResourceBindFlag;

struct TextureDesc {
	std::string m_name = "Unnamed Texture";
	char const* m_source = "";
	Renderer* m_owner = nullptr;
	Resource* m_handle = nullptr;
	void* m_initialData = nullptr;
	ResourceBindFlag m_bindFlags = RESOURCE_BIND_NONE;
	bool m_isMultiSample = false;
	size_t m_stride = 0;
	IntVec2 m_dimensions = IntVec2::ZERO;
	TextureFormat m_format = TextureFormat::R8G8B8A8_UNORM;
	TextureFormat m_clearFormat = TextureFormat::R8G8B8A8_UNORM;
	Rgba8 m_clearColour = Rgba8();

};


class Texture : public Resource {
	friend class Renderer;
public:
	TextureFormat GetFormat() const { return m_info.m_format; }
	TextureFormat GetClearFormat() const { return m_info.m_clearFormat; }
	bool IsRenderTargetCompatible() const { return m_info.m_bindFlags & RESOURCE_BIND_RENDER_TARGET_BIT; }
	bool IsShaderResourceCompatible() const { return m_info.m_bindFlags & RESOURCE_BIND_SHADER_RESOURCE_BIT; }
	bool IsDepthStencilCompatible() const { return m_info.m_bindFlags & RESOURCE_BIND_DEPTH_STENCIL_BIT; }
	bool IsUnorderedAccessCompatible() const { return m_info.m_bindFlags & RESOURCE_BIND_UNORDERED_ACCESS_VIEW_BIT; }
	char const* GetSource() const { return m_info.m_source; }
	IntVec2 GetDimensions() const { return m_info.m_dimensions; }
	char const* GetDebugName() const { return m_info.m_name.c_str(); }
private:
	Texture(): Resource("Unnamed Texture"){};
	Texture(TextureDesc const& createInfo);
	~Texture();
private:
	Resource* m_uploadRsc = nullptr;
	TextureDesc m_info = {};

};

struct Sampler {
	unsigned int m_descriptor = 0;
};