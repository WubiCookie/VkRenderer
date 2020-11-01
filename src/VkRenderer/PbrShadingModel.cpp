#include "PbrShadingModel.hpp"

#include "CommandBuffer.hpp"
#include "CommandBufferPool.hpp"
#include "Material.hpp"

#include <iostream>

namespace shader
{
struct PointLights : public sdw::StructInstance
{
	PointLights(ast::Shader* shader, ast::expr::ExprPtr expr)
	    : StructInstance{ shader, std::move(expr) },
	      position{ getMember<sdw::Vec3>("position") },
	      color{ getMember<sdw::Vec4>("color") },
	      intensity{ getMember<sdw::Float>("intensity") }
	{
	}

	PointLights& operator=(PointLights const& rhs)
	{
		sdw::StructInstance::operator=(rhs);
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

	sdw::Vec3 position;
	sdw::Vec4 color;
	sdw::Float intensity;

private:
	using sdw::StructInstance::getMember;
	using sdw::StructInstance::getMemberArray;
};

Writer_Parameter(PointLights);

enum class TypeName
{
	ePointLights = 100,
};
}  // namespace shader

namespace sdw
{
template <>
struct TypeTraits<shader::PointLights>
{
	static ast::type::Kind constexpr TypeEnum =
	    ast::type::Kind(shader::TypeName::ePointLights);
};
}  // namespace sdw

namespace cdm
{
using DistributionGGX_Signature =
    sdw::Function<sdw::Float, sdw::InVec3, sdw::InVec3, sdw::InFloat>;
using GeometrySchlickGGX_Signature =
    sdw::Function<sdw::Float, sdw::InFloat, sdw::InFloat>;
using GeometrySmith_Signature =
    sdw::Function<sdw::Float, sdw::InVec3, sdw::InVec3, sdw::InVec3,
                  sdw::InFloat>;
using fresnelSchlick_Signature =
    sdw::Function<sdw::Vec4, sdw::InFloat, sdw::InVec4>;
using fresnelSchlickRoughness_Signature =
    sdw::Function<sdw::Vec4, sdw::InFloat, sdw::InVec4, sdw::InFloat>;

struct FragmentShaderBuildData : PbrShadingModel::FragmentShaderBuildDataBase
{
	std::unique_ptr<sdw::SampledImageCubeRgba32> irradianceMap;
	std::unique_ptr<sdw::SampledImageCubeRgba32> prefilteredMap;
	std::unique_ptr<sdw::SampledImage2DRg32> brdfLut;

	std::unique_ptr<sdw::ArraySsboT<shader::PointLights>> pointLights;

	std::unique_ptr<sdw::Float> PI;

	DistributionGGX_Signature DistributionGGX;
	GeometrySchlickGGX_Signature GeometrySchlickGGX;
	GeometrySmith_Signature GeometrySmith;
	fresnelSchlick_Signature fresnelSchlick;
	fresnelSchlickRoughness_Signature fresnelSchlickRoughness;
};

PbrShadingModel::PbrShadingModel(const VulkanDevice& vulkanDevice,
                                 uint32_t maxPointLights)
    : m_vulkanDevice(&vulkanDevice),
      m_maxPointLights(maxPointLights)
{
	auto& vk = *m_vulkanDevice.get();

#pragma region descriptor pool
	std::array poolSizes{
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
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

		// VkDescriptorSetLayoutBinding layoutBindingShadingModelBuffer{};
		// layoutBindingShadingModelBuffer.binding = 3;
		// layoutBindingShadingModelBuffer.descriptorCount = 1;
		// layoutBindingShadingModelBuffer.descriptorType =
		// VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		// layoutBindingShadingModelBuffer.stageFlags =
		// VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding layoutBindingLightsBuffer{};
		layoutBindingLightsBuffer.binding = 4;
		layoutBindingLightsBuffer.descriptorCount = 1;
		layoutBindingLightsBuffer.descriptorType =
		    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBindingLightsBuffer.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array layoutBindings{ layoutBindingIrradianceMapImages,
			                       layoutBindingPrefilteredMapImages,
			                       layoutBindingBrdfLutImages,
			                       // layoutBindingShadingModelBuffer,
			                       layoutBindingLightsBuffer };

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
#pragma endregion

#pragma region lights buffer
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

	// vk.updateDescriptorSets({ shadingModelUboWrite, pointLightsUboWrite });
	vk.updateDescriptorSets({ pointLightsUboWrite });
}

void PbrShadingModel::uploadPointLightsStagin()
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

CombinedMaterialShadingFragmentFunction
PbrShadingModel::combinedMaterialFragmentFunction(
    sdw::FragmentWriter& writer, MaterialFragmentFunction& materialFunction,
    FragmentShaderBuildDataBase* shaderBuildData)
{
	FragmentShaderBuildData* buildData =
	    dynamic_cast<FragmentShaderBuildData*>(shaderBuildData);
	if (buildData == nullptr)
		throw std::runtime_error("invalid shaderBuildData type");

	using namespace sdw;

	buildData->irradianceMap = std::make_unique<sdw::SampledImageCubeRgba32>(
	    writer.declSampledImage<FImgCubeRgba32>("irradianceMap", 0, 1));

	buildData->prefilteredMap = std::make_unique<sdw::SampledImageCubeRgba32>(
	    writer.declSampledImage<FImgCubeRgba32>("prefilteredMap", 1, 1));

	buildData->brdfLut = std::make_unique<sdw::SampledImage2DRg32>(
	    writer.declSampledImage<FImg2DRg32>("brdfLut", 2, 1));

	buildData->pointLights =
	    std::make_unique<sdw::ArraySsboT<shader::PointLights>>(
	        writer.declArrayShaderStorageBuffer<shader::PointLights>(
	            "pointLights", 4, 1));

	buildData->PI = std::make_unique<sdw::Float>(
	    writer.declConstant("PI", 3.14159265359_f));

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
		    Locale(oneMinusCosThetaPowFive,
		           oneMinusCosTheta * oneMinusCosTheta * oneMinusCosTheta *
		               oneMinusCosTheta * oneMinusCosTheta);
		    writer.returnStmt(F0 + (vec4(1.0_f) - F0) *
		                               vec4(pow(1.0_f - cosTheta, 5.0_f)));
	    },
	    InFloat{ writer, "cosTheta" }, InVec4{ writer, "F0" });

	buildData->fresnelSchlickRoughness = writer.implementFunction<Vec4>(
	    "fresnelSchlickRoughness",
	    [&writer, buildData](const Float& cosTheta, const Vec4& F0,
	                         const Float& roughness) {
		    writer.returnStmt(F0 + (max(vec4(1.0_f - roughness), F0) - F0) *
		                               vec4(pow(1.0_f - cosTheta, 5.0_f)));
	    },
	    InFloat{ writer, "cosTheta" }, InVec4{ writer, "F0" },
	    InFloat{ writer, "roughness" });

	return writer.implementFunction<Vec4>(
	    "combinedMaterialShading",
	    [&writer, &materialFunction, buildData](
	        const UInt& inMaterialInstanceIndex, const Vec3& wsPosition_arg,
	        const Vec3& wsNormal_arg, const Vec3& wsTangent_arg,
	        const Vec3& wsViewPosition_arg) {
		    Locale(pi, *buildData->PI);
		    Locale(albedo, vec4(0.0_f));
		    Locale(wsPosition, wsPosition_arg);
		    Locale(wsNormal, wsNormal_arg);
		    Locale(wsTangent, wsTangent_arg);
		    Locale(wsViewPosition, wsViewPosition_arg);

		    Locale(metalness, 0.0_f);
		    Locale(roughness, 0.3_f);

		    Locale(instanceIndex, inMaterialInstanceIndex);

		    Locale(unused, materialFunction(instanceIndex, albedo, wsPosition,
		                                    wsNormal, wsTangent, metalness,
		                                    roughness));

		    Locale(V, normalize(wsViewPosition - wsPosition));

		    Locale(F0, mix(vec4(0.04_f), albedo, vec4(metalness)));

		    Locale(Lo, vec4(0.0_f));

		    Locale(F, vec4(0.0_f));
		    Locale(kS, vec4(0.0_f));
		    Locale(kD, vec4(0.0_f));
		    Locale(specular, vec4(0.0_f));

		    // ========================================================
		    Locale(wsLightPos,
		           buildData->pointLights->operator[](0_u).position);
		    Locale(wsLightColor,
		           buildData->pointLights->operator[](0_u).color);
		    Locale(L, normalize(wsLightPos - wsPosition));
		    Locale(H, normalize(V + L));
		    Locale(distance, length(wsLightPos - wsPosition));
		    Locale(attenuation, 1.0_f / (distance * distance));
		    Locale(radiance, wsLightColor * vec4(attenuation));

		    Locale(NDF, buildData->DistributionGGX(wsNormal, H, roughness));
		    Locale(G, buildData->GeometrySmith(wsNormal, V, L, roughness));
		    F = buildData->fresnelSchlick(max(dot(H, V), 0.0_f), F0);

		    kS = F;
		    kD = vec4(1.0_f) - kS;
		    kD = kD * vec4(1.0_f - metalness);

		    Locale(nominator, NDF * G * F);
		    Locale(denominator, 4.0_f * max(dot(wsNormal, V), 0.0_f) *
		                            max(dot(wsNormal, L), 0.0_f));
		    Locale(loopSpecular, nominator / vec4(max(denominator, 0.001_f)));

		    Locale(NdotL, max(dot(wsNormal, L), 0.0_f));

		    Lo += (kD * albedo / vec4(pi) + loopSpecular) * radiance *
		          vec4(NdotL);
		    // ========================================================

		    // FOR
		    // Locale(L, normalize(fragTanLightPos - fragTanFragPos));
		    // Locale(H, normalize(V + L));
		    // Locale(distance, length(fragTanLightPos - fragTanFragPos));
		    // Locale(attenuation, 1.0_f / (distance * distance));
		    // Locale(radiance, vec4(attenuation));
		    //
		    // Locale(NDF, DistributionGGX(N, H, roughness));
		    // Locale(G, GeometrySmith(N, V, L, roughness));
		    // F = fresnelSchlick(max(dot(H, V), 0.0_f), F0);
		    //
		    // Locale(nominator, NDF * G * F);
		    // Locale(denominator,
		    //       4.0_f * max(dot(N, V), 0.0_f) * max(dot(N, L), 0.0_f)
		    //       +
		    //           0.001_f);
		    // Locale(specular, nominator / vec4(denominator));
		    //
		    // kS = F;
		    // kD = vec4(1.0_f) - kS;
		    // kD = kD * vec4(1.0_f - metalness);
		    //
		    // Locale(NdotL, max(dot(N, L), 0.0_f));
		    //
		    // Lo += (kD * albedo / vec4(PI) + specular) * radiance *
		    // vec4(NdotL);
		    // ROF

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
		    //writer.returnStmt(wsLightPos);
	    },
	    InUInt{ writer, "inMaterialInstanceIndex" },
	    InVec3{ writer, "wsPosition_arg" }, InVec3{ writer, "wsNormal_arg" },
	    InVec3{ writer, "wsTangent_arg" },
	    InVec3{ writer, "wsViewPosition_arg" });
}

std::unique_ptr<PbrShadingModel::FragmentShaderBuildDataBase>
PbrShadingModel::instantiateFragmentShaderBuildData()
{
	return std::make_unique<FragmentShaderBuildData>();
}
}  // namespace cdm
