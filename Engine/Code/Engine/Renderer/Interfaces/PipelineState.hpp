#pragma once
#include <d3d12.h>

struct PipelineStateDesc {

};

struct PipelineState {
	~PipelineState();
	ID3D12PipelineState* m_pso = nullptr;
};