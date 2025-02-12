#include "Engine/Renderer/Shader.hpp"

char const* Shader::GetDefaultEntryPoint(ShaderType shaderType)
{
	switch (shaderType)
	{
	case Vertex:	return "VertexMain";
	case Pixel:		return "PixelMain";
	case Geometry:	return "GeometryMain";
	case Mesh:		return "MeshMain";
	case Hull:		return "HullMain";
	case Domain:	return "DomainMain";
	case Compute:	return "ComputeMain";
	case NUM_SHADER_TYPES:
	default:
	case InvalidShader:
		ERROR_AND_DIE("NO KNOW ENTRYPOINT");
		return nullptr;
	}
}

wchar_t const* Shader::GetTarget(ShaderType shaderType)
{
	switch (shaderType)
	{
	case Vertex:	return L"vs_6_0";
	case Pixel:		return L"ps_6_0";
	case Geometry:	return L"gs_6_0";
	case Mesh:		return L"gs_6_0";
	case Hull:		return L"hs_6_0";
	case Domain:	return L"ds_6_0";
	case Compute:	return L"cs_6_0";
	case NUM_SHADER_TYPES:
	default:
	case InvalidShader:
		ERROR_AND_DIE("NO KNOW ENTRYPOINT");
		return nullptr;
	}
}
