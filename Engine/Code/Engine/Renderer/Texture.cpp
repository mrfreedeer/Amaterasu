#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Interfaces/Resource.hpp"

Texture::Texture(TextureDesc const& createInfo):
	m_info(createInfo)
{

}

Texture::~Texture()
{

}

ResourceView* Texture::GetRenderTargetView() const
{
	ResourceView* rtView = m_rsc->GetRenderTargetView();
	return (rtView && rtView->m_valid) ? rtView : nullptr;
}

ResourceView* Texture::GetDepthStencilView() const
{
	ResourceView* dsvView = m_rsc->GetDepthStencilView();
	return (dsvView && dsvView->m_valid) ? dsvView : nullptr;
}

ResourceView* Texture::GetShaderResourceView() const
{
	ResourceView* srvView = m_rsc->GetShaderResourceView();
	return(srvView && srvView->m_valid) ? srvView : nullptr;
}
