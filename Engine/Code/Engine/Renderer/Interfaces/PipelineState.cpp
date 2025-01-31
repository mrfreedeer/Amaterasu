#include "Engine/Renderer/Interfaces/PipelineState.hpp"
#include "Engine/Renderer/GraphicsCommon.hpp"

PipelineState::~PipelineState()
{
	DX_SAFE_RELEASE(m_pso);
}

