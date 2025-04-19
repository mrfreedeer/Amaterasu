#include "Engine/Renderer/Interfaces/Resource.hpp"

void Resource::Map(void*& mapSource, size_t beginRange, size_t endRange)
{
	D3D12_RANGE range = { beginRange, endRange };
	HRESULT mapResult = m_rawRsc->Map(0, &range, &mapSource);
	ThrowIfFailed(mapResult, "FAILED TO MAP RESOURCE");
}

void Resource::Unmap()
{
	m_rawRsc->Unmap(0, nullptr);
}

TransitionBarrier Resource::GetTransitionBarrier(ResourceStates afterState)
{
	TransitionBarrier outBarrier = {};

	outBarrier.m_before = (ResourceStates)m_currentState;
	outBarrier.m_after = afterState;
	outBarrier.m_rsc = this;

	return outBarrier;
}

Resource::~Resource()
{
	DX_SAFE_RELEASE(m_rawRsc);
}
