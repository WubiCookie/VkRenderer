#ifndef VKRENDERER_CAMERA_HPP
#define VKRENDERER_CAMERA_HPP 1

#include "cdm_maths.hpp"

namespace cdm
{
class Camera
{
public:
	perspective m_perspective;
	unscaled_transform3d m_transform;

	matrix4 view() const;
	matrix4 proj() const;
	matrix4 viewproj() const;
};
}  // namespace cdm

#endif  // VKRENDERER_CAMERA_HPP
