#pragma once
#include <vector>
#include "Engine/Renderer/GraphicsCommon.hpp"

struct ShaderDesc {
	char const* m_path = nullptr;
	char const* m_name = nullptr;
	ShaderType m_type = InvalidShader;
};

struct Shader {
	char const* m_name = nullptr;
	char const* m_path = nullptr;
	ShaderType m_type = ShaderType::InvalidShader;
	std::vector<uint8_t> m_byteCode;

};