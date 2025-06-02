#pragma once
#include "Engine/Math/Mat44.hpp"
#include "Engine/Core/Rgba8.hpp"
// File where we can find all GPU related Structs that are provided by the engine


// Helper C++ side struct for creating constant buffers
struct CameraConstants
{
	Mat44 ProjectionMatrix;
	Mat44 ViewMatrix;
};

struct ModelConstants
{
	Mat44 ModelMatrix = Mat44();
	float ModelColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float ModelPadding[4];
};

struct DrawInfoConstants
{
	unsigned int CameraBufferInd;
	unsigned int ModelBufferInd;
	unsigned int TextureStart;
};