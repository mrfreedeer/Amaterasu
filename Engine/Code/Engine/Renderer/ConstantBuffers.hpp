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
	Mat44 ModelMatrix;
	Rgba8 ModelColor;
	float ModelPadding[4];
};