#pragma once

#include "VulkanDevice.hpp"

#include "cdm_vulkan.hpp"

#include <array>

namespace cdm
{
class CommandPool final : public VulkanDeviceObject
{
	Movable<VkCommandPool> m_commandPool;

public:
	CommandPool(const VulkanDevice& device, uint32_t queueFamilyIndex,
	            VkCommandPoolCreateFlags flags = 0);
	CommandPool(const CommandPool&) = delete;
	CommandPool(CommandPool&& cb) = default;
	~CommandPool();

	CommandPool& operator=(const CommandPool&) = delete;
	CommandPool& operator=(CommandPool&& cb) = default;

	VkCommandPool& get();
	const VkCommandPool& get() const;
	VkCommandPool& commandPool() { return get(); }
	const VkCommandPool& commandPool() const { return get(); }
	operator VkCommandPool&() { return commandPool(); }
	operator const VkCommandPool&() const { return commandPool(); }
};
}  // namespace cdm
