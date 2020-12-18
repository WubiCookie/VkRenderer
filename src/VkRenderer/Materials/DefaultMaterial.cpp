#include "DefaultMaterial.hpp"
#include "RenderWindow.hpp"
#include "TextureFactory.hpp"

#include <iostream>
#include <stdexcept>

using namespace sdw;

namespace shader
{
struct DefaultMaterialData : public StructInstance
{
	DefaultMaterialData(ShaderWriter& writer, ast::expr::ExprPtr expr)
	    : StructInstance{ writer, std::move(expr) },
	      color{ getMember<Vec4>("color") },
	      metalness{ getMember<Float>("metalness") },
	      roughness{ getMember<Float>("roughness") },
	      albedoIndex{ getMember<UInt>("albedoIndex") }
	{
	}

	DefaultMaterialData& operator=(DefaultMaterialData const& rhs)
	{
		StructInstance::operator=(rhs);
		return *this;
	}

	static ast::type::StructPtr makeType(ast::type::TypesCache& cache)
	{
		auto result =
		    cache.getStruct(ast::type::MemoryLayout::eStd140, "DefaultMaterialData");

		if (result->empty())
		{
			result->declMember("color", ast::type::Kind::eVec4F);
			result->declMember("metalness", ast::type::Kind::eFloat);
			result->declMember("roughness", ast::type::Kind::eFloat);
			result->declMember("albedoIndex", ast::type::Kind::eUInt);
		}

		return result;
	}

	Vec4 color;
	Float metalness;
	Float roughness;
	UInt albedoIndex;

private:
	using StructInstance::getMember;
	using StructInstance::getMemberArray;
};

Writer_Parameter(DefaultMaterialData);

enum class TypeName
{
	eDefaultMaterialData = 200,
};
}  // namespace shader

namespace sdw
{
template <>
struct TypeTraits<shader::DefaultMaterialData>
{
	static ast::type::Kind constexpr TypeEnum =
	    ast::type::Kind(shader::TypeName::eDefaultMaterialData);
};
}  // namespace sdw

namespace cdm
{
DefaultMaterial::DefaultMaterial(RenderWindow& renderWindow,
                                 PbrShadingModel& shadingModel,
                                 uint32_t instancePoolSize)
    : Material(renderWindow, shadingModel, instancePoolSize),
      m_textureRef(m_texture)
{
	auto& vk = renderWindow.device();

	m_uniformBuffer =
	    Buffer(vk, sizeof(UBOStruct) * (size_t(instancePoolSize) + 1),
	           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
	           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_uniformBuffer.setName("DefaultMaterial SSBO");

	UBOStruct uboStruct;
	uboStruct.color = vector4{ 0.9f, 0.5f, 0.25f, 1.0f };
	uboStruct.metalness = 0.1f;
	uboStruct.roughness = 0.3f;
	uboStruct.textureIndex = 1023;

	m_uboStructs.resize(size_t(instancePoolSize) + 1, uboStruct);

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

	void* ptr = m_uniformBuffer.map();
	std::memcpy(ptr, m_uboStructs.data(),
	            sizeof(*m_uboStructs.data()) * m_uboStructs.size());
	m_uniformBuffer.unmap();

#pragma region descriptor pool
	std::array poolSizes{
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                      1024 },
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
	VkDescriptorSetLayoutBinding layoutBindingUbo{};
	layoutBindingUbo.binding = 0;
	layoutBindingUbo.descriptorCount = 1;
	layoutBindingUbo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	layoutBindingUbo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding layoutBindingTextureUbo{};
	layoutBindingTextureUbo.binding = 1;
	layoutBindingTextureUbo.descriptorCount = 1024;
	layoutBindingTextureUbo.descriptorType =
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindingTextureUbo.stageFlags =
	    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array layoutBindings{
		layoutBindingUbo,
		layoutBindingTextureUbo,
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

	VkDescriptorBufferInfo setBufferInfo{};
	setBufferInfo.buffer = m_uniformBuffer;
	setBufferInfo.range = sizeof(UBOStruct) * (size_t(instancePoolSize) + 1);
	setBufferInfo.offset = 0;

	vk::WriteDescriptorSet uboWrite;
	uboWrite.descriptorCount = 1;
	uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	uboWrite.dstArrayElement = 0;
	uboWrite.dstBinding = 0;
	uboWrite.dstSet = m_descriptorSet;
	uboWrite.pBufferInfo = &setBufferInfo;

	vk.updateDescriptorSets(uboWrite);

	TextureFactory f(vk);

#pragma region default texture
	f.setFormat(VK_FORMAT_R8G8B8A8_UNORM);
	f.setUsage(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

	m_texture = f.createTexture2D();

	// m_texture = Texture2D(
	//	renderWindow, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
	//	VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
	//	VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	union U
	{
		uint32_t defaultTextureTexel;
		uint8_t b[4];
	};

	U u;
	u.b[0] = 255;
	u.b[1] = 255;
	u.b[2] = 255;
	u.b[3] = 255;

	VkBufferImageCopy defaultTextureCopy{};
	defaultTextureCopy.bufferImageHeight = 1;
	defaultTextureCopy.bufferRowLength = 1;
	defaultTextureCopy.imageExtent.width = 1;
	defaultTextureCopy.imageExtent.height = 1;
	defaultTextureCopy.imageExtent.depth = 1;
	defaultTextureCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	defaultTextureCopy.imageSubresource.baseArrayLayer = 0;
	defaultTextureCopy.imageSubresource.layerCount = 1;
	defaultTextureCopy.imageSubresource.mipLevel = 0;

	m_texture.uploadDataImmediate(
	    &u.defaultTextureTexel, sizeof(uint32_t), defaultTextureCopy,
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = m_texture.view();
	imageInfo.sampler = m_texture.sampler();

	vk::WriteDescriptorSet textureWrite;
	textureWrite.descriptorCount = 1;
	textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureWrite.dstArrayElement = 0;
	textureWrite.dstBinding = 1;
	textureWrite.dstSet = m_descriptorSet;
	textureWrite.pImageInfo = &imageInfo;

	m_textures.reserve(1024);

	for (uint32_t i = 0; i < 1024; i++)
	{
		textureWrite.dstArrayElement = i;
		m_textures.push_back(m_texture);
		vk.updateDescriptorSets(textureWrite);
	}

	/*vk::WriteDescriptorSet textureWrite;
	textureWrite.descriptorCount = 1;
	textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureWrite.dstArrayElement = 0;
	textureWrite.dstBinding = 1;
	textureWrite.dstSet = m_descriptorSet;
	textureWrite.pImageInfo = &imageInfo;*/

#pragma endregion
}

float DefaultMaterial::floatParameter(const std::string& name,
                                      uint32_t instanceIndex)
{
	if (name == "metalness")
		return m_uboStructs[instanceIndex].metalness;
	else if (name == "roughness")
		return m_uboStructs[instanceIndex].roughness;
	else
		return Material::floatParameter(name, instanceIndex);
}

vector4 DefaultMaterial::vec4Parameter(const std::string& name,
                                       uint32_t instanceIndex)
{
	if (name == "color")
		return m_uboStructs[instanceIndex].color;
	else
		return Material::vec4Parameter(name, instanceIndex);
}

void DefaultMaterial::setFloatParameter(const std::string& name,
                                        uint32_t instanceIndex, float a)
{
	if (name == "metalness")
		m_uboStructs[instanceIndex].metalness = a;
	else if (name == "roughness")
		m_uboStructs[instanceIndex].roughness = a;
	else
		Material::setFloatParameter(name, instanceIndex, a);

	UBOStruct* ptr = m_uniformBuffer.map<UBOStruct>();
	ptr[instanceIndex] = m_uboStructs[instanceIndex];
	m_uniformBuffer.unmap();
}

void DefaultMaterial::setVec4Parameter(const std::string& name,
                                       uint32_t instanceIndex,
                                       const vector4& a)
{
	if (name == "color")
		m_uboStructs[instanceIndex].color = a;
	else
		Material::setVec4Parameter(name, instanceIndex, a);

	UBOStruct* ptr = m_uniformBuffer.map<UBOStruct>();
	ptr[instanceIndex] = m_uboStructs[instanceIndex];
	m_uniformBuffer.unmap();
}

void DefaultMaterial::setTextureParameter(const std::string& name,
                                          uint32_t instanceIndex,
                                          Texture2D& texture)
{
	auto found =
	    std::find_if(m_textures.begin(), m_textures.end(),
	                 [&](const Texture2D& a) { return a == texture; });

	uint32_t textureIndex = 0;
	if (found == m_textures.end())
	{
		for (auto& tex : m_textures)
		{
			if (tex.get() == m_texture)
			{
				tex = texture;
				break;
			}
			textureIndex++;
		}

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = texture.view();
		imageInfo.sampler = texture.sampler();

		vk::WriteDescriptorSet textureWrite;
		textureWrite.descriptorCount = 1;
		textureWrite.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		textureWrite.dstArrayElement = textureIndex;
		textureWrite.dstBinding = 1;
		textureWrite.dstSet = m_descriptorSet;
		textureWrite.pImageInfo = &imageInfo;

		auto& vk = renderWindow().device();
		vk.updateDescriptorSets(textureWrite);

		m_textureRef = texture;
	}
	else
	{
		textureIndex = std::distance(m_textures.begin(), found);
	}

	m_uboStructs[instanceIndex].textureIndex = textureIndex;

	UBOStruct* ptr = m_uniformBuffer.map<UBOStruct>();
	ptr[instanceIndex].textureIndex = textureIndex;
	m_uniformBuffer.unmap();
}

MaterialVertexFunction DefaultMaterial::vertexFunction(
    sdw::VertexWriter& writer, VertexShaderBuildDataBase* shaderBuildData)
{
	using namespace sdw;

	return writer.implementFunction<Float>(
	    "DefaultVertex",
	    [&](Vec3 inOutPosition, Vec3 inOutNormal) {
		    //inOutPosition = inOutPosition;
		    writer.returnStmt(0.0_f);
	    },
	    InOutVec3{ writer, "inOutPosition" },
	    InOutVec3{ writer, "inOutNormal" });
}

MaterialFragmentFunction DefaultMaterial::fragmentFunction(
    sdw::FragmentWriter& writer, FragmentShaderBuildDataBase* shaderBuildData)
{
	FragmentShaderBuildData* buildData =
	    dynamic_cast<FragmentShaderBuildData*>(shaderBuildData);

	if (buildData == nullptr)
		throw std::runtime_error("invalid shaderBuildData type");

	using namespace sdw;

	// buildData->uboStruct = std::make_unique<sdw::StructInstance>(
	//    sdw::Struct(writer, "DefaultMaterialUBOStruct"));
	// buildData->uboStruct->declMember<Vec4>("color");
	// buildData->uboStruct->declMember<Float>("metalness");
	// buildData->uboStruct->declMember<Float>("roughness");
	// buildData->uboStruct->end();

	buildData->ssbo = std::make_unique<sdw::ArraySsboT<shader::DefaultMaterialData>>(writer, "DefaultMaterialUBO", 0, 2);

	buildData->textures =
	    std::make_unique<sdw::Array<sdw::SampledImage2DRgba32>>(
	        writer.declSampledImageArray<FImg2DRgba32>("tex", 1, 2, 1024));

	// buildData->ubo->declMember<DefaultMaterialUBOStruct>(
	//    "materials", instancePoolSize() + 1);
	//buildData->ubo->declMember<Vec4>("color0");
	//buildData->ubo->declMember<Float>("metalness0");
	//buildData->ubo->declMember<Float>("roughness0");
	//buildData->ubo->declMember<UInt>("textureIndex0");
	//buildData->ubo->declMember<Vec4>("color1");
	//buildData->ubo->declMember<Float>("metalness1");
	//buildData->ubo->declMember<Float>("roughness1");
	//buildData->ubo->declMember<UInt>("textureIndex1");
	//buildData->ubo->declMember<Vec4>("color2");
	//buildData->ubo->declMember<Float>("metalness2");
	//buildData->ubo->declMember<Float>("roughness2");
	//buildData->ubo->declMember<UInt>("textureIndex2");
	//buildData->ubo->declMember<Vec4>("color3");
	//buildData->ubo->declMember<Float>("metalness3");
	//buildData->ubo->declMember<Float>("roughness3");
	//buildData->ubo->declMember<UInt>("textureIndex3");
	//buildData->ubo->declMember<Vec4>("color4");
	//buildData->ubo->declMember<Float>("metalness4");
	//buildData->ubo->declMember<Float>("roughness4");
	//buildData->ubo->declMember<UInt>("textureIndex4");
	//buildData->ubo->end();

	MaterialFragmentFunction res = writer.implementFunction<Float>(
	    "DefaultFragment",
	    [&writer, buildData](const UInt& inMaterialInstanceIndex,
	                         Vec4 inOutAlbedo, Vec2 inOutUv,
	                         Vec3 inOutWsPosition, Vec3 inOutWsNormal,
	                         Vec3 inOutWsTangent, Float inOutMetalness,
	                         Float inOutRoughness) {
		    // Locale(material,
		    //       buildData->ubo->getMemberArray<DefaultMaterialUBOStruct>(
		    //           "materials")[inMaterialInstanceIndex]);

		    // inOutAlbedo = material.color;
		    // inOutMetalness = material.metalness;
		    // inOutRoughness = material.roughness;

			auto& ssbo = *buildData->ssbo;
		    auto& textures = *buildData->textures;

			inOutAlbedo =
		        textures[ssbo[inMaterialInstanceIndex].albedoIndex].sample(
		            inOutUv);

			inOutMetalness = ssbo[inMaterialInstanceIndex].metalness;
			inOutRoughness = ssbo[inMaterialInstanceIndex].roughness;

		    //IF(writer, inMaterialInstanceIndex == 0_u)
		    //{
			   // sdw::Vec4 sample =
			   //     (*buildData->textures)[buildData->ubo->getMember<UInt>(
			   //                                "textureIndex0")]
			   //         .sample(inOutUv);
			   // inOutAlbedo = sample;

			   // inOutMetalness =
			   //     buildData->ubo->getMember<Float>("metalness0");
			   // inOutRoughness =
			   //     buildData->ubo->getMember<Float>("roughness0");
		    //}
		    //ELSEIF(inMaterialInstanceIndex == 1_u)
		    //{
			   // sdw::Vec4 sample =
			   //     (*buildData->textures)[buildData->ubo->getMember<UInt>(
			   //                                "textureIndex1")]
			   //         .sample(inOutUv);
			   // inOutAlbedo = sample;
			   // inOutMetalness =
			   //     buildData->ubo->getMember<Float>("metalness1");
			   // inOutRoughness =
			   //     buildData->ubo->getMember<Float>("roughness1");
		    //}
		    //ELSEIF(inMaterialInstanceIndex == 2_u)
		    //{
			   // sdw::Vec4 sample =
			   //     (*buildData->textures)[buildData->ubo->getMember<UInt>(
			   //                                "textureIndex2")]
			   //         .sample(inOutUv);
			   // inOutAlbedo = sample;
			   // inOutMetalness =
			   //     buildData->ubo->getMember<Float>("metalness2");
			   // inOutRoughness =
			   //     buildData->ubo->getMember<Float>("roughness2");
		    //}
		    //ELSEIF(inMaterialInstanceIndex == 3_u)
		    //{
			   // sdw::Vec4 sample =
			   //     (*buildData->textures)[buildData->ubo->getMember<UInt>(
			   //                                "textureIndex3")]
			   //         .sample(inOutUv);
			   // inOutAlbedo = sample;
			   // inOutMetalness =
			   //     buildData->ubo->getMember<Float>("metalness3");
			   // inOutRoughness =
			   //     buildData->ubo->getMember<Float>("roughness3");
		    //}
		    //ELSE
		    //{
			   // inOutAlbedo = buildData->ubo->getMember<Vec4>("color4");
			   // inOutMetalness =
			   //     buildData->ubo->getMember<Float>("metalness4");
			   // inOutRoughness =
			   //     buildData->ubo->getMember<Float>("roughness4");
		    //}
		    //FI;

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
	    InUInt{ writer, "inMaterialInstanceIndex" },
	    InOutVec4{ writer, "inOutAlbedo" }, InOutVec2{ writer, "inOutUv" },
	    InOutVec3{ writer, "inOutWsPosition" },
	    InOutVec3{ writer, "inOutWsNormal" },
	    InOutVec3{ writer, "inOutWsTangent" },
	    InOutFloat{ writer, "inOutMetalness" },
	    InOutFloat{ writer, "inOutRoughness" });

	return res;
}

std::unique_ptr<Material::FragmentShaderBuildDataBase>
DefaultMaterial::instantiateFragmentShaderBuildData()
{
	return std::make_unique<FragmentShaderBuildData>();
}
}  // namespace cdm
