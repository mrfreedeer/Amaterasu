#include "Engine/Renderer/Texture.hpp"

Texture::Texture(TextureDesc const& createInfo):
	Resource(createInfo.m_name.c_str()),
	m_info(createInfo)
{

}

Texture::~Texture()
{

}

