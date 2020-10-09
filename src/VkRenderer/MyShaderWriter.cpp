#include "MyShaderWriter.hpp"

namespace cdm
{
Ubo::Ubo(VertexWriter& writer, std::string const& name, uint32_t bind,
         uint32_t set, ast::type::MemoryLayout layout)
    : sdw::Ubo(writer, name, bind, set)
{
	VkDescriptorSetLayoutBinding b{};
	b.binding = bind;
	b.descriptorCount = 1;
	b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	writer.m_descriptors.push_back({ set, b });
}

Ubo::Ubo(FragmentWriter& writer, std::string const& name, uint32_t bind,
         uint32_t set, ast::type::MemoryLayout layout)
    : sdw::Ubo(writer, name, bind, set)
{
	VkDescriptorSetLayoutBinding b{};
	b.binding = bind;
	b.descriptorCount = 1;
	b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	writer.m_descriptors.push_back({ set, b });
}

Ubo::Ubo(ComputeWriter& writer, std::string const& name, uint32_t bind,
         uint32_t set, ast::type::MemoryLayout layout)
    : sdw::Ubo(writer, name, bind, set)
{
	VkDescriptorSetLayoutBinding b{};
	b.binding = bind;
	b.descriptorCount = 1;
	b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	writer.m_descriptors.push_back({ set, b });
}

UniqueShaderModule VertexWriter::createShaderModule(
    const VulkanDevice& vk) const
{
	std::vector<uint32_t> bytecode = spirv::serialiseSpirv(getShader());

	vk::ShaderModuleCreateInfo createInfo;
	createInfo.codeSize = bytecode.size() * sizeof(*bytecode.data());
	createInfo.pCode = bytecode.data();

	return vk.create(createInfo);
}

VertexShaderHelperResult VertexWriter::createHelperResult(
    const VulkanDevice& vk) const
{
	return { m_vertexInputHelper, m_descriptors, createShaderModule(vk) };
}

UniqueShaderModule FragmentWriter::createShaderModule(
    const VulkanDevice& vk) const
{
	std::vector<uint32_t> bytecode = spirv::serialiseSpirv(getShader());

	vk::ShaderModuleCreateInfo createInfo;
	createInfo.codeSize = bytecode.size() * sizeof(*bytecode.data());
	createInfo.pCode = bytecode.data();

	return vk.create(createInfo);
}

FragmentShaderHelperResult FragmentWriter::createHelperResult(
    const VulkanDevice& vk) const
{
	return { m_outputAttachments, m_descriptors, createShaderModule(vk) };
}

void ComputeWriter::addDescriptor(uint32_t binding, uint32_t set, VkDescriptorType type)
{
	VkDescriptorSetLayoutBinding b{};
	b.binding = binding;
	b.descriptorCount = 1;
	b.descriptorType = type;
	b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	m_descriptors.push_back({ set, b });
}

UniqueShaderModule ComputeWriter::createShaderModule(
    const VulkanDevice& vk) const
{
	std::vector<uint32_t> bytecode = spirv::serialiseSpirv(getShader());

	vk::ShaderModuleCreateInfo createInfo;
	createInfo.codeSize = bytecode.size() * sizeof(*bytecode.data());
	createInfo.pCode = bytecode.data();

	return vk.create(createInfo);
}

ComputeShaderHelperResult ComputeWriter::createHelperResult(
    const VulkanDevice& vk) const
{
	return { m_descriptors, createShaderModule(vk) };
}
}  // namespace cdm
