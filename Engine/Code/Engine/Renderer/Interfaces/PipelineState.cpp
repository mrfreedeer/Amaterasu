#include "Engine/Renderer/Interfaces/PipelineState.hpp"
#include <d3d12.h>

PipelineState::~PipelineState()
{
	DX_SAFE_RELEASE(m_pso);
}

