#include "Buffer.hpp"

#include "RenderWindow.hpp"

#include <stdexcept>

namespace cdm
{
Buffer::Buffer(RenderWindow& renderWindow, VkDeviceSize bufferSize,
               VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
               VkMemoryPropertyFlags requiredFlags)
    : rw(&renderWindow)
{
	auto& vk = rw.get()->device();

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

Buffer::Buffer(Buffer&& buffer) noexcept
{
	rw = std::exchange(buffer.rw, nullptr);

	m_allocation = std::exchange(buffer.m_allocation, nullptr);
	m_buffer = std::exchange(buffer.m_buffer, nullptr);

	m_allocInfo = std::exchange(buffer.m_allocInfo, VmaAllocationInfo{});
}

Buffer::~Buffer()
{
	if (rw.get())
	{
		auto& vk = rw.get()->device();

		if (m_buffer)
			vmaDestroyBuffer(vk.allocator(), m_buffer.get(),
			                 m_allocation.get());
	}
}

Buffer& Buffer::operator=(Buffer&& buffer) noexcept
{
	rw = std::exchange(buffer.rw, nullptr);

	m_allocation = std::exchange(buffer.m_allocation, nullptr);
	m_buffer = std::exchange(buffer.m_buffer, nullptr);

	m_allocInfo = std::exchange(buffer.m_allocInfo, VmaAllocationInfo{});

	return *this;
}

void* Buffer::map()
{
	auto& vk = rw.get()->device();
	void* data;
	vmaMapMemory(vk.allocator(), m_allocation.get(), &data);
	return data;
}

void Buffer::unmap()
{
	auto& vk = rw.get()->device();
	vmaUnmapMemory(vk.allocator(), m_allocation.get());
}
}  // namespace cdm
