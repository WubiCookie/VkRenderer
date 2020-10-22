#ifndef VKRENDERER_COMMAND_BUFFER_POOL_HPP
#define VKRENDERER_COMMAND_BUFFER_POOL_HPP

#include "CommandBuffer.hpp"
#include "VulkanDevice.hpp"

#include <vector>

namespace cdm
{
struct FrameCommandBuffer
{
	CommandBuffer commandBuffer;
	UniqueFence fence;
	UniqueSemaphore semaphore;
	bool submitted = false;

	VkResult wait(uint64_t timeout = UINT64_MAX);
	void reset();
	bool isAvailable();
	VkResult submit(VkQueue queue);
};

class CommandBufferPool final
{
	std::reference_wrapper<const VulkanDevice> m_device;
	UniqueCommandPool m_commandPool;

	std::vector<FrameCommandBuffer> m_frameCommandBuffers;

public:
	CommandBufferPool(const VulkanDevice& vulkanDevice,
	    VkCommandPoolCreateFlags flags = VkCommandPoolCreateFlags());
	CommandBufferPool(const CommandBufferPool&) = default;
	CommandBufferPool(CommandBufferPool&&) = default;
	~CommandBufferPool();

	CommandBufferPool& operator=(const CommandBufferPool&) = default;
	CommandBufferPool& operator=(CommandBufferPool&&) = default;

	const VkCommandPool& commandPool() const { return m_commandPool.get(); }
	const VulkanDevice& device() const { return m_device.get(); }

	FrameCommandBuffer& getAvailableCommandBuffer();
	void waitForAllCommandBuffers();

	void reset();
};
}  // namespace cdm

#endif  // VKRENDERER_COMMAND_BUFFER_POOL_HPP