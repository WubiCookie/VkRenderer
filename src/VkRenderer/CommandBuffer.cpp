#include "CommandBuffer.hpp"

#include <stdexcept>

CommandBuffer::CommandBuffer(const VulkanDevice& device_,
                             VkCommandPool parentCommandPool,
                             VkCommandBufferLevel level)
    : VulkanDeviceObject(device_),
      m_parentCommandPool(parentCommandPool)
{
	using namespace cdm;

	auto& vk = this->device();

	if (m_parentCommandPool.get() == nullptr)
	{
		throw std::runtime_error("error: parentCommandPool is nullptr");
	}

	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.commandPool = m_parentCommandPool.get();
	allocInfo.level = level;
	allocInfo.commandBufferCount = 1;

	if (vk.allocate(allocInfo, &m_commandBuffer.get()) != VK_SUCCESS)
	{
		throw std::runtime_error("error: failed to allocate command buffers");
	}
}

CommandBuffer::CommandBuffer(CommandBuffer&& cb) noexcept
    : VulkanDeviceObject(std::move(cb)),
      m_parentCommandPool(std::exchange(cb.m_parentCommandPool, nullptr)),
      m_commandBuffer(std::exchange(cb.m_commandBuffer, nullptr))
{
}

CommandBuffer::~CommandBuffer()
{
	auto& vk = this->device();

	if (m_parentCommandPool.get())
	{
		vk.free(m_parentCommandPool.get(), m_commandBuffer.get());
	}
}

CommandBuffer& CommandBuffer::operator=(CommandBuffer&& cb) noexcept
{
	VulkanDeviceObject::operator=(std::move(cb));

	m_parentCommandPool = std::exchange(cb.m_parentCommandPool, nullptr);
	m_commandBuffer = std::exchange(cb.m_commandBuffer, nullptr);

	return *this;
}

VkCommandBuffer CommandBuffer::commandBuffer()
{
	return m_commandBuffer.get();
}

bool CommandBuffer::outdated() const { return false; }

void CommandBuffer::recreate() {}
