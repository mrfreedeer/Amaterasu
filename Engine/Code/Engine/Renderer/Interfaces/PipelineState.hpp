#pragma once
#include "Engine/Renderer/GraphicsCommon.hpp"

struct ID3D12PipelineState;
struct Shader;

struct PipelineStateDesc {
	PipelineType m_type = PipelineType::Graphics;
	char const* m_debugName = nullptr;
	bool m_depthEnable = false;
	bool m_stencilEnable = false;
	unsigned int m_renderTargetCount = 0;
	unsigned int m_sampleCount = 1;
	FillMode m_fillMode = FillMode::SOLID;
	CullMode m_cullMode = CullMode::NONE;
	WindingOrder m_windingOrder = WindingOrder::COUNTERCLOCKWISE;
	DepthFunc m_depthFunc = DepthFunc::ALWAYS;
	TopologyType m_topology = TopologyType::TOPOLOGY_TYPE_TRIANGLE;
	BlendMode m_blendModes[8] = { BlendMode::OPAQUE };
	TextureFormat m_renderTargetFormats[8] = { TextureFormat::R8G8B8A8_UNORM };
	TextureFormat m_depthStencilFormat = TextureFormat::D24_UNORM_S8_UINT;
	Shader* m_byteCodes[ShaderType::NUM_SHADER_TYPES] = { nullptr };
};

struct PipelineState {
	PipelineState(ID3D12PipelineState* pso) : m_pso(pso) {}
	~PipelineState();

	PipelineStateDesc m_desc = {};
	ID3D12PipelineState* m_pso = nullptr;
};