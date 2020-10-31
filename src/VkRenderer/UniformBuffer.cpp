#include "UniformBuffer.hpp"

namespace cdm
{
UniformBuffer::UniformBuffer(const VulkanDevice& vulkanDevice,
                             VkDeviceSize bufferSize, VkBufferUsageFlags usage,
                             VmaMemoryUsage memoryUsage,
                             VkMemoryPropertyFlags requiredFlags,
                             UboBuilder uboBuilder)
    : Buffer(vulkanDevice, bufferSize,
             usage | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, memoryUsage,
             requiredFlags),
      m_uboBuilder(uboBuilder)
{
}

sdw::Ubo UniformBuffer::buildUbo(sdw::ShaderWriter& writer, uint32_t binding,
                                  uint32_t set) const
{
	return m_uboBuilder(writer, binding, set);
}
}  // namespace cdm
