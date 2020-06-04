#pragma once

//#include "Material.hpp"
#include "VulkanDevice.hpp"

#include <vector>

namespace cdm
{
struct VertexInputState
{
	std::vector<VkVertexInputBindingDescription> bindings;
	std::vector<VkVertexInputAttributeDescription> attributes;
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
};

struct DescriptorPoolSizesInfo
{
	std::vector<VkDescriptorPoolSize> sizes;
	vk::DescriptorPoolCreateInfo poolInfo;
};

struct WriteBufferDescriptorSet
{
	VkDescriptorBufferInfo setBufferInfo{};
	vk::WriteDescriptorSet write;
};
}  // namespace cdm
