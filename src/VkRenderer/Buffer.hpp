#pragma once

#include "VulkanDevice.hpp"

#include <vector>

namespace cdm
{
class RenderWindow;

class Buffer
{
	Movable<RenderWindow*> rw;

	Movable<VmaAllocation> m_allocation;
	Movable<VkBuffer> m_buffer;

	VmaAllocationInfo m_allocInfo{};

public:
	Buffer() = default;
	Buffer(RenderWindow& renderWindow, VkDeviceSize bufferSize,
	       VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
	       VkMemoryPropertyFlags requiredFlags);
	Buffer(const Buffer&) = delete;
	Buffer(Buffer&& buffer) = default;
	~Buffer();

	Buffer& operator=(const Buffer&) = delete;
	Buffer& operator=(Buffer&& buffer) = default;

	VkDeviceSize size() const { return m_allocInfo.size; }
	VkDeviceSize offset() const { return m_allocInfo.offset; }
	VkDeviceMemory deviceMemory() const { return m_allocInfo.deviceMemory; }
	const VkBuffer& get() const { return m_buffer.get(); }
	const VmaAllocation& allocation() const { return m_allocation.get(); }

	void setName(std::string_view name);

	void* map();
	template <typename T>
	T* map()
	{
		return static_cast<T*>(map());
	}
	void unmap();

	void upload(const void* data, size_t size);
	template <typename T>
	void upload(const T* data, size_t count)
	{
		upload(static_cast<const void*>(data), count * sizeof(T));
	}
	template <typename T>
	void upload(const T& data)
	{
		upload(&data, sizeof(T));
	}

	template <typename T = uint8_t>
	std::vector<T> download()
	{
		std::vector<T> res(size() / sizeof(T));
		T* ptr = map<T>();
		std::memcpy(res.data(), ptr, size());
		unmap();
		return res;
	}

	operator const VkBuffer&() const noexcept { return get(); }
};
}  // namespace cdm
