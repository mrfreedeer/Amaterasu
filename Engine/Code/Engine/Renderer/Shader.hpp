#pragma once
#include <vector>
#include "Engine/Renderer/GraphicsCommon.hpp"

struct ShaderLoadInfo {
	char const* m_path = nullptr;
	char const* m_name = nullptr;
};

struct ShaderDesc : public ShaderLoadInfo {
	char const* m_entryPoint = nullptr;
	ShaderType m_type = InvalidShader;
};

/// <summary>
/// For this load struct, the path will be a path to parent folder
/// </summary>
struct ShaderPipelineDesc : public ShaderLoadInfo {
	char const* m_firstEntryPoint = nullptr;
	char const* m_secEntryPoint = nullptr;
	ShaderType m_firstShaderType = InvalidShader;
	// Second is always assumed to be a pixel shader
	
};

struct Shader {
	char const* m_name = nullptr;
	char const* m_path = nullptr;
	char const* m_entryPoint = nullptr;
	ShaderType m_type = ShaderType::InvalidShader;
	std::vector<uint8_t> m_byteCode;
	std::vector<uint8_t> m_reflection;
	std::vector<uint8_t> m_rootSignature;

	static char const* GetDefaultEntryPoint(ShaderType shaderType);
	static wchar_t const* GetTarget(ShaderType shaderType);
};

struct ShaderPipeline {
	Shader* m_firstShader = nullptr;
	Shader* m_pixelShader = nullptr;
};