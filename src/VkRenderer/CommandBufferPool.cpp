#include "CommandBufferPool.hpp"
#include "RenderWindow.hpp"

namespace cdm
{
VkResult FrameCommandBuffer::wait(uint64_t timeout)
{
	const auto& vk = commandBuffer.device();
	return vk.wait(fence, timeout);
}

void FrameCommandBuffer::reset()
{
	const auto& vk = commandBuffer.device();

	commandBuffer.reset();
	vk.resetFence(fence);
	submitted = false;
}

bool FrameCommandBuffer::isAvailable()
{
	const auto& vk = commandBuffer.device();

	return submitted == false ||
	       (submitted == true && vk.getFenceStatus(fence) == VK_SUCCESS);
}

VkResult FrameCommandBuffer::submit(VkQueue queue)
{
	const auto& vk = commandBuffer.device();

	VkResult res = vk.queueSubmit(queue, commandBuffer, fence);
	submitted = true;

	return res;
}

CommandBufferPool::CommandBufferPool(const VulkanDevice& vulkanDevice,
                                     VkCommandPoolCreateFlags flags)
    : m_device(vulkanDevice)
{
	const auto& vk = device();

	QueueFamilyIndices queueFamilyIndices = vk.queueFamilyIndices();

	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolInfo.flags = flags;

	if (vk.create(poolInfo, m_commandPool) != VK_SUCCESS)
		throw std::runtime_error("error: failed to create command pool");

	vk.debugMarkerSetObjectName(m_commandPool,
	                            "CommandBufferPool::commandPool");
}

CommandBufferPool::~CommandBufferPool()
{
	const auto& vk = device();

	for (auto& frame : m_frameCommandBuffers)
		if (frame.submitted)
			vk.wait(frame.fence);
}

FrameCommandBuffer& CommandBufferPool::getAvailableCommandBuffer()
{
	const auto& vk = device();

	int outCommandBufferIndex = 0;
	for (auto& frame : m_frameCommandBuffers)
	{
		if (frame.isAvailable())
			return frame;

		/// AAAAAAAAAAAAH
		outCommandBufferIndex++;
		if (outCommandBufferIndex >= 256)
			abort();
	}

	m_frameCommandBuffers.push_back(
	    FrameCommandBuffer{ CommandBuffer(vk, commandPool()), vk.createFence(),
	                        // vk.createFence(VK_FENCE_CREATE_SIGNALED_BIT),
	                        vk.createSemaphore() });

#pragma region marker names
	vk.debugMarkerSetObjectName(
	    m_frameCommandBuffers.front().commandBuffer.get(),
	    "CommandBufferPool::frameCommandBuffers[" +
	        std::to_string(outCommandBufferIndex) + "].commandBuffer");
	vk.debugMarkerSetObjectName(m_frameCommandBuffers.front().fence.get(),
	                            "CommandBufferPool::frameCommandBuffers[" +
	                                std::to_string(outCommandBufferIndex) +
	                                "].fence");
	vk.debugMarkerSetObjectName(m_frameCommandBuffers.front().semaphore.get(),
	                            "CommandBufferPool::frameCommandBuffers[" +
	                                std::to_string(outCommandBufferIndex) +
	                                "].semaphore");
#pragma endregion

	return m_frameCommandBuffers.back();
}

void CommandBufferPool::waitForAllCommandBuffers()
{
	const auto& vk = device();

	for (auto& frame : m_frameCommandBuffers)
	{
		if (frame.submitted)
		{
			vk.wait(frame.fence);
			frame.reset();
		}
	}
}

void CommandBufferPool::reset()
{
	const auto& vk = device();
	waitForAllCommandBuffers();
	vk.resetCommandPool(m_commandPool, VkCommandPoolResetFlags());
}
}  // namespace cdm
