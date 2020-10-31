#pragma once

#include "Buffer.hpp"
#include "MyShaderWriter.hpp"

#include <functional>

namespace cdm
{
// sdw::Ubo uboBuilder(sdw::ShaderWriter& writer,
//                     uint32_t binding,
//                     uint32_t set);
using UboBuilder =
    std::function<sdw::Ubo(sdw::ShaderWriter&, uint32_t, uint32_t)>;

class UniformBuffer : public Buffer
{
	UboBuilder m_uboBuilder;

public:
	UniformBuffer() = default;
	UniformBuffer(const VulkanDevice& vulkanDevice, VkDeviceSize bufferSize,
	              VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
	              VkMemoryPropertyFlags requiredFlags, UboBuilder uboBuilder);
	UniformBuffer(const UniformBuffer&) = delete;
	UniformBuffer(UniformBuffer&& uniformBuffer) = default;
	~UniformBuffer() = default;

	sdw::Ubo buildUbo(sdw::ShaderWriter& writer, uint32_t binding,
	                   uint32_t set) const;

	UniformBuffer& operator=(const UniformBuffer&) = delete;
	UniformBuffer& operator=(UniformBuffer&& uniformBuffer) = default;
};
}  // namespace cdm
