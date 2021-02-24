#include "Camera.hpp"

namespace cdm
{
matrix4 Camera::view() const { return matrix4(m_transform.get_inversed()); }

matrix4 Camera::proj() const { return matrix4(m_perspective); }

matrix4 Camera::viewproj() const
{
	return m_perspective * m_transform.get_inversed();
}
}  // namespace cdm
