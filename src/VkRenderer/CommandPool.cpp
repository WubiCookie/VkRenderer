#include "CommandPool.hpp"

#include <stdexcept>

namespace cdm
{
CommandPool::CommandPool(const VulkanDevice& device_,
                         uint32_t queueFamilyIndex,
                         VkCommandPoolCreateFlags flags)
    : VulkanDeviceObject(device_)
{
	const auto& vk = this->device();

	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.queueFamilyIndex = queueFamilyIndex;
	poolInfo.flags = flags;

	if (vk.create(poolInfo, m_commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("error: failed to create command pool");
	}
}

CommandPool::~CommandPool()
{
	const auto& vk = device();

	vk.destroy(m_commandPool.get());
}

VkCommandPool& CommandPool::get() { return m_commandPool.get(); }

const VkCommandPool& CommandPool::get() const { return m_commandPool.get(); }
}  // namespace cdm
