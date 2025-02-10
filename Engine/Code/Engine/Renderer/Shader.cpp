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
