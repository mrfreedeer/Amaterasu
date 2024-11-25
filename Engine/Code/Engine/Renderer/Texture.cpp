#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Interfaces/Resource.hpp"

Texture::Texture(TextureCreateInfo const& createInfo):
	m_info(createInfo)
{

}

Texture::~Texture()
{

}

ResourceView* Texture::GetRenderTargetView() const
{
	return m_rsc->GetRenderTargetView();
}

ResourceView* Texture::GetDepthStencilView() const
{
	return m_rsc->GetDepthStencilView();
}

ResourceView* Texture::GetShaderResourceView() const
{
	return m_rsc->GetShaderResourceView();
}

