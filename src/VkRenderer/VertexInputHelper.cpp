#include "VertexInputHelper.hpp"

namespace cdm
{
void VertexInputHelper::addInputBinding(
    const VkVertexInputBindingDescription& binding)
{
	for (const auto& b : bindings)
	{
		if (b.binding == binding.binding)
			return;
	}

	bindings.push_back(binding);
}

void VertexInputHelper::addInputBinding(uint32_t binding, uint32_t stride,
                                        VkVertexInputRate rate)
{
	VkVertexInputBindingDescription d{};
	d.binding = binding;
	d.stride = stride;
	d.inputRate = rate;

	addInputBinding(d);
}

void VertexInputHelper::addInputAttribute(
    const VkVertexInputAttributeDescription& attribute)
{
	attributes.push_back(attribute);
}

void VertexInputHelper::addInputAttribute(uint32_t location, uint32_t binding,
                                          VkFormat format, uint32_t offset)
{
	VkVertexInputAttributeDescription a{};
	a.location = location;
	a.binding = binding;
	a.format = format;
	a.offset = offset;

	addInputAttribute(a);
}

void VertexInputHelper::addInputAttribute(uint32_t location, uint32_t binding,
                                          VkFormat format)
{
	uint32_t offset = 0;

	for (const auto& a : attributes)
	{
		if (a.binding == binding && a.location < location)
		{
			if (a.format == VK_FORMAT_R32_SFLOAT)
				offset += sizeof(float) * 1;
			else if (a.format == VK_FORMAT_R32G32_SFLOAT)
				offset += sizeof(float) * 2;
			else if (a.format == VK_FORMAT_R32G32B32_SFLOAT)
				offset += sizeof(float) * 3;
			else if (a.format == VK_FORMAT_R32G32B32A32_SFLOAT)
				offset += sizeof(float) * 4;
			else
				abort();
		}
	}

	addInputAttribute(location, binding, format, offset);
}

vk::PipelineVertexInputStateCreateInfo VertexInputHelper::getCreateInfo()
{
	for (auto& binding : bindings)
	{
		binding.stride = 0;
		for (const auto& attribute : attributes)
		{
			if (binding.binding == attribute.binding)
			{
				if (attribute.format == VK_FORMAT_R32_SFLOAT)
					binding.stride += sizeof(float) * 1;
				else if (attribute.format == VK_FORMAT_R32G32_SFLOAT)
					binding.stride += sizeof(float) * 2;
				else if (attribute.format == VK_FORMAT_R32G32B32_SFLOAT)
					binding.stride += sizeof(float) * 3;
				else if (attribute.format == VK_FORMAT_R32G32B32A32_SFLOAT)
					binding.stride += sizeof(float) * 4;
				else
					abort();
			}
		}
	}

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.vertexBindingDescriptionCount = uint32_t(bindings.size());
	vertexInputInfo.pVertexBindingDescriptions =
	    bindings.size() ? bindings.data() : nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount =
	    uint32_t(attributes.size());
	vertexInputInfo.pVertexAttributeDescriptions =
	    attributes.size() ? attributes.data() : nullptr;

	return vertexInputInfo;
}
}  // namespace cdm
