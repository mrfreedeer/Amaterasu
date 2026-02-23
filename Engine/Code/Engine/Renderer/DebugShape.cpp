#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/DebugShape.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Billboard.hpp"
#include "Engine/Math/Mat44.hpp"


bool DebugShape::CanShapeBeDeleted()
{
	if(m_info.m_duration == -1.0f) return false;
	return m_info.m_stopwach.HasDurationElapsed();
}

void DebugShape::StartWatch(Clock const& clock)
{
	m_info.m_stopwach.Start(&clock, m_info.m_duration);
}

Mat44 const DebugShape::GetBillboardModelMatrix(Camera const& camera) const
{
	Vec3 position = m_info.m_modelMatrix.GetTranslation3D();
	Mat44 billboardMat = Billboard::GetModelMatrixForBillboard(position, camera, BillboardType::CameraFacingXYZ);
	return billboardMat;
}

Rgba8 const DebugShape::GetModelColor() const
{
	if(m_info.m_stopwach.m_duration == 0.0) return m_info.m_startColor;
	return Rgba8::InterpolateColors(m_info.m_startColor, m_info.m_endColor, m_info.m_stopwach.GetElapsedFraction());
}

bool DebugShape::IsTextType() const
{
	unsigned int mask = (DebugShapeFlagsBit::DebugScreenShapeText | DebugShapeFlagsBit::DebugWorldShapeText);

	return m_info.m_flags & mask;
}
