#pragma once

#include "VulkanDevice.hpp"

namespace cdm
{
class RenderWindow;

class Buffer final
{
	Moveable<RenderWindow*> rw;

	Moveable<VmaAllocation> m_allocation;
	Moveable<VkBuffer> m_buffer;

	VmaAllocationInfo m_allocInfo{};

public:
	Buffer() = default;
	Buffer(RenderWindow& renderWindow, VkDeviceSize bufferSize,
	       VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
	       VkMemoryPropertyFlags requiredFlags);
	Buffer(const Buffer&) = delete;
	Buffer(Buffer&& buffer) noexcept;
	~Buffer();

	Buffer& operator=(const Buffer&) = delete;
	Buffer& operator=(Buffer&& buffer) noexcept;

	VkDeviceSize size() const { return m_allocInfo.size; }
	VkDeviceSize offset() const { return m_allocInfo.offset; }
	VkDeviceMemory deviceMemory() const
	{
		return m_allocInfo.deviceMemory;
	}
	const VkBuffer& get() const { return m_buffer.get(); }
	const VmaAllocation& allocation() const
	{
		return m_allocation.get();
	}

	void* map();
	template<typename T>
	T* map() { return static_cast<T*>(map()); }
	void unmap();
};
}  // namespace cdm
