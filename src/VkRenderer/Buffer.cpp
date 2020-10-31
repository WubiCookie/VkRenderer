#include "Buffer.hpp"

#include "RenderWindow.hpp"

#include <stdexcept>

namespace cdm
{
Buffer::Buffer(const VulkanDevice& vulkanDevice, VkDeviceSize bufferSize,
               VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
               VkMemoryPropertyFlags requiredFlags)
    : m_vulkanDevice(&vulkanDevice)
{
	auto& vk = *m_vulkanDevice.get();

	vk::BufferCreateInfo info;
	info.size = bufferSize;
	info.usage = usage;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = memoryUsage;
	// allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	allocCreateInfo.requiredFlags = requiredFlags;

	vmaCreateBuffer(vk.allocator(), &info, &allocCreateInfo, &m_buffer.get(),
	                &m_allocation.get(), &m_allocInfo);

	if (m_buffer == false)
		throw std::runtime_error("could not create buffer");
}

Buffer::~Buffer()
{
	if (m_vulkanDevice)
	{
		auto& vk = *m_vulkanDevice.get();

		if (m_buffer)
			vmaDestroyBuffer(vk.allocator(), m_buffer.get(),
			                 m_allocation.get());
	}
}

void Buffer::setName(std::string_view name)
{
	if (get())
	{
		auto& vk = *m_vulkanDevice.get();
		vk.debugMarkerSetObjectName(
		    get(), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, name);
	}
}

void* Buffer::map()
{
	auto& vk = *m_vulkanDevice.get();
	void* data;
	vmaMapMemory(vk.allocator(), m_allocation.get(), &data);
	return data;
}

void Buffer::unmap()
{
	auto& vk = *m_vulkanDevice.get();
	vmaUnmapMemory(vk.allocator(), m_allocation.get());
}

void Buffer::upload(const void* data, size_t size)
{
	/// TODO: check buffer size
	std::memcpy(map(), data, size);
	unmap();
}
}  // namespace cdm
