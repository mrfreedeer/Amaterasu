#pragma once
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"
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

struct TextureCreateInfo {
	std::string m_name = "Unnamed Texture";
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


class Texture {
	friend class Renderer;
public:
	ResourceView* GetRenderTargetView() const;
	ResourceView* GetDepthStencilView() const;
	ResourceView* GetShaderResourceView() const;
	TextureFormat GetFormat() const { return m_info.m_format; }
	bool IsRenderTargetCompatible() const { return m_info.m_bindFlags & RESOURCE_BIND_RENDER_TARGET_BIT; }
	bool IsShaderResourceCompatible() const { return m_info.m_bindFlags & RESOURCE_BIND_SHADER_RESOURCE_BIT; }
	bool IsDepthStencilCompatible() const { return m_info.m_bindFlags & RESOURCE_BIND_DEPTH_STENCIL_BIT; }
	bool IsUnorderedAccessCompatible() const { return m_info.m_bindFlags & RESOURCE_BIND_UNORDERED_ACCESS_VIEW_BIT; }
private:
	Texture();
	Texture(TextureCreateInfo const& createInfo);
	~Texture();
private:
	Resource* m_rsc = nullptr;
	TextureCreateInfo m_info = {};

};