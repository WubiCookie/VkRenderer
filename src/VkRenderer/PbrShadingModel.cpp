#include "PbrShadingModel.hpp"

#include "CommandBuffer.hpp"
#include "CommandBufferPool.hpp"
#include "Material.hpp"

#include <iostream>

using namespace sdw;

namespace shader
{
struct ShadingModelData : public StructInstance
{
	ShadingModelData(ast::Shader* shader, ast::expr::ExprPtr expr)
	    : StructInstance{ shader, std::move(expr) },
	      pointLightsCount{ getMember<UInt>("pointLightsCount") },
	      directionalLightsCount{ getMember<UInt>("directionalLightsCount") }
	{
	}

	ShadingModelData& operator=(ShadingModelData const& rhs)
	{
		StructInstance::operator=(rhs);
		return *this;
	}

	static ast::type::StructPtr makeType(ast::type::TypesCache& cache)
	{
		auto result =
		    cache.getStruct(ast::type::MemoryLayout::eStd140, "PointLights");

		if (result->empty())
		{
			result->declMember("pointLightsCount", ast::type::Kind::eUInt);
			result->declMember("directionalLightsCount",
			                   ast::type::Kind::eUInt);
		}

		return result;
	}

	UInt pointLightsCount;
	UInt directionalLightsCount;

private:
	using StructInstance::getMember;
	using StructInstance::getMemberArray;
};

struct PointLights : public StructInstance
{
	PointLights(ast::Shader* shader, ast::expr::ExprPtr expr)
	    : StructInstance{ shader, std::move(expr) },
	      position{ getMember<Vec3>("position") },
	      color{ getMember<Vec4>("color") },
	      intensity{ getMember<Float>("intensity") }
	{
	}

	PointLights& operator=(PointLights const& rhs)
	{
		StructInstance::operator=(rhs);
		return *this;
	}

	static ast::type::StructPtr makeType(ast::type::TypesCache& cache)
	{
		auto result =
		    cache.getStruct(ast::type::MemoryLayout::eStd140, "PointLights");

		if (result->empty())
		{
			result->declMember("position", ast::type::Kind::eVec3F);
			result->declMember("color", ast::type::Kind::eVec4F);
			result->declMember("intensity", ast::type::Kind::eFloat);
		}

		return result;
	}

	Vec3 position;
	Vec4 color;
	Float intensity;

private:
	using StructInstance::getMember;
	using StructInstance::getMemberArray;
};

struct DirectionalLights : public StructInstance
{
	DirectionalLights(ast::Shader* shader, ast::expr::ExprPtr expr)
	    : StructInstance{ shader, std::move(expr) },
	      direction{ getMember<Vec3>("direction") },
	      color{ getMember<Vec4>("color") },
	      intensity{ getMember<Float>("intensity") }
	{
	}

	DirectionalLights& operator=(DirectionalLights const& rhs)
	{
		StructInstance::operator=(rhs);
		return *this;
	}

	static ast::type::StructPtr makeType(ast::type::TypesCache& cache)
	{
		auto result = cache.getStruct(ast::type::MemoryLayout::eStd140,
		                              "DirectionalLights");

		if (result->empty())
		{
			result->declMember("direction", ast::type::Kind::eVec3F);
			result->declMember("color", ast::type::Kind::eVec4F);
			result->declMember("intensity", ast::type::Kind::eFloat);
		}

		return result;
	}

	Vec3 direction;
	Vec4 color;
	Float intensity;

private:
	using StructInstance::getMember;
	using StructInstance::getMemberArray;
};

Writer_Parameter(ShadingModelData);
Writer_Parameter(PointLights);
Writer_Parameter(DirectionalLights);

enum class TypeName
{
	eShadingModelData = 100,
	ePointLights,
	eDirectionalLights,
};
}  // namespace shader

namespace sdw
{
template <>
struct TypeTraits<shader::ShadingModelData>
{
	static ast::type::Kind constexpr TypeEnum =
	    ast::type::Kind(shader::TypeName::eShadingModelData);
};
template <>
struct TypeTraits<shader::PointLights>
{
	static ast::type::Kind constexpr TypeEnum =
	    ast::type::Kind(shader::TypeName::ePointLights);
};
template <>
struct TypeTraits<shader::DirectionalLights>
{
	static ast::type::Kind constexpr TypeEnum =
	    ast::type::Kind(shader::TypeName::eDirectionalLights);
};
}  // namespace sdw

namespace cdm
{
using DistributionGGX_Signature = Function<Float, InVec3, InVec3, InFloat>;
using GeometrySchlickGGX_Signature = Function<Float, InFloat, InFloat>;
using GeometrySmith_Signature =
    Function<Float, InVec3, InVec3, InVec3, InFloat>;
using fresnelSchlick_Signature = Function<Vec4, InFloat, InVec4>;
using fresnelSchlickRoughness_Signature =
    Function<Vec4, InFloat, InVec4, InFloat>;

struct FragmentShaderBuildData : PbrShadingModel::FragmentShaderBuildDataBase
{
	std::unique_ptr<SampledImageCubeRgba32> irradianceMap;
	std::unique_ptr<SampledImageCubeRgba32> prefilteredMap;
	std::unique_ptr<SampledImage2DRg32> brdfLut;

	std::unique_ptr<sdw::Ubo> shadingModelData;
	std::unique_ptr<ArraySsboT<shader::PointLights>> pointLights;
	std::unique_ptr<ArraySsboT<shader::DirectionalLights>> directionalLights;

	std::unique_ptr<Float> PI;

	DistributionGGX_Signature DistributionGGX;
	GeometrySchlickGGX_Signature GeometrySchlickGGX;
	GeometrySmith_Signature GeometrySmith;
	fresnelSchlick_Signature fresnelSchlick;
	fresnelSchlickRoughness_Signature fresnelSchlickRoughness;
};

PbrShadingModel::PbrShadingModel(const VulkanDevice& vulkanDevice,
                                 uint32_t maxPointLights,
                                 uint32_t maxDirectionalLights)
    : m_vulkanDevice(&vulkanDevice),
      m_maxPointLights(maxPointLights),
      m_maxDirectionalLights(maxDirectionalLights)
{
	auto& vk = *m_vulkanDevice.get();

#pragma region descriptor pool
	std::array poolSizes{
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
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
	{
		VkDescriptorSetLayoutBinding layoutBindingIrradianceMapImages{};
		layoutBindingIrradianceMapImages.binding = 0;
		layoutBindingIrradianceMapImages.descriptorCount = 1;
		layoutBindingIrradianceMapImages.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindingIrradianceMapImages.stageFlags =
		    VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding layoutBindingPrefilteredMapImages{};
		layoutBindingPrefilteredMapImages.binding = 1;
		layoutBindingPrefilteredMapImages.descriptorCount = 1;
		layoutBindingPrefilteredMapImages.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindingPrefilteredMapImages.stageFlags =
		    VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding layoutBindingBrdfLutImages{};
		layoutBindingBrdfLutImages.binding = 2;
		layoutBindingBrdfLutImages.descriptorCount = 1;
		layoutBindingBrdfLutImages.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindingBrdfLutImages.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding layoutBindingShadingModelBuffer{};
		layoutBindingShadingModelBuffer.binding = 3;
		layoutBindingShadingModelBuffer.descriptorCount = 1;
		layoutBindingShadingModelBuffer.descriptorType =
		    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBindingShadingModelBuffer.stageFlags =
		    VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding layoutBindingPointLightsBuffer{};
		layoutBindingPointLightsBuffer.binding = 4;
		layoutBindingPointLightsBuffer.descriptorCount = 1;
		layoutBindingPointLightsBuffer.descriptorType =
		    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBindingPointLightsBuffer.stageFlags =
		    VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding layoutBindingDirectionalLightsBuffer{};
		layoutBindingDirectionalLightsBuffer.binding = 5;
		layoutBindingDirectionalLightsBuffer.descriptorCount = 1;
		layoutBindingDirectionalLightsBuffer.descriptorType =
		    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBindingDirectionalLightsBuffer.stageFlags =
		    VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array layoutBindings{ layoutBindingIrradianceMapImages,
			                       layoutBindingPrefilteredMapImages,
			                       layoutBindingBrdfLutImages,
			                       layoutBindingShadingModelBuffer,
			                       layoutBindingPointLightsBuffer,
			                       layoutBindingDirectionalLightsBuffer };

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

#pragma region shading model buffer
	m_shadingModelUbo = Buffer(
	    vk, sizeof(ShadingModelUboStruct),
	    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkDescriptorBufferInfo shadingModelUboInfo = {};
	shadingModelUboInfo.buffer = m_shadingModelUbo;
	shadingModelUboInfo.offset = 0;
	shadingModelUboInfo.range = sizeof(ShadingModelUboStruct);

	vk::WriteDescriptorSet shadingModelUboWrite;
	shadingModelUboWrite.descriptorCount = 1;
	shadingModelUboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	shadingModelUboWrite.dstArrayElement = 0;
	shadingModelUboWrite.dstBinding = 3;
	shadingModelUboWrite.dstSet = m_descriptorSet;
	shadingModelUboWrite.pBufferInfo = &shadingModelUboInfo;

	m_shadingModelStaging =
	    StagingBuffer(vk, sizeof(sizeof(ShadingModelUboStruct)));
#pragma endregion

#pragma region point lights buffer
	m_pointLightsUbo = Buffer(
	    vk, sizeof(PointLightUboStruct) * m_maxPointLights,
	    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkDescriptorBufferInfo pointLightsUboInfo = {};
	pointLightsUboInfo.buffer = m_pointLightsUbo;
	pointLightsUboInfo.offset = 0;
	pointLightsUboInfo.range = sizeof(PointLightUboStruct) * m_maxPointLights;

	vk::WriteDescriptorSet pointLightsUboWrite;
	pointLightsUboWrite.descriptorCount = 1;
	pointLightsUboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pointLightsUboWrite.dstArrayElement = 0;
	pointLightsUboWrite.dstBinding = 4;
	pointLightsUboWrite.dstSet = m_descriptorSet;
	pointLightsUboWrite.pBufferInfo = &pointLightsUboInfo;

	m_pointLightsStaging =
	    StagingBuffer(vk, sizeof(PointLightUboStruct) * m_maxPointLights);
#pragma endregion

#pragma region directioanl lights buffer
	m_directionalLightsUbo = Buffer(
	    vk, sizeof(DirectionalLightUboStruct) * m_maxDirectionalLights,
	    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkDescriptorBufferInfo directionalLightsUboInfo = {};
	directionalLightsUboInfo.buffer = m_directionalLightsUbo;
	directionalLightsUboInfo.offset = 0;
	directionalLightsUboInfo.range =
	    sizeof(DirectionalLightUboStruct) * m_maxDirectionalLights;

	vk::WriteDescriptorSet directionalLightsUboWrite;
	directionalLightsUboWrite.descriptorCount = 1;
	directionalLightsUboWrite.descriptorType =
	    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	directionalLightsUboWrite.dstArrayElement = 0;
	directionalLightsUboWrite.dstBinding = 5;
	directionalLightsUboWrite.dstSet = m_descriptorSet;
	directionalLightsUboWrite.pBufferInfo = &directionalLightsUboInfo;

	m_directionalLightsStaging = StagingBuffer(
	    vk, sizeof(DirectionalLightUboStruct) * m_maxDirectionalLights);
#pragma endregion

	vk.updateDescriptorSets({ shadingModelUboWrite, pointLightsUboWrite,
	                          directionalLightsUboWrite });
}

void PbrShadingModel::uploadShadingModelDataStaging()
{
	auto& vk = *m_vulkanDevice.get();
	CommandBufferPool pool(vk, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	auto& frame = pool.getAvailableCommandBuffer();
	CommandBuffer& cb = frame.commandBuffer;

	VkBufferCopy region = {};
	region.size = sizeof(ShadingModelUboStruct);

	cb.begin();
	cb.copyBuffer(m_shadingModelStaging, m_shadingModelUbo, region);
	cb.end();

	if (frame.submit(vk.graphicsQueue()) != VK_SUCCESS)
		throw std::runtime_error("failed to submit copy command buffer");

	pool.waitForAllCommandBuffers();
}

void PbrShadingModel::uploadPointLightsStaging()
{
	auto& vk = *m_vulkanDevice.get();
	CommandBufferPool pool(vk, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	auto& frame = pool.getAvailableCommandBuffer();
	CommandBuffer& cb = frame.commandBuffer;

	VkBufferCopy region = {};
	region.size = sizeof(PointLightUboStruct) * m_maxPointLights;

	cb.begin();
	cb.copyBuffer(m_pointLightsStaging, m_pointLightsUbo, region);
	cb.end();

	if (frame.submit(vk.graphicsQueue()) != VK_SUCCESS)
		throw std::runtime_error("failed to submit copy command buffer");

	pool.waitForAllCommandBuffers();
}

void PbrShadingModel::uploadDirectionalLightsStaging()
{
	auto& vk = *m_vulkanDevice.get();
	CommandBufferPool pool(vk, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	auto& frame = pool.getAvailableCommandBuffer();
	CommandBuffer& cb = frame.commandBuffer;

	VkBufferCopy region = {};
	region.size = sizeof(DirectionalLightUboStruct) * m_maxDirectionalLights;

	cb.begin();
	cb.copyBuffer(m_directionalLightsStaging, m_directionalLightsUbo, region);
	cb.end();

	if (frame.submit(vk.graphicsQueue()) != VK_SUCCESS)
		throw std::runtime_error("failed to submit copy command buffer");

	pool.waitForAllCommandBuffers();
}

CombinedMaterialShadingFragmentFunction
PbrShadingModel::combinedMaterialFragmentFunction(
    sdw::FragmentWriter& writer, MaterialFragmentFunction& materialFunction,
    FragmentShaderBuildDataBase* shaderBuildData)
{
	FragmentShaderBuildData* buildData =
	    dynamic_cast<FragmentShaderBuildData*>(shaderBuildData);
	if (buildData == nullptr)
		throw std::runtime_error("invalid shaderBuildData type");

	buildData->irradianceMap = std::make_unique<SampledImageCubeRgba32>(
	    writer.declSampledImage<FImgCubeRgba32>("irradianceMap", 0, 1));

	buildData->prefilteredMap = std::make_unique<SampledImageCubeRgba32>(
	    writer.declSampledImage<FImgCubeRgba32>("prefilteredMap", 1, 1));

	buildData->brdfLut = std::make_unique<SampledImage2DRg32>(
	    writer.declSampledImage<FImg2DRg32>("brdfLut", 2, 1));

	buildData->shadingModelData = std::make_unique<sdw::Ubo>(
	    writer.declUniformBuffer<sdw::Ubo>("shadingModelData", 3, 1));
	buildData->shadingModelData->declMember<UInt>("pointLightsCount");
	buildData->shadingModelData->declMember<UInt>("directionalLightsCount");
	buildData->shadingModelData->end();

	buildData->pointLights = std::make_unique<ArraySsboT<shader::PointLights>>(
	    writer.declArrayShaderStorageBuffer<shader::PointLights>("pointLights",
	                                                             4, 1));

	buildData->directionalLights =
	    std::make_unique<ArraySsboT<shader::DirectionalLights>>(
	        writer.declArrayShaderStorageBuffer<shader::DirectionalLights>(
	            "directionalLights", 5, 1));

	buildData->PI =
	    std::make_unique<Float>(writer.declConstant("PI", 3.14159265359_f));

	buildData->DistributionGGX = writer.implementFunction<Float>(
	    "DistributionGGX",
	    [&writer, buildData](const Vec3& N, const Vec3& H,
	                         const Float& roughness) {
		    Locale(a, roughness * roughness);
		    Locale(a2, a * a);
		    Locale(NdotH, max(dot(N, H), 0.0_f));
		    Locale(NdotH2, NdotH * NdotH);

		    Locale(denom, NdotH2 * (a2 - 1.0_f) + 1.0_f);
		    denom = *buildData->PI * denom * denom;

		    writer.returnStmt(a2 / denom);
	    },
	    InVec3{ writer, "N" }, InVec3{ writer, "H" },
	    InFloat{ writer, "roughness" });

	buildData->GeometrySchlickGGX = writer.implementFunction<Float>(
	    "GeometrySchlickGGX",
	    [&writer, buildData](const Float& NdotV, const Float& roughness) {
		    Locale(r, roughness + 1.0_f);
		    Locale(k, (r * r) / 8.0_f);

		    Locale(denom, NdotV * (1.0_f - k) + k);

		    writer.returnStmt(NdotV / denom);
	    },
	    InFloat{ writer, "NdotV" }, InFloat{ writer, "roughness" });

	buildData->GeometrySmith = writer.implementFunction<Float>(
	    "GeometrySmith",
	    [&writer, buildData](const Vec3& N, const Vec3& V, const Vec3& L,
	                         const Float& roughness) {
		    Locale(NdotV, max(dot(N, V), 0.0_f));
		    Locale(NdotL, max(dot(N, L), 0.0_f));
		    Locale(ggx1, buildData->GeometrySchlickGGX(NdotV, roughness));
		    Locale(ggx2, buildData->GeometrySchlickGGX(NdotL, roughness));

		    writer.returnStmt(ggx1 * ggx2);
	    },
	    InVec3{ writer, "N" }, InVec3{ writer, "V" }, InVec3{ writer, "L" },
	    InFloat{ writer, "roughness" });

	buildData->fresnelSchlick = writer.implementFunction<Vec4>(
	    "fresnelSchlick",
	    [&writer, buildData](const Float& cosTheta, const Vec4& F0) {
		    Locale(oneMinusCosTheta, 1.0_f - cosTheta);
		    Locale(oneMinusCosThetaPowTwo,
		           oneMinusCosTheta * oneMinusCosTheta);
		    Locale(oneMinusCosThetaPowFive, oneMinusCosThetaPowTwo *
		                                        oneMinusCosThetaPowTwo *
		                                        oneMinusCosTheta);
		    writer.returnStmt(F0 + (vec4(1.0_f) - F0) *
		                               vec4(oneMinusCosThetaPowFive));
	    },
	    InFloat{ writer, "cosTheta" }, InVec4{ writer, "F0" });

	buildData->fresnelSchlickRoughness = writer.implementFunction<Vec4>(
	    "fresnelSchlickRoughness",
	    [&writer, buildData](const Float& cosTheta, const Vec4& F0,
	                         const Float& roughness) {
		    Locale(oneMinusCosTheta, 1.0_f - cosTheta);
		    Locale(oneMinusCosThetaPowTwo,
		           oneMinusCosTheta * oneMinusCosTheta);
		    Locale(oneMinusCosThetaPowFive, oneMinusCosThetaPowTwo *
		                                        oneMinusCosThetaPowTwo *
		                                        oneMinusCosTheta);
		    writer.returnStmt(F0 + (max(vec4(1.0_f - roughness), F0) - F0) *
		                               vec4(oneMinusCosThetaPowFive));
	    },
	    InFloat{ writer, "cosTheta" }, InVec4{ writer, "F0" },
	    InFloat{ writer, "roughness" });

	return writer.implementFunction<Vec4>(
	    "combinedMaterialShading",
	    [&writer, &materialFunction, buildData](
	        const UInt& inMaterialInstanceIndex, const Vec3& wsPosition_arg,
	        const Vec2& uv_arg, const Vec3& wsNormal_arg,
	        const Vec3& wsTangent_arg, const Vec3& wsViewPosition_arg) {
		    Locale(pi, *buildData->PI);
		    Locale(pointLightsCount,
		           buildData->shadingModelData->getMember<UInt>(
		               "pointLightsCount"));
		    Locale(directionalLightsCount,
		           buildData->shadingModelData->getMember<UInt>(
		               "directionalLightsCount"));
		    Locale(albedo, vec4(0.0_f));
		    Locale(wsPosition, wsPosition_arg);
		    Locale(wsNormal, wsNormal_arg);
		    Locale(wsTangent, wsTangent_arg);
		    Locale(wsViewPosition, wsViewPosition_arg);

		    Locale(metalness, 0.0_f);
		    Locale(roughness, 0.3_f);

		    Locale(instanceIndex, inMaterialInstanceIndex);

		    Locale(unused, materialFunction(instanceIndex, albedo, uv_arg,
		                                    wsPosition, wsNormal, wsTangent,
		                                    metalness, roughness));

		    Locale(V, normalize(wsViewPosition - wsPosition));

		    Locale(F0, mix(vec4(0.04_f), albedo, vec4(metalness)));

		    Locale(Lo, vec4(0.0_f));

		    Locale(F, vec4(0.0_f));
		    Locale(kS, vec4(0.0_f));
		    Locale(kD, vec4(0.0_f));
		    Locale(specular, vec4(0.0_f));

		    FOR(writer, UInt, i, 0_u, i < pointLightsCount, ++i)
		    {
			    Locale(wsLightPos,
			           buildData->pointLights->operator[](i).position);
			    Locale(lightColor,
			           buildData->pointLights->operator[](i).color);
			    Locale(L, normalize(wsLightPos - wsPosition));
			    Locale(H, normalize(V + L));
			    Locale(distance, length(wsLightPos - wsPosition));
			    Locale(attenuation, 1.0_f / (distance * distance));
			    Locale(radiance, lightColor * vec4(attenuation));

			    Locale(NDF,
			           buildData->DistributionGGX(wsNormal, H, roughness));
			    Locale(G, buildData->GeometrySmith(wsNormal, V, L, roughness));
			    F = buildData->fresnelSchlick(max(dot(H, V), 0.0_f), F0);

			    kS = F;
			    kD = vec4(1.0_f) - kS;
			    kD = kD * vec4(1.0_f - metalness);

			    Locale(nominator, NDF * G * F);
			    Locale(denominator, 4.0_f * max(dot(wsNormal, V), 0.0_f) *
			                            max(dot(wsNormal, L), 0.0_f));
			    Locale(loopSpecular,
			           nominator / vec4(max(denominator, 0.001_f)));

			    Locale(NdotL, max(dot(wsNormal, L), 0.0_f));

			    Lo += (kD * albedo / vec4(pi) + loopSpecular) * radiance *
			          vec4(NdotL);
		    }
		    ROF;
		    FOR(writer, UInt, i, 0_u, i < directionalLightsCount, ++i)
		    {
			    Locale(radiance,
			           buildData->directionalLights->operator[](i).color);
			    Locale(L,
			           normalize(-buildData->directionalLights->operator[](i)
			                          .direction));
			    Locale(H, normalize(V + L));

			    Locale(NDF,
			           buildData->DistributionGGX(wsNormal, H, roughness));
			    Locale(G, buildData->GeometrySmith(wsNormal, V, L, roughness));
			    F = buildData->fresnelSchlick(max(dot(H, V), 0.0_f), F0);

			    kS = F;
			    kD = vec4(1.0_f) - kS;
			    kD = kD * vec4(1.0_f - metalness);

			    Locale(nominator, NDF * G * F);
			    Locale(denominator, 4.0_f * max(dot(wsNormal, V), 0.0_f) *
			                            max(dot(wsNormal, L), 0.0_f));
			    Locale(loopSpecular,
			           nominator / vec4(max(denominator, 0.001_f)));

			    Locale(NdotL, max(dot(wsNormal, L), 0.0_f));

			    Lo += (kD * albedo / vec4(pi) + loopSpecular) * radiance *
			          vec4(NdotL);
		    }
		    ROF;

		    F = buildData->fresnelSchlickRoughness(
		        max(dot(wsNormal, V), 0.0_f), F0, roughness);

		    kS = F;
		    kD = vec4(1.0_f) - kS;
		    kD = kD * vec4(1.0_f - metalness);

		    Locale(R, reflect(-V, wsNormal));

		    Locale(irradiance, buildData->irradianceMap->sample(wsNormal));
		    Locale(diffuse, irradiance * albedo);

		    Locale(MAX_REFLECTION_LOD,
		           writer.cast<Float>(buildData->prefilteredMap->getLevels()));
		    Locale(prefilteredColor, buildData->prefilteredMap->lod(
		                                 R, roughness * MAX_REFLECTION_LOD));
		    Locale(brdf,
		           buildData->brdfLut
		               ->sample(vec2(max(dot(wsNormal, V), 0.0_f), roughness))
		               .rg());
		    specular = prefilteredColor * (F * brdf.x() + brdf.y());

		    Locale(ambient, kD * diffuse + specular);

		    Locale(color, ambient + Lo);

		    writer.returnStmt(color);
		    // writer.returnStmt(wsLightPos);
	    },
	    InUInt{ writer, "inMaterialInstanceIndex" },
	    InVec3{ writer, "wsPosition_arg" }, InVec2{ writer, "uv_arg" },
	    InVec3{ writer, "wsNormal_arg" }, InVec3{ writer, "wsTangent_arg" },
	    InVec3{ writer, "wsViewPosition_arg" });
}

std::unique_ptr<PbrShadingModel::FragmentShaderBuildDataBase>
PbrShadingModel::instantiateFragmentShaderBuildData()
{
	return std::make_unique<FragmentShaderBuildData>();
}
}  // namespace cdm
