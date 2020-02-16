#pragma once

#include "VulkanDevice.hpp"

class RenderPass final : public VulkanDeviceObject
{
	cdm::Moveable<VkRenderPass> m_renderPass;

public:
	RenderPass(const VulkanDevice& device_, VkFormat format);
	~RenderPass();

	VkRenderPass renderPass();
	operator VkRenderPass();
};
