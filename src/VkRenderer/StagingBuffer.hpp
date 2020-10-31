#pragma once

#include "VulkanDevice.hpp"

#include "Buffer.hpp"

namespace cdm
{
class StagingBuffer final : public Buffer
{
public:
	StagingBuffer() = default;
	StagingBuffer(const VulkanDevice& vulkanDevice, VkDeviceSize dataSize)
	    : Buffer(vulkanDevice, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	             VMA_MEMORY_USAGE_CPU_TO_GPU,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	{
	}
	StagingBuffer(const VulkanDevice& vulkanDevice, const void* data,
	              VkDeviceSize dataSize)
	    : Buffer(vulkanDevice, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	             VMA_MEMORY_USAGE_CPU_TO_GPU,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	{
		upload(data, dataSize);
	}
	template <typename T>
	StagingBuffer(const VulkanDevice& vulkanDevice, const T* data,
	              VkDeviceSize count)
	    : StagingBuffer(vulkanDevice, static_cast<const void*>(data),
	                    count * sizeof(T))
	{
	}
	template <typename T>
	StagingBuffer(const VulkanDevice& vulkanDevice, const T& data)
	    : StagingBuffer(vulkanDevice, &data, sizeof(T))
	{
	}
	StagingBuffer(const StagingBuffer&) = delete;
	StagingBuffer(StagingBuffer&& StagingBuffer) = default;
	~StagingBuffer() = default;

	StagingBuffer& operator=(const StagingBuffer&) = delete;
	StagingBuffer& operator=(StagingBuffer&& StagingBuffer) = default;
};
}  // namespace cdm
