#include "Material.hpp"
#include "MyShaderWriter.hpp"
#include "CommandBuffer.hpp"
#include "RenderPass.hpp"
#include "RenderWindow.hpp"
#include "UniformBuffer.hpp"

#include <array>
#include <iostream>

namespace cdm
{
MaterialInstance::MaterialInstance(Material& material, uint32_t instanceOffset) : m_material(material), m_instanceOffset(instanceOffset)
{
}

	float    MaterialInstance::floatParameter(const std::string& name) { return material().floatParameter(name, m_instanceOffset); }
	vector2  MaterialInstance::vec2Parameter (const std::string& name) { return material().vec2Parameter (name, m_instanceOffset); }
	vector3  MaterialInstance::vec3Parameter (const std::string& name) { return material().vec3Parameter (name, m_instanceOffset); }
	vector4  MaterialInstance::vec4Parameter (const std::string& name) { return material().vec4Parameter (name, m_instanceOffset); }
	matrix2  MaterialInstance::mat2Parameter (const std::string& name) { return material().mat2Parameter (name, m_instanceOffset); }
	matrix3  MaterialInstance::mat3Parameter (const std::string& name) { return material().mat3Parameter (name, m_instanceOffset); }
	matrix4  MaterialInstance::mat4Parameter (const std::string& name) { return material().mat4Parameter (name, m_instanceOffset); }
	uint32_t MaterialInstance::uintParameter (const std::string& name) { return material().uintParameter (name, m_instanceOffset); }
	int32_t  MaterialInstance::intParameter  (const std::string& name) { return material().intParameter  (name, m_instanceOffset); }

	void MaterialInstance::setFloatParameter(const std::string& name, float a)         { material().setFloatParameter(name, m_instanceOffset, a); }
	void MaterialInstance::setVec2Parameter(const std::string& name, const vector2& a) { material().setVec2Parameter(name, m_instanceOffset, a); }
	void MaterialInstance::setVec3Parameter(const std::string& name, const vector3& a) { material().setVec3Parameter(name, m_instanceOffset, a); }
	void MaterialInstance::setVec4Parameter(const std::string& name, const vector4& a) { material().setVec4Parameter(name, m_instanceOffset, a); }
	void MaterialInstance::setMat2Parameter(const std::string& name, const matrix2& a) { material().setMat2Parameter(name, m_instanceOffset, a); }
	void MaterialInstance::setMat3Parameter(const std::string& name, const matrix3& a) { material().setMat3Parameter(name, m_instanceOffset, a); }
	void MaterialInstance::setMat4Parameter(const std::string& name, const matrix4& a) { material().setMat4Parameter(name, m_instanceOffset, a); }
	void MaterialInstance::setUintParameter(const std::string& name, uint32_t a)       { material().setUintParameter(name, m_instanceOffset, a); }
	void MaterialInstance::setIntParameter(const std::string& name, int32_t a)         { material().setIntParameter(name, m_instanceOffset, a); }
	void MaterialInstance::setTextureParameter(const std::string& name, Texture2D& a)  { material().setTextureParameter(name, m_instanceOffset, a); }

//float MaterialInstance::floatParameter(const std::string& name)
//{
//	auto found = m_floatParameters.find(name);
//	if (found != m_floatParameters.end())
//		return found->second.value;
//
//	throw std::runtime_error("could not find float parameter " + name);
//}
//
//vector2 MaterialInstance::vec2Parameter(const std::string& name)
//{
//	auto found = m_vec2Parameters.find(name);
//	if (found != m_vec2Parameters.end())
//		return found->second.value;
//
//	throw std::runtime_error("could not find vec2 parameter " + name);
//}
//
//vector3 MaterialInstance::vec3Parameter(const std::string& name)
//{
//	auto found = m_vec3Parameters.find(name);
//	if (found != m_vec3Parameters.end())
//		return found->second.value;
//
//	throw std::runtime_error("could not find vec3 parameter " + name);
//}
//
//vector4 MaterialInstance::vec4Parameter(const std::string& name)
//{
//	auto found = m_vec4Parameters.find(name);
//	if (found != m_vec4Parameters.end())
//		return found->second.value;
//
//	throw std::runtime_error("could not find vec4 parameter " + name);
//}
//
//matrix2 MaterialInstance::mat2Parameter(const std::string& name)
//{
//	auto found = m_mat2Parameters.find(name);
//	if (found != m_mat2Parameters.end())
//		return found->second.value;
//
//	throw std::runtime_error("could not find mat2 parameter " + name);
//}
//
//matrix3 MaterialInstance::mat3Parameter(const std::string& name)
//{
//	auto found = m_mat3Parameters.find(name);
//	if (found != m_mat3Parameters.end())
//		return found->second.value;
//
//	throw std::runtime_error("could not find mat3 parameter " + name);
//}
//
//matrix4 MaterialInstance::mat4Parameter(const std::string& name)
//{
//	auto found = m_mat4Parameters.find(name);
//	if (found != m_mat4Parameters.end())
//		return found->second.value;
//
//	throw std::runtime_error("could not find mat4 parameter " + name);
//}
//
//uint32_t MaterialInstance::uintParameter(const std::string& name)
//{
//	auto found = m_uintParameters.find(name);
//	if (found != m_uintParameters.end())
//		return found->second.value;
//
//	throw std::runtime_error("could not find uint parameter " + name);
//}
//
//int32_t MaterialInstance::intParameter(const std::string& name)
//{
//	auto found = m_intParameters.find(name);
//	if (found != m_intParameters.end())
//		return found->second.value;
//
//	throw std::runtime_error("could not find int parameter " + name);
//}
//
//void MaterialInstance::setFloatParameter(const std::string& name, float a)
//{
//	auto found = m_floatParameters.find(name);
//	if (found != m_floatParameters.end())
//	{
//		found->second.value = a;
//		std::byte* ptr = found->second.uniformBuffer.get()->map<std::byte>();
//		std::memcpy(ptr + found->second.offset * m_instanceOffset.get(), &a,
//		            sizeof(a));
//		found->second.uniformBuffer.get()->unmap();
//	}
//}
//
//void MaterialInstance::setVec2Parameter(const std::string& name,
//                                        const vector2& a)
//{
//	auto found = m_vec2Parameters.find(name);
//	if (found != m_vec2Parameters.end())
//	{
//		found->second.value = a;
//		std::byte* ptr = found->second.uniformBuffer.get()->map<std::byte>();
//		std::memcpy(ptr + found->second.offset * m_instanceOffset.get(), &a,
//		            sizeof(a));
//		found->second.uniformBuffer.get()->unmap();
//	}
//}
//
//void MaterialInstance::setVec3Parameter(const std::string& name,
//                                        const vector3& a)
//{
//	auto found = m_vec3Parameters.find(name);
//	if (found != m_vec3Parameters.end())
//	{
//		found->second.value = a;
//		std::byte* ptr = found->second.uniformBuffer.get()->map<std::byte>();
//		std::memcpy(ptr + found->second.offset * m_instanceOffset.get(), &a,
//		            sizeof(a));
//		found->second.uniformBuffer.get()->unmap();
//	}
//}
//
//void MaterialInstance::setVec4Parameter(const std::string& name,
//                                        const vector4& a)
//{
//	auto found = m_vec4Parameters.find(name);
//	if (found != m_vec4Parameters.end())
//	{
//		found->second.value = a;
//		std::byte* ptr = found->second.uniformBuffer.get()->map<std::byte>();
//		std::memcpy(ptr + found->second.offset * m_instanceOffset.get(), &a,
//		            sizeof(a));
//		found->second.uniformBuffer.get()->unmap();
//	}
//}
//
//void MaterialInstance::setMat2Parameter(const std::string& name,
//                                        const matrix2& a)
//{
//	auto found = m_mat2Parameters.find(name);
//	if (found != m_mat2Parameters.end())
//	{
//		found->second.value = a;
//		std::byte* ptr = found->second.uniformBuffer.get()->map<std::byte>();
//		std::memcpy(ptr + found->second.offset * m_instanceOffset.get(), &a,
//		            sizeof(a));
//		found->second.uniformBuffer.get()->unmap();
//	}
//}
//
//void MaterialInstance::setMat3Parameter(const std::string& name,
//                                        const matrix3& a)
//{
//	auto found = m_mat3Parameters.find(name);
//	if (found != m_mat3Parameters.end())
//	{
//		found->second.value = a;
//		std::byte* ptr = found->second.uniformBuffer.get()->map<std::byte>();
//		std::memcpy(ptr + found->second.offset * m_instanceOffset.get(), &a,
//		            sizeof(a));
//		found->second.uniformBuffer.get()->unmap();
//	}
//}
//
//void MaterialInstance::setMat4Parameter(const std::string& name,
//                                        const matrix4& a)
//{
//	auto found = m_mat4Parameters.find(name);
//	if (found != m_mat4Parameters.end())
//	{
//		found->second.value = a;
//		std::byte* ptr = found->second.uniformBuffer.get()->map<std::byte>();
//		std::memcpy(ptr + found->second.offset * m_instanceOffset.get(), &a,
//		            sizeof(a));
//		found->second.uniformBuffer.get()->unmap();
//	}
//}
//
//void MaterialInstance::setUintParameter(const std::string& name, uint32_t a)
//{
//	auto found = m_uintParameters.find(name);
//	if (found != m_uintParameters.end())
//	{
//		found->second.value = a;
//		std::byte* ptr = found->second.uniformBuffer.get()->map<std::byte>();
//		std::memcpy(ptr + found->second.offset * m_instanceOffset.get(), &a,
//		            sizeof(a));
//		found->second.uniformBuffer.get()->unmap();
//	}
//}
//
//void MaterialInstance::setIntParameter(const std::string& name, int32_t a)
//{
//	auto found = m_intParameters.find(name);
//	if (found != m_intParameters.end())
//	{
//		found->second.value = a;
//		std::byte* ptr = found->second.uniformBuffer.get()->map<std::byte>();
//		std::memcpy(ptr + found->second.offset * m_instanceOffset.get(), &a,
//		            sizeof(a));
//		found->second.uniformBuffer.get()->unmap();
//	}
//}

void MaterialInstance::bind(CommandBuffer& cb, VkPipelineLayout layout)
{
	m_material.get().bind(cb, layout);
}

void MaterialInstance::pushOffset(CommandBuffer& cb, VkPipelineLayout layout)
{
	uint32_t offset = m_instanceOffset;
	cb.pushConstants(layout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, &offset);
}

Material::Material(PbrShadingModel& shadingModel,
                   uint32_t instancePoolSize)
    : m_shadingModel(&shadingModel),
      m_instancePoolSize(instancePoolSize)
{
	m_instances.reserve(m_instancePoolSize);
	/// TODO: resize descriptor pool
}

std::unique_ptr<Material::VertexShaderBuildDataBase>
Material::instantiateVertexShaderBuildData()
{
	return std::make_unique<VertexShaderBuildDataBase>();
}

std::unique_ptr<Material::FragmentShaderBuildDataBase>
Material::instantiateFragmentShaderBuildData()
{
	return std::make_unique<FragmentShaderBuildDataBase>();
}

MaterialInstance* Material::instanciate()
{
	if (m_instances.size() >= m_instancePoolSize)
		return nullptr;

	m_instances.emplace_back(
	    MaterialInstance(*this, uint32_t(m_instances.size() + 1)));
	//m_instances.back().m_floatParameters = m_floatParameters;
	//m_instances.back().m_vec2Parameters = m_vec2Parameters;
	//m_instances.back().m_vec3Parameters = m_vec3Parameters;
	//m_instances.back().m_vec4Parameters = m_vec4Parameters;
	//m_instances.back().m_mat2Parameters = m_mat2Parameters;
	//m_instances.back().m_mat3Parameters = m_mat3Parameters;
	//m_instances.back().m_mat4Parameters = m_mat4Parameters;
	//m_instances.back().m_uintParameters = m_uintParameters;
	//m_instances.back().m_intParameters = m_intParameters;
	//m_instances.back().m_instanceOffset = m_instances.size() + 1;

	return &m_instances.back();
}

//float Material::floatParameter(const std::string& name)
//{
//	auto found = m_floatParameters.find(name);
//	if (found != m_floatParameters.end())
//		return found->second.value;
//
//	throw std::runtime_error("could not find float parameter " + name);
//}
//
//vector2 Material::vec2Parameter(const std::string& name)
//{
//	auto found = m_vec2Parameters.find(name);
//	if (found != m_vec2Parameters.end())
//		return found->second.value;
//
//	throw std::runtime_error("could not find vec2 parameter " + name);
//}
//
//vector3 Material::vec3Parameter(const std::string& name)
//{
//	auto found = m_vec3Parameters.find(name);
//	if (found != m_vec3Parameters.end())
//		return found->second.value;
//
//	throw std::runtime_error("could not find vec3 parameter " + name);
//}
//
//vector4 Material::vec4Parameter(const std::string& name)
//{
//	auto found = m_vec4Parameters.find(name);
//	if (found != m_vec4Parameters.end())
//		return found->second.value;
//
//	throw std::runtime_error("could not find vec4 parameter " + name);
//}
//
//matrix2 Material::mat2Parameter(const std::string& name)
//{
//	auto found = m_mat2Parameters.find(name);
//	if (found != m_mat2Parameters.end())
//		return found->second.value;
//
//	throw std::runtime_error("could not find mat2 parameter " + name);
//}
//
//matrix3 Material::mat3Parameter(const std::string& name)
//{
//	auto found = m_mat3Parameters.find(name);
//	if (found != m_mat3Parameters.end())
//		return found->second.value;
//
//	throw std::runtime_error("could not find mat3 parameter " + name);
//}
//
//matrix4 Material::mat4Parameter(const std::string& name)
//{
//	auto found = m_mat4Parameters.find(name);
//	if (found != m_mat4Parameters.end())
//		return found->second.value;
//
//	throw std::runtime_error("could not find mat4 parameter " + name);
//}
//
//uint32_t Material::uintParameter(const std::string& name)
//{
//	auto found = m_uintParameters.find(name);
//	if (found != m_uintParameters.end())
//		return found->second.value;
//
//	throw std::runtime_error("could not find uint parameter " + name);
//}
//
//int32_t Material::intParameter(const std::string& name)
//{
//	auto found = m_intParameters.find(name);
//	if (found != m_intParameters.end())
//		return found->second.value;
//
//	throw std::runtime_error("could not find int parameter " + name);
//}
//
//void Material::setFloatParameter(const std::string& name, float a)
//{
//	auto found = m_floatParameters.find(name);
//	if (found != m_floatParameters.end())
//	{
//		found->second.value = a;
//		std::byte* ptr = found->second.uniformBuffer.get()->map<std::byte>();
//		std::memcpy(ptr + found->second.offset, &a, sizeof(a));
//		found->second.uniformBuffer.get()->unmap();
//	}
//}
//
//void Material::setVec2Parameter(const std::string& name, const vector2& a)
//{
//	auto found = m_vec2Parameters.find(name);
//	if (found != m_vec2Parameters.end())
//	{
//		found->second.value = a;
//		std::byte* ptr = found->second.uniformBuffer.get()->map<std::byte>();
//		std::memcpy(ptr + found->second.offset, &a, sizeof(a));
//		found->second.uniformBuffer.get()->unmap();
//	}
//}
//
//void Material::setVec3Parameter(const std::string& name, const vector3& a)
//{
//	auto found = m_vec3Parameters.find(name);
//	if (found != m_vec3Parameters.end())
//	{
//		found->second.value = a;
//		std::byte* ptr = found->second.uniformBuffer.get()->map<std::byte>();
//		std::memcpy(ptr + found->second.offset, &a, sizeof(a));
//		found->second.uniformBuffer.get()->unmap();
//	}
//}
//
//void Material::setVec4Parameter(const std::string& name, const vector4& a)
//{
//	auto found = m_vec4Parameters.find(name);
//	if (found != m_vec4Parameters.end())
//	{
//		found->second.value = a;
//		std::byte* ptr = found->second.uniformBuffer.get()->map<std::byte>();
//		std::memcpy(ptr + found->second.offset, &a, sizeof(a));
//		found->second.uniformBuffer.get()->unmap();
//	}
//}
//
//void Material::setMat2Parameter(const std::string& name, const matrix2& a)
//{
//	auto found = m_mat2Parameters.find(name);
//	if (found != m_mat2Parameters.end())
//	{
//		found->second.value = a;
//		std::byte* ptr = found->second.uniformBuffer.get()->map<std::byte>();
//		std::memcpy(ptr + found->second.offset, &a, sizeof(a));
//		found->second.uniformBuffer.get()->unmap();
//	}
//}
//
//void Material::setMat3Parameter(const std::string& name, const matrix3& a)
//{
//	auto found = m_mat3Parameters.find(name);
//	if (found != m_mat3Parameters.end())
//	{
//		found->second.value = a;
//		std::byte* ptr = found->second.uniformBuffer.get()->map<std::byte>();
//		std::memcpy(ptr + found->second.offset, &a, sizeof(a));
//		found->second.uniformBuffer.get()->unmap();
//	}
//}
//
//void Material::setMat4Parameter(const std::string& name, const matrix4& a)
//{
//	auto found = m_mat4Parameters.find(name);
//	if (found != m_mat4Parameters.end())
//	{
//		found->second.value = a;
//		std::byte* ptr = found->second.uniformBuffer.get()->map<std::byte>();
//		std::memcpy(ptr + found->second.offset, &a, sizeof(a));
//		found->second.uniformBuffer.get()->unmap();
//	}
//}
//
//void Material::setUintParameter(const std::string& name, uint32_t a)
//{
//	auto found = m_uintParameters.find(name);
//	if (found != m_uintParameters.end())
//	{
//		found->second.value = a;
//		std::byte* ptr = found->second.uniformBuffer.get()->map<std::byte>();
//		std::memcpy(ptr + found->second.offset, &a, sizeof(a));
//		found->second.uniformBuffer.get()->unmap();
//	}
//}
//
//void Material::setIntParameter(const std::string& name, int32_t a)
//{
//	auto found = m_intParameters.find(name);
//	if (found != m_intParameters.end())
//	{
//		found->second.value = a;
//		std::byte* ptr = found->second.uniformBuffer.get()->map<std::byte>();
//		std::memcpy(ptr + found->second.offset, &a, sizeof(a));
//		found->second.uniformBuffer.get()->unmap();
//	}
//}

void Material::bind(CommandBuffer& cb, VkPipelineLayout layout)
{
	cb.bindDescriptorSet(VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS,
	                     layout, 0, m_descriptorSet);
}

void Material::pushOffset(CommandBuffer& cb, VkPipelineLayout layout)
{
	uint32_t offset = 0;
	cb.pushConstants(layout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, &offset);
}
}  // namespace cdm
