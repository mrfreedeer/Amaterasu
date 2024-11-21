#include "Engine/Renderer/Interfaces/Resource.hpp"

ResourceView* Resource::GetDepthStencilView()
{
	if (!m_dsv) ERROR_RECOVERABLE("REQUESTING NULL DSV");

	return nullptr;
}

void Resource::Map(void*& mapSource, size_t beginRange, size_t endRange)
{
	D3D12_RANGE range = { beginRange, endRange };
	HRESULT mapResult = m_rsc->Map(0, &range, &mapSource);
}

void Resource::Unmap()
{
	m_rsc->Unmap(0, nullptr);
}

Resource::~Resource()
{
	DX_SAFE_RELEASE(m_rsc);
}
