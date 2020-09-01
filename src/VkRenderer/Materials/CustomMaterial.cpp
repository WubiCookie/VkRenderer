#include "CustomMaterial.hpp"
#include "RenderWindow.hpp"

#include <iostream>
#include <stdexcept>

namespace cdm
{
struct CustomMaterialUBOStruct : public sdw::StructInstance
{
	CustomMaterialUBOStruct(ast::Shader* shader, ast::expr::ExprPtr expr)
	    : StructInstance(shader, std::move(expr)),
	      color(getMember<sdw::Vec4>("color")),
	      metalness(getMember<sdw::Float>("metalness")),
	      roughness(getMember<sdw::Float>("roughness"))
	{
	}
	CustomMaterialUBOStruct& operator=(CustomMaterialUBOStruct const& rhs)
	{
		StructInstance::operator=(rhs);
		return *this;
	}

	static ast::type::StructPtr makeType(ast::type::TypesCache& cache)
	{
		auto result = std::make_unique<ast::type::Struct>(
		    cache, ast::type::MemoryLayout::eStd140,
		    "CustomMaterialUBOStruct");

		if (result->empty())
		{
			result->declMember("color", ast::type::Kind::eVec4F);
			result->declMember("metalness", ast::type::Kind::eFloat);
			result->declMember("roughness", ast::type::Kind::eFloat);
		}

		return result;
	}

	// static std::unique_ptr<sdw::Struct> declare(sdw::ShaderWriter& writer)
	//{
	//	return std::make_unique<sdw::Struct>(writer,
	//	                                     makeType(writer.getTypesCache()));
	//}

	sdw::Vec4 color;
	sdw::Float metalness;
	sdw::Float roughness;

private:
	using sdw::StructInstance::getMember;
	using sdw::StructInstance::getMemberArray;
};
}  // namespace cdm

namespace sdw
{
template <>
struct TypeTraits<cdm::CustomMaterialUBOStruct>
{
	static ast::type::Kind constexpr TypeEnum =
	    ast::type::Kind(int(ast::type::Kind::eCount) + 1);
};
}  // namespace sdw

namespace cdm
{
CustomMaterial::CustomMaterial(RenderWindow& renderWindow,
                               PbrShadingModel& shadingModel,
                               uint32_t instancePoolSize)
    : Material(renderWindow, shadingModel, instancePoolSize)
{
	auto& vk = renderWindow.device();

	m_albedoTexture.resize(instancePoolSize + 1);
	m_normalTexture.resize(instancePoolSize + 1);
	m_metalnessTexture.resize(instancePoolSize + 1);
	m_roughnessTexture.resize(instancePoolSize + 1);

	// m_uniformBuffer = Buffer(
	//    renderWindow, sizeof(UBOStruct) * (size_t(instancePoolSize) + 1),
	//    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
	//    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	//        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	// m_uniformBuffer.setName("CustomMaterial UBO");

	// UBOStruct uboStruct;
	// uboStruct.color = vector4{ 0.9f, 0.5f, 0.25f, 1.0f };
	// uboStruct.metalness = 0.1f;
	// uboStruct.roughness = 0.3f;

	// m_uboStructs.resize(size_t(instancePoolSize) + 1, uboStruct);

	// for (auto& s : m_uboStructs)
	//	s = uboStruct;
	//
	// m_vec4Parameters["color"] =
	//    Vec4Parameter{ vector4{ 0.9f, 0.5f, 0.25f, 1.0f }, 0,
	//	               &m_uniformBuffer };
	// m_floatParameters["metalness"] =
	//    FloatParameter{ 0.1f, sizeof(vector4), &m_uniformBuffer };
	// m_floatParameters["roughness"] =
	//    FloatParameter{ 0.3f,
	//	                sizeof(vector4) * (size_t(instancePoolSize) + 1) +
	//	                    sizeof(vector4),
	//	                &m_uniformBuffer };
	//
	// std::vector<vector4> colors(instancePoolSize + 1,
	//                            m_vec4Parameters["color"].value);
	// std::vector<vector4> metalnesses(
	//    instancePoolSize + 1,
	//    vector4(m_floatParameters["metalness"].value, 0, 0, 0));
	// std::vector<vector4> roughnesses(
	//    instancePoolSize + 1,
	//    vector4(m_floatParameters["roughness"].value, 0, 0, 0));
	//
	// m_uboStruct.color = m_vec4Parameters["color"].value;
	// m_uboStruct.metalness = m_floatParameters["metalness"].value;
	// m_uboStruct.roughness = m_floatParameters["roughness"].value;

	// void* ptr = m_uniformBuffer.map();
	// std::memcpy(ptr, m_uboStructs.data(),
	//            sizeof(*m_uboStructs.data()) * m_uboStructs.size());
	// m_uniformBuffer.unmap();

#pragma region descriptor pool
	std::array poolSizes{
		// VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                      (instancePoolSize + 1) * 4 },
	};

	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.maxSets = 1;
	poolInfo.poolSizeCount = uint32_t(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();

	m_descriptorPool = vk.create(poolInfo);
	if (!m_descriptorPool)
	{
		std::cerr << "error: failed to create descriptor pool" << std::endl;
		abort();
	}
#pragma endregion

#pragma region descriptor set layout
	VkDescriptorSetLayoutBinding layoutBindingAlbedos{};
	layoutBindingAlbedos.binding = 0;
	layoutBindingAlbedos.descriptorCount = instancePoolSize + 1;
	layoutBindingAlbedos.descriptorType =
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindingAlbedos.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding layoutBindingNormals{};
	layoutBindingNormals.binding = 1;
	layoutBindingNormals.descriptorCount = instancePoolSize + 1;
	layoutBindingNormals.descriptorType =
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindingNormals.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding layoutBindingMetalnesses{};
	layoutBindingMetalnesses.binding = 2;
	layoutBindingMetalnesses.descriptorCount = instancePoolSize + 1;
	layoutBindingMetalnesses.descriptorType =
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindingMetalnesses.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding layoutBindingRoughnesses{};
	layoutBindingRoughnesses.binding = 3;
	layoutBindingRoughnesses.descriptorCount = instancePoolSize + 1;
	layoutBindingRoughnesses.descriptorType =
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindingRoughnesses.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array layoutBindings{
		layoutBindingAlbedos,
		layoutBindingNormals,
		layoutBindingMetalnesses,
		layoutBindingRoughnesses,
	};

	vk::DescriptorSetLayoutCreateInfo setLayoutInfo;
	setLayoutInfo.bindingCount = uint32_t(layoutBindings.size());
	setLayoutInfo.pBindings = layoutBindings.data();

	m_descriptorSetLayout = vk.create(setLayoutInfo);
	if (!m_descriptorSetLayout)
	{
		std::cerr << "error: failed to create descriptor set layout"
		          << std::endl;
		abort();
	}
#pragma endregion

#pragma region descriptor set
	m_descriptorSet = vk.allocate(m_descriptorPool, m_descriptorSetLayout);

	if (!m_descriptorSet)
	{
		std::cerr << "error: failed to allocate descriptor set" << std::endl;
		abort();
	}
#pragma endregion

	// VkDescriptorBufferInfo setBufferInfo{};
	// setBufferInfo.buffer = m_uniformBuffer;
	// setBufferInfo.range = sizeof(UBOStruct) * (size_t(instancePoolSize) +
	// 1); setBufferInfo.offset = 0;

	// VkDescriptorImageInfo setAlbedosInfo{};
	// setAlbedosInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	// setAlbedosInfo.imageView =

	// vk::WriteDescriptorSet uboWrite;
	// uboWrite.descriptorCount = 1;
	// uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	// uboWrite.dstArrayElement = 0;
	// uboWrite.dstBinding = 0;
	// uboWrite.dstSet = m_descriptorSet;
	// uboWrite.pBufferInfo = &setBufferInfo;

	// vk.updateDescriptorSets(uboWrite);
}

// float CustomMaterial::floatParameter(const std::string& name,
//                                      uint32_t instanceIndex)
//{
//	if (name == "metalness")
//		return m_uboStructs[instanceIndex].metalness;
//	else if (name == "roughness")
//		return m_uboStructs[instanceIndex].roughness;
//	else
//		return Material::floatParameter(name, instanceIndex);
//}
//
// vector4 CustomMaterial::vec4Parameter(const std::string& name,
//                                       uint32_t instanceIndex)
//{
//	if (name == "color")
//		return m_uboStructs[instanceIndex].color;
//	else
//		return Material::vec4Parameter(name, instanceIndex);
//}

const Texture2D* CustomMaterial::Texture2DParameter(const std::string& name,
                                                    uint32_t instanceIndex)
{
	if (name == "albedo")
		return m_albedoTexture[instanceIndex];
	else if (name == "normal")
		return m_normalTexture[instanceIndex];
	else if (name == "metalness")
		return m_metalnessTexture[instanceIndex];
	else if (name == "roughness")
		return m_roughnessTexture[instanceIndex];
	else
		return nullptr;
}

// void CustomMaterial::setFloatParameter(const std::string& name,
//                                        uint32_t instanceIndex, float a)
//{
//	if (name == "metalness")
//		m_uboStructs[instanceIndex].metalness = a;
//	else if (name == "roughness")
//		m_uboStructs[instanceIndex].roughness = a;
//	else
//		Material::setFloatParameter(name, instanceIndex, a);
//
//	UBOStruct* ptr = m_uniformBuffer.map<UBOStruct>();
//	ptr[instanceIndex] = m_uboStructs[instanceIndex];
//	m_uniformBuffer.unmap();
//}
//
// void CustomMaterial::setVec4Parameter(const std::string& name,
//                                       uint32_t instanceIndex,
//                                       const vector4& a)
//{
//	if (name == "color")
//		m_uboStructs[instanceIndex].color = a;
//	else
//		Material::setVec4Parameter(name, instanceIndex, a);
//
//	UBOStruct* ptr = m_uniformBuffer.map<UBOStruct>();
//	ptr[instanceIndex] = m_uboStructs[instanceIndex];
//	m_uniformBuffer.unmap();
//}

void CustomMaterial::setTexture2DParameter(const std::string& name,
                                           uint32_t instanceIndex,
                                           const Texture2D* a)
{
	uint32_t binding = -1;

	if (name == "albedo")
	{
		m_albedoTexture[instanceIndex] = a;
		binding = 0;
	}
	else if (name == "normal")
	{
		m_normalTexture[instanceIndex] = a;
		binding = 1;
	}
	else if (name == "metalness")
	{
		m_metalnessTexture[instanceIndex] = a;
		binding = 2;
	}
	else if (name == "roughness")
	{
		m_roughnessTexture[instanceIndex] = a;
		binding = 3;
	}

	if (a)
	{
		auto& vk = renderWindow().device();

		VkDescriptorImageInfo setAlbedosInfo{};
		setAlbedosInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		setAlbedosInfo.imageView = a->view();
		setAlbedosInfo.sampler = a->sampler();

		vk::WriteDescriptorSet uboWrite;
		uboWrite.descriptorCount = 1;
		uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		uboWrite.dstArrayElement = instanceIndex;
		uboWrite.dstBinding = binding;
		uboWrite.dstSet = m_descriptorSet;
		uboWrite.pImageInfo = &setAlbedosInfo;

		vk.updateDescriptorSets(uboWrite);
	}
}

MaterialVertexFunction CustomMaterial::vertexFunction(
    sdw::VertexWriter& writer, VertexShaderBuildDataBase* shaderBuildData)
{
	using namespace sdw;

	return writer.implementFunction<Float>(
	    "DefaultVertex",
	    [&](Vec3& inOutPosition, Vec3& inOutNormal) {
		    inOutPosition = inOutPosition;
		    writer.returnStmt(0.0_f);
	    },
	    InOutVec3{ writer, "inOutPosition" },
	    InOutVec3{ writer, "inOutNormal" });
}

MaterialFragmentFunction CustomMaterial::fragmentFunction(
    sdw::FragmentWriter& writer, FragmentShaderBuildDataBase* shaderBuildData)
{
	FragmentShaderBuildData* buildData =
	    dynamic_cast<FragmentShaderBuildData*>(shaderBuildData);

	if (buildData == nullptr)
		throw std::runtime_error("invalid shaderBuildData type");

	using namespace sdw;

	// buildData->uboStruct = std::make_unique<sdw::StructInstance>(
	//    sdw::Struct(writer, "CustomMaterialUBOStruct"));
	// buildData->uboStruct->declMember<Vec4>("color");
	// buildData->uboStruct->declMember<Float>("metalness");
	// buildData->uboStruct->declMember<Float>("roughness");
	// buildData->uboStruct->end();

	// buildData->ubo = std::make_unique<sdw::Ubo>(sdw::Ubo(
	//    writer, "CustomMaterialUBO", 0, 2));
	//// buildData->ubo->declMember<CustomMaterialUBOStruct>(
	////    "materials", instancePoolSize() + 1);
	// buildData->ubo->declMember<Vec4>("color0");
	// buildData->ubo->declMember<Float>("metalness0");
	// buildData->ubo->declMember<Float>("roughness0");
	// buildData->ubo->declMember<Vec4>("color1");
	// buildData->ubo->declMember<Float>("metalness1");
	// buildData->ubo->declMember<Float>("roughness1");
	// buildData->ubo->declMember<Vec4>("color2");
	// buildData->ubo->declMember<Float>("metalness2");
	// buildData->ubo->declMember<Float>("roughness2");
	// buildData->ubo->end();

	buildData->albedos =
	    std::make_unique<sdw::Array<sdw::SampledImage2DRgba32>>(
	        writer.declSampledImageArray<FImg2DRgba32>(
	            "albedos", 0, 2, instancePoolSize() + 1));

	buildData->normals =
	    std::make_unique<sdw::Array<sdw::SampledImage2DRgba32>>(
	        writer.declSampledImageArray<FImg2DRgba32>(
	            "normals", 1, 2, instancePoolSize() + 1));

	buildData->metalnesses =
	    std::make_unique<sdw::Array<sdw::SampledImage2DRgba32>>(
	        writer.declSampledImageArray<FImg2DRgba32>(
	            "metalnesses", 2, 2, instancePoolSize() + 1));

	buildData->roughnesses =
	    std::make_unique<sdw::Array<sdw::SampledImage2DRgba32>>(
	        writer.declSampledImageArray<FImg2DRgba32>(
	            "roughnesses", 3, 2, instancePoolSize() + 1));

	// auto t = writer.declSampledImageArray<FImg2DRgba32>("albedos", 0, 2,
	// instancePoolSize() + 1)

	MaterialFragmentFunction res = writer.implementFunction<Float>(
	    "CustomFragment",
	    [&writer, buildData](const UInt& inMaterialInstanceIndex,
	                         const Vec2& inUV, Vec4& inOutAlbedo,
	                         Vec3& inOutWsPosition, Vec3& inOutWsNormal,
	                         Vec3& inOutWsTangent, Float& inOutMetalness,
	                         Float& inOutRoughness) {
		    // Locale(material,
		    //       buildData->ubo->getMemberArray<CustomMaterialUBOStruct>(
		    //           "materials")[inMaterialInstanceIndex]);

		    // inOutAlbedo = material.color;
		    // inOutMetalness = material.metalness;
		    // inOutRoughness = material.roughness;

		    Locale(albedo,
		           pow(texture((*buildData->albedos)[inMaterialInstanceIndex],
		                       inUV),
		               vec4(2.2_f)));

			//inOutWsNormal = vec3(0.0_f, 1.0_f, 0.0_f);
			//inOutWsTangent = vec3(1.0_f, 0.0_f, 0.0_f);

		    Locale(
		        sampledNormal,
		        texture((*buildData->normals)[inMaterialInstanceIndex], inUV)
		            .xyz());
		    sampledNormal =
		        normalize((sampledNormal * vec3(2.0_f) - vec3(1.0_f)));

		    Locale(B, cross(inOutWsNormal, inOutWsTangent));
		    Locale(WorldToTangent,
		           transpose(mat3(inOutWsTangent, B, inOutWsNormal)));
		    Locale(TangentToWorld, inverse(WorldToTangent));

		    inOutWsNormal =
		        normalize(TangentToWorld * sampledNormal);
			inOutWsNormal.z() = -inOutWsNormal.z();

		    Locale(metalness,
		           texture((*buildData->metalnesses)[inMaterialInstanceIndex],
		                   inUV)
		               .r());

		    Locale(roughness,
		           texture((*buildData->roughnesses)[inMaterialInstanceIndex],
		                   inUV)
		               .r());

		    inOutAlbedo = albedo;
		    //inOutAlbedo = vec4(1.0_f);
		    //inOutMetalness = metalness;
		    //inOutRoughness = roughness;
		    inOutMetalness = 0.0_f;
		    inOutRoughness = 0.7_f;

		    //    (*buildData->ubo)[inMaterialInstanceIndex].getMember<Vec4>(
		    //        "color");
		    // inOutMetalness =
		    //    (*buildData->ubo)[inMaterialInstanceIndex].getMember<Float>(
		    //        "metalness");
		    // inOutRoughness =
		    //    (*buildData->ubo)[inMaterialInstanceIndex].getMember<Float>(
		    //        "roughness");
		    writer.returnStmt(0.0_f);
	    },
	    InUInt{ writer, "inMaterialInstanceIndex" }, InVec2{ writer, "inUV" },
	    InOutVec4{ writer, "inOutAlbedo" },
	    InOutVec3{ writer, "inOutWsPosition" },
	    InOutVec3{ writer, "inOutWsNormal" },
	    InOutVec3{ writer, "inOutWsTangent" },
	    InOutFloat{ writer, "inOutMetalness" },
	    InOutFloat{ writer, "inOutRoughness" });

	return res;
}

std::unique_ptr<Material::FragmentShaderBuildDataBase>
CustomMaterial::instantiateFragmentShaderBuildData()
{
	return std::make_unique<FragmentShaderBuildData>();
}
}  // namespace cdm
