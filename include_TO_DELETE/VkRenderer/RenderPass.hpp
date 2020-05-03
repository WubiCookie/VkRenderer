#pragma once

#include "VulkanDevice.hpp"

namespace cdm
{
class RenderPass final : public VulkanDeviceObject
{
	Moveable<VkRenderPass> m_renderPass;

public:
	RenderPass(const VulkanDevice& device_, VkFormat format);
	~RenderPass();

	VkRenderPass renderPass();
	operator VkRenderPass();
};
}  // namespace cdm
