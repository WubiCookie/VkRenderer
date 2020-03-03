#pragma once

#include "VulkanDevice.hpp"

#include <vector>

namespace cdm
{
class RenderWindow;
class RenderPass;
class ImageView;

class Framebuffer final : public VulkanDeviceObject
{
	std::reference_wrapper<RenderWindow> rw;

	std::reference_wrapper<RenderPass> m_renderPass;
	std::vector<std::reference_wrapper<ImageView>> m_imageViews;

	Moveable<VkFramebuffer> m_framebuffer;

public:
	Framebuffer(RenderWindow& renderWindow, RenderPass& renderPass,
	            std::vector<std::reference_wrapper<ImageView>> imageViews);
	~Framebuffer();

	VkFramebuffer framebuffer();
	operator VkFramebuffer();

private:
	bool outdated() const;
	void recreate();
};
}  // namespace cdm
