#include "CommandBufferPool.hpp"
#include "RenderWindow.hpp"

namespace cdm
{
VkResult FrameCommandBuffer::wait(uint64_t timeout)
{
	const auto& vk = commandBuffer.device();
	return vk.wait(fence, timeout);
}

bool FrameCommandBuffer::isAvailable()
{
	const auto& vk = commandBuffer.device();

	return available == true && submitted == false;
}

VkResult FrameCommandBuffer::submit(VkQueue queue)
{
	const auto& vk = commandBuffer.device();

	VkResult res = vk.queueSubmit(queue, commandBuffer, fence);
	submitted = true;
	available = false;

	return res;
}

// ======================================================================

VkResult ResettableFrameCommandBuffer::wait(uint64_t timeout)
{
	const auto& vk = commandBuffer.device();
	return vk.wait(fence, timeout);
}

void ResettableFrameCommandBuffer::reset()
{
	const auto& vk = commandBuffer.device();

	commandBuffer.reset();
	vk.resetFence(fence);
	submitted = false;
}

bool ResettableFrameCommandBuffer::isAvailable()
{
	const auto& vk = commandBuffer.device();

	if (vk.getFenceStatus(fence) == VK_SUCCESS)
	{
		commandBuffer.reset();
		vk.resetFence(fence);
		submitted = false;
	}

	return submitted == false;
}

VkResult ResettableFrameCommandBuffer::submit(VkQueue queue)
{
	const auto& vk = commandBuffer.device();

	VkResult res = vk.queueSubmit(queue, commandBuffer, fence);
	submitted = true;

	return res;
}

// ======================================================================

CommandBufferPool::CommandBufferPool(const VulkanDevice& vulkanDevice,
                                     VkCommandPoolCreateFlags flags)
    : m_device(vulkanDevice),
      m_flags(flags)
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

	uint32_t outCommandBufferIndex = 0;
	for (auto& frame : m_frameCommandBuffers)
	{
		if (frame.isAvailable())
			return frame;

		outCommandBufferIndex++;
		/// AAAAAAAAAAAAH
		// if (outCommandBufferIndex >= 256)
		//  abort();
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

	std::vector<VkFence> fences;
	fences.reserve(m_frameCommandBuffers.size());

	for (auto& frame : m_frameCommandBuffers)
		if (frame.submitted)
			fences.push_back(frame.fence);

	if (fences.empty() == false)
		vk.wait(uint32_t(fences.size()), fences.data(), true);
	// vk.wait();

	for (auto& frame : m_frameCommandBuffers)
		frame.submitted = false;

	m_registeredStagingBuffer.clear();
}

void CommandBufferPool::reset()
{
	const auto& vk = device();
	waitForAllCommandBuffers();
	vk.resetCommandPool(m_commandPool, VkCommandPoolResetFlags());

	for (auto& frame : m_frameCommandBuffers)
	{
		if (vk.getFenceStatus(frame.fence) == VK_SUCCESS)
			vk.resetFence(frame.fence);
		frame.submitted = false;
	}
}

void CommandBufferPool::registerResource(StagingBuffer stagingBuffer)
{
	m_registeredStagingBuffer.emplace_back(std::move(stagingBuffer));
}

// ======================================================================

ResettableCommandBufferPool::ResettableCommandBufferPool(
    const VulkanDevice& vulkanDevice, VkCommandPoolCreateFlags flags)
    : m_device(vulkanDevice),
      m_flags(flags | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
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

ResettableCommandBufferPool::~ResettableCommandBufferPool()
{
	const auto& vk = device();

	for (auto& frame : m_frameCommandBuffers)
		if (frame.submitted)
			vk.wait(frame.fence);
}

ResettableFrameCommandBuffer&
ResettableCommandBufferPool::getAvailableCommandBuffer()
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

	m_frameCommandBuffers.push_back(ResettableFrameCommandBuffer{
	    CommandBuffer(vk, commandPool()), vk.createFence(),
	    // vk.createFence(VK_FENCE_CREATE_SIGNALED_BIT),
	    vk.createSemaphore() });

#pragma region marker names
	vk.debugMarkerSetObjectName(
	    m_frameCommandBuffers.front().commandBuffer.get(),
	    "ResettableCommandBufferPool::frameCommandBuffers[" +
	        std::to_string(outCommandBufferIndex) + "].commandBuffer");
	vk.debugMarkerSetObjectName(
	    m_frameCommandBuffers.front().fence.get(),
	    "ResettableCommandBufferPool::frameCommandBuffers[" +
	        std::to_string(outCommandBufferIndex) + "].fence");
	vk.debugMarkerSetObjectName(
	    m_frameCommandBuffers.front().semaphore.get(),
	    "ResettableCommandBufferPool::frameCommandBuffers[" +
	        std::to_string(outCommandBufferIndex) + "].semaphore");
#pragma endregion

	return m_frameCommandBuffers.back();
}

void ResettableCommandBufferPool::waitForAllCommandBuffers()
{
	const auto& vk = device();

	// std::vector<VkFence> fences;
	// fences.reserve(m_frameCommandBuffers.size());

	for (auto& frame : m_frameCommandBuffers)
	{
		if (frame.submitted)
		{
			vk.wait(frame.fence);

			if ((m_flags & VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) ==
			    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
				frame.reset();
			else
			{
				vk.resetFence(frame.fence);
				frame.submitted = false;
			}
		}
	}
}

void ResettableCommandBufferPool::reset()
{
	const auto& vk = device();
	waitForAllCommandBuffers();
	vk.resetCommandPool(m_commandPool, VkCommandPoolResetFlags());

	for (auto& frame : m_frameCommandBuffers)
		frame.reset();
}
}  // namespace cdm
