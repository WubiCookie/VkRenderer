#pragma once

#include "cdm_vulkan.hpp"

#include <vector>

namespace cdm
{
class VertexInputHelper
{
public:
	std::vector<VkVertexInputBindingDescription> bindings;
	std::vector<VkVertexInputAttributeDescription> attributes;

	VertexInputHelper() = default;
	~VertexInputHelper() = default;

	VertexInputHelper(const VertexInputHelper&) = default;
	VertexInputHelper(VertexInputHelper&&) = default;

	VertexInputHelper& operator=(const VertexInputHelper&) = default;
	VertexInputHelper& operator=(VertexInputHelper&&) = default;

	void addInputBinding(const VkVertexInputBindingDescription& binding);
	void addInputBinding(uint32_t binding, uint32_t stride,
	                     VkVertexInputRate rate =
	                         VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX);

	void addInputAttribute(const VkVertexInputAttributeDescription& attribute);
	void addInputAttribute(uint32_t location, uint32_t binding,
	                       VkFormat format, uint32_t offset);
	void addInputAttribute(uint32_t location, uint32_t binding,
	                       VkFormat format);

	vk::PipelineVertexInputStateCreateInfo getCreateInfo();
};
}  // namespace cdm
