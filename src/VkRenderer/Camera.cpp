#include "Camera.hpp"

namespace cdm
{
matrix4 Camera::view() const { return m_transform.get_inversed(); }

matrix4 Camera::proj() const { return m_perspective; }

matrix4 Camera::viewproj() const
{
	return m_perspective * m_transform.get_inversed();
}
}  // namespace cdm
