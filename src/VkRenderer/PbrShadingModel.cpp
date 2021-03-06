#include "PbrShadingModel.hpp"

#include "MyShaderWriter.hpp"
#include "CommandBuffer.hpp"
#include "CommandBufferPool.hpp"
#include "Material.hpp"

#include <iostream>

using namespace sdw;

namespace shader
{
struct ShadingModelData : public StructInstance
{
	ShadingModelData(sdw::ShaderWriter& writer, ast::expr::ExprPtr expr)
	    : StructInstance{ writer, std::move(expr) },
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
	PointLights(sdw::ShaderWriter& writer, ast::expr::ExprPtr expr)
	    : StructInstance{ writer, std::move(expr) },
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
	DirectionalLights(sdw::ShaderWriter& writer, ast::expr::ExprPtr expr)
	    : StructInstance{ writer, std::move(expr) },
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
using DistributionBeckmann_Signature =
    Function<Float, InVec3, InFloat, InFloat>;
using GeometrySchlickGGX_Signature = Function<Float, InFloat, InFloat>;
using GeometrySmith_Signature =
    Function<Float, InVec3, InVec3, InVec3, InFloat>;
using Lambda_Signature = Function<Float, InVec3, InFloat, InFloat>;
using GeometryBeckmann_Signature =
    Function<Float, InVec3, InVec3, InFloat, InFloat>;
using fresnelSchlick_Signature = Function<Vec4, InFloat, InVec4>;
using fresnelSchlickRoughness_Signature =
    Function<Vec4, InFloat, InVec4, InFloat>;
using LTC_Evaluate_Signature = Function<Vec3, InVec3, InVec3, InVec3, InMat3>;

struct FragmentShaderBuildData : PbrShadingModel::FragmentShaderBuildDataBase
{
	std::unique_ptr<SampledImageCubeRgba32> irradianceMap;
	std::unique_ptr<SampledImageCubeRgba32> prefilteredMap;
	std::unique_ptr<SampledImage2DRg32> brdfLut;
	std::unique_ptr<SampledImage2DRgba32> ltcMat;
	std::unique_ptr<SampledImage2DRg32> ltcAmp;

	std::unique_ptr<sdw::Ubo> shadingModelData;
	std::unique_ptr<ArraySsboT<shader::PointLights>> pointLights;
	std::unique_ptr<ArraySsboT<shader::DirectionalLights>> directionalLights;

	std::unique_ptr<Float> PI;

	// std::unique_ptr<Scene::SceneUbo> sceneUbo;
	std::unique_ptr<SampledImage2DShadowR32> shadowmap;

	DistributionGGX_Signature DistributionGGX;
	DistributionBeckmann_Signature DistributionBeckmann;
	GeometrySchlickGGX_Signature GeometrySchlickGGX;
	GeometrySmith_Signature GeometrySmith;
	Lambda_Signature Lambda;
	GeometryBeckmann_Signature GeometryBeckmann;
	fresnelSchlick_Signature fresnelSchlick;
	fresnelSchlickRoughness_Signature fresnelSchlickRoughness;
	LTC_Evaluate_Signature LTC_Evaluate;
	//LTC_Evaluate_Signature LTC_Evaluate_spec;
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
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
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
		    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		layoutBindingPointLightsBuffer.stageFlags =
		    VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding layoutBindingDirectionalLightsBuffer{};
		layoutBindingDirectionalLightsBuffer.binding = 5;
		layoutBindingDirectionalLightsBuffer.descriptorCount = 1;
		layoutBindingDirectionalLightsBuffer.descriptorType =
		    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		layoutBindingDirectionalLightsBuffer.stageFlags =
		    VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding layoutBindingLtcMatImage{};
		layoutBindingLtcMatImage.binding = 6;
		layoutBindingLtcMatImage.descriptorCount = 1;
		layoutBindingLtcMatImage.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindingLtcMatImage.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding layoutBindingLtcAmpImage{};
		layoutBindingLtcAmpImage.binding = 7;
		layoutBindingLtcAmpImage.descriptorCount = 1;
		layoutBindingLtcAmpImage.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindingLtcAmpImage.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array layoutBindings{ layoutBindingIrradianceMapImages,
			                       layoutBindingPrefilteredMapImages,
			                       layoutBindingBrdfLutImages,
			                       layoutBindingShadingModelBuffer,
			                       layoutBindingPointLightsBuffer,
			                       layoutBindingDirectionalLightsBuffer,
			                       layoutBindingLtcMatImage,
			                       layoutBindingLtcAmpImage };

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
	m_shadingModelUbo.setName("PbrShadingModel shadingModelUbo buffer");

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

	m_shadingModelStaging = StagingBuffer(vk, sizeof(ShadingModelUboStruct));
	m_shadingModelStaging.setName(
	    "PbrShadingModel shadingModelStaging buffer");
#pragma endregion

#pragma region point lights buffer
	m_pointLightsUbo = Buffer(
	    vk, sizeof(PointLightUboStruct) * m_maxPointLights,
	    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_pointLightsUbo.setName("PbrShadingModel pointLightsUbo buffer");

	VkDescriptorBufferInfo pointLightsUboInfo = {};
	pointLightsUboInfo.buffer = m_pointLightsUbo;
	pointLightsUboInfo.offset = 0;
	pointLightsUboInfo.range = sizeof(PointLightUboStruct) * m_maxPointLights;

	vk::WriteDescriptorSet pointLightsUboWrite;
	pointLightsUboWrite.descriptorCount = 1;
	pointLightsUboWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pointLightsUboWrite.dstArrayElement = 0;
	pointLightsUboWrite.dstBinding = 4;
	pointLightsUboWrite.dstSet = m_descriptorSet;
	pointLightsUboWrite.pBufferInfo = &pointLightsUboInfo;

	m_pointLightsStaging =
	    StagingBuffer(vk, sizeof(PointLightUboStruct) * m_maxPointLights);
	m_pointLightsStaging.setName("PbrShadingModel pointLightsStaging buffer");
#pragma endregion

#pragma region directional lights buffer
	m_directionalLightsUbo = Buffer(
	    vk, sizeof(DirectionalLightUboStruct) * m_maxDirectionalLights,
	    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_directionalLightsUbo.setName(
	    "PbrShadingModel directionalLightsUbo buffer");

	VkDescriptorBufferInfo directionalLightsUboInfo = {};
	directionalLightsUboInfo.buffer = m_directionalLightsUbo;
	directionalLightsUboInfo.offset = 0;
	directionalLightsUboInfo.range =
	    sizeof(DirectionalLightUboStruct) * m_maxDirectionalLights;

	vk::WriteDescriptorSet directionalLightsUboWrite;
	directionalLightsUboWrite.descriptorCount = 1;
	directionalLightsUboWrite.descriptorType =
	    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	directionalLightsUboWrite.dstArrayElement = 0;
	directionalLightsUboWrite.dstBinding = 5;
	directionalLightsUboWrite.dstSet = m_descriptorSet;
	directionalLightsUboWrite.pBufferInfo = &directionalLightsUboInfo;

	m_directionalLightsStaging = StagingBuffer(
	    vk, sizeof(DirectionalLightUboStruct) * m_maxDirectionalLights);
	m_directionalLightsStaging.setName(
	    "PbrShadingModel directionalLightsStaging buffer");
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
    FragmentShaderBuildDataBase* shaderBuildData, Scene::SceneUbo& sceneUbo)
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

	buildData->ltcMat = std::make_unique<SampledImage2DRgba32>(
	    writer.declSampledImage<FImg2DRgba32>("ltcMat", 6, 1));

	buildData->ltcAmp = std::make_unique<SampledImage2DRg32>(
	    writer.declSampledImage<FImg2DRg32>("ltcAmp", 7, 1));

	buildData->PI =
	    std::make_unique<Float>(writer.declConstant("PI", 3.14159265359_f));

	buildData->shadowmap = std::make_unique<SampledImage2DShadowR32>(
	    writer.declSampledImage<FImg2DShadowR32>("shadowmap", 2, 0));

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

	buildData->DistributionBeckmann = writer.implementFunction<Float>(
	    "DistributionBeckmann",
	    [&writer, buildData](const Vec3& wh, const Float& alphax,
	                         const Float& alphay) {
		    Locale(cosThetah, wh.z());
		    Locale(cos2Thetah, cosThetah * cosThetah);
		    Locale(sin2Thetah, max(0.0_f, 1.0_f - cos2Thetah));
		    Locale(tan2Thetah, sin2Thetah / cos2Thetah);
		    Locale(sinThetah, sqrt(sin2Thetah));

		    Locale(cosPhih,
		           TERNARY(writer, Float, sinThetah == 0.0_f, 1.0_f,
		                   sdw::clamp(wh.x() / sinThetah, -1.0_f, 1.0_f)));
		    Locale(sinPhih,
		           TERNARY(writer, Float, sinThetah == 0.0_f, 0.0_f,
		                   sdw::clamp(wh.y() / sinThetah, -1.0_f, 1.0_f)));
		    Locale(cos2Phih, cosPhih * cosPhih);
		    Locale(sin2Phih, sinPhih * sinPhih);

		    IF(writer, sdw::isinf(tan2Thetah)) { writer.returnStmt(0.0_f); }
		    ELSE
		    {
			    Locale(cos4Thetah,
			           cosThetah * cosThetah * cosThetah * cosThetah);
			    writer.returnStmt(
			        sdw::exp(-tan2Thetah * (cos2Phih / (alphax * alphax) +
			                                sin2Phih / (alphay * alphay))) /
			        //(cos2Phih / alphaX2 + sin2Phih / alphaY2)) /
			        //(*buildData->PI * alphaX * alphaY * cos4Thetah);
			        (3.14159_f * alphax * alphay * cos4Thetah));
		    }
		    FI;
	    },
	    InVec3{ writer, "wh" }, InFloat{ writer, "alphax" },
	    InFloat{ writer, "alphay" });

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

	buildData->Lambda = writer.implementFunction<Float>(
	    "Lambda",
	    [&writer, buildData](const Vec3& w, const Float& alphax,
	                         const Float& alphay) {
		    Locale(cosTheta, w.z());
		    Locale(cos2Theta, cosTheta * cosTheta);
		    Locale(sin2Theta, max(0.0_f, 1.0_f - cos2Theta));
		    Locale(sinTheta, sqrt(sin2Theta));
		    Locale(tanTheta, sinTheta / cosTheta);
		    Locale(absTanTheta, abs(tanTheta));
		    Locale(cosPhi,
		           TERNARY(writer, Float, sinTheta == 0.0_f, 1.0_f,
		                   sdw::clamp(w.x() / sinTheta, -1.0_f, 1.0_f)));
		    Locale(sinPhi,
		           TERNARY(writer, Float, sinTheta == 0.0_f, 0.0_f,
		                   sdw::clamp(w.y() / sinTheta, -1.0_f, 1.0_f)));
		    Locale(cos2Phi, cosPhi * cosPhi);
		    Locale(sin2Phi, sinPhi * sinPhi);

		    IF(writer, isinf(absTanTheta)) { writer.returnStmt(0.0_f); }
		    ELSE
		    {
			    Locale(alpha, sqrt(cos2Phi * (alphax * alphax) +
			                       sin2Phi * (alphay * alphay)));
			    Locale(a, 1.0_f / (alpha * absTanTheta));
			    IF(writer, a >= 1.6_f) { writer.returnStmt(0.0_f); }
			    ELSE
			    {
				    writer.returnStmt((1.0_f - 1.259_f * a + 0.396_f * a * a) /
				                      (3.535_f * a + 2.181_f * a * a));
			    }
			    FI;
		    }
		    FI;
	    },
	    InVec3{ writer, "w" }, InFloat{ writer, "alphax" },
	    InFloat{ writer, "alphay" });

	buildData->GeometryBeckmann = writer.implementFunction<Float>(
	    "GeometryBeckmann",
	    [&writer, buildData](const Vec3& wo, const Vec3& wi,
	                         const Float& alphax, const Float& alphay) {
		    writer.returnStmt(1.0_f /
		                      (1.0_f + buildData->Lambda(wo, alphax, alphay) +
		                       buildData->Lambda(wi, alphax, alphay)));
	    },
	    InVec3{ writer, "wo" }, InVec3{ writer, "wi" },
	    InFloat{ writer, "alphax" }, InFloat{ writer, "alphay" });

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

	buildData->LTC_Evaluate = writer.implementFunction<Vec3>(
	    "LTC_Evaluate",
	    [&writer, buildData, &sceneUbo](const Vec3& N, const Vec3& V,
	                                    const Vec3& P, const Mat3& Minv) {
		    Locale(T1, normalize(V - N * dot(V, N)));
		    Locale(T2, normalize(cross(N, T1)));
		    // Locale(T1, normalize(O - wsNormal * dot(O, wsNormal)));
		    // Locale(T2, cross(wsNormal, T1));

		    Locale(M, Minv * transpose(mat3(T1, T2, N)));

		    Locale(sigma, sceneUbo.getSigma());

		    Locale(lightRotx,
		           transpose(mat3(vec3(1.0_f, 0.0_f, 0.0_f),
		                          vec3(0.0_f, cos(sigma), -sin(sigma)),
		                          vec3(0.0_f, sin(sigma), cos(sigma)))));

		    // Locale(lightRoty,
		    //       transpose(mat3(vec3(cos(M[0][1]), 0.0_f, sin(M[0][1])),
		    //                      vec3(0.0_f, 1.0_f, 0.0_f),
		    //                      vec3(-sin(M[0][1]), 0.0_f,
		    //                      cos(M[0][1])))));

		    Locale(lightRotz,
		           transpose(mat3(vec3(cos(sigma), -sin(sigma), 0.0_f),
		                          vec3(sin(sigma), cos(sigma), 0.0_f),
		                          vec3(0.0_f, 0.0_f, 1.0_f))));

		    // Locale(lightTr, lightRotz * lightRotx * lightRoty);
		    Locale(lightTr, lightRotz);

		    Locale(P1, vec3(10.0_f, 0.0_f, 10.0_f));
		    Locale(P2, vec3(-10.0_f, 0.0_f, 10.0_f));
		    Locale(P3, vec3(-10.0_f, 0.0_f, -10.0_f));
		    Locale(P4, vec3(10.0_f, 0.0_f, -10.0_f));

		    Locale(L0, M * ((buildData->pointLights->operator[](0).position +
		                     (lightTr * P1)) -
		                    P));
		    Locale(L1, M * ((buildData->pointLights->operator[](0).position +
		                     (lightTr * P2)) -
		                    P));
		    Locale(L2, M * ((buildData->pointLights->operator[](0).position +
		                     (lightTr * P3)) -
		                    P));
		    Locale(L3, M * ((buildData->pointLights->operator[](0).position +
		                     (lightTr * P4)) -
		                    P));
		    Locale(L4, L3);

		    Locale(numVertices, 0_u);

#pragma region ClipQuadToHorizon
		    // detect clipping config
		    Locale(config, 0_u);
		    IF(writer, L0.z() > 0.0_f) { config += 1_u; }
		    FI;
		    IF(writer, L1.z() > 0.0_f) { config += 2_u; }
		    FI;
		    IF(writer, L2.z() > 0.0_f) { config += 4_u; }
		    FI;
		    IF(writer, L3.z() > 0.0_f) { config += 8_u; }
		    FI;

		    // clip
		    numVertices = 0_u;

		    IF(writer, config == 0_u)
		    {
			    // clip all
		    }
		    ELSEIF(config == 1_u)  // V1 clip V2 V3 V4
		    {
			    numVertices = 3_u;
			    L1 = -L1.z() * L0 + L0.z() * L1;
			    L2 = -L3.z() * L0 + L0.z() * L3;
		    }
		    ELSEIF(config == 2_u)  // V2 clip V1 V3 V4
		    {
			    numVertices = 3_u;
			    L0 = -L0.z() * L1 + L1.z() * L0;
			    L2 = -L2.z() * L1 + L1.z() * L2;
		    }
		    ELSEIF(config == 3_u)  // V1 V2 clip V3 V4
		    {
			    numVertices = 4_u;
			    L2 = -L2.z() * L1 + L1.z() * L2;
			    L3 = -L3.z() * L0 + L0.z() * L3;
		    }
		    ELSEIF(config == 4_u)  // V3 clip V1 V2 V4
		    {
			    numVertices = 3_u;
			    L0 = -L3.z() * L2 + L2.z() * L3;
			    L1 = -L1.z() * L2 + L2.z() * L1;
		    }
		    ELSEIF(config == 5_u)  // V1 V3 clip V2 V4) impossible
		    {
			    numVertices = 0_u;
		    }
		    ELSEIF(config == 6_u)  // V2 V3 clip V1 V4
		    {
			    numVertices = 4_u;
			    L0 = -L0.z() * L1 + L1.z() * L0;
			    L3 = -L3.z() * L2 + L2.z() * L3;
		    }
		    ELSEIF(config == 7_u)  // V1 V2 V3 clip V4
		    {
			    numVertices = 5_u;
			    L4 = -L3.z() * L0 + L0.z() * L3;
			    L3 = -L3.z() * L2 + L2.z() * L3;
		    }
		    ELSEIF(config == 8_u)  // V4 clip V1 V2 V3
		    {
			    numVertices = 3_u;
			    L0 = -L0.z() * L3 + L3.z() * L0;
			    L1 = -L2.z() * L3 + L3.z() * L2;
			    L2 = L3;
		    }
		    ELSEIF(config == 9_u)  // V1 V4 clip V2 V3
		    {
			    numVertices = 4_u;
			    L1 = -L1.z() * L0 + L0.z() * L1;
			    L2 = -L2.z() * L3 + L3.z() * L2;
		    }
		    ELSEIF(config == 10_u)  // V2 V4 clip V1 V3) impossible
		    {
			    numVertices = 0_u;
		    }
		    ELSEIF(config == 11_u)  // V1 V2 V4 clip V3
		    {
			    numVertices = 5_u;
			    L4 = L3;
			    L3 = -L2.z() * L3 + L3.z() * L2;
			    L2 = -L2.z() * L1 + L1.z() * L2;
		    }
		    ELSEIF(config == 12_u)  // V3 V4 clip V1 V2
		    {
			    numVertices = 4_u;
			    L1 = -L1.z() * L2 + L2.z() * L1;
			    L0 = -L0.z() * L3 + L3.z() * L0;
		    }
		    ELSEIF(config == 13_u)  // V1 V3 V4 clip V2
		    {
			    numVertices = 5_u;
			    L4 = L3;
			    L3 = L2;
			    L2 = -L1.z() * L2 + L2.z() * L1;
			    L1 = -L1.z() * L0 + L0.z() * L1;
		    }
		    ELSEIF(config == 14_u)  // V2 V3 V4 clip V1
		    {
			    numVertices = 5_u;
			    L4 = -L0.z() * L3 + L3.z() * L0;
			    L0 = -L0.z() * L1 + L1.z() * L0;
		    }
		    ELSEIF(config == 15_u)  // V1 V2 V3 V4
		    {
			    numVertices = 4_u;
		    }
		    FI;

		    IF(writer, numVertices == 3_u) { L3 = L0; }
		    FI;
		    IF(writer, numVertices == 4_u) { L4 = L0; }
		    FI;
#pragma endregion

		    Locale(schlick, vec2(0.0_f));

		    IF(writer, numVertices == 0_u) { writer.returnStmt(vec3(0.0_f)); }
		    FI;

			    L0 = normalize(L0);
			    L1 = normalize(L1);
			    L2 = normalize(L2);
			    L3 = normalize(L3);
			    L4 = normalize(L4);

			    Locale(sum, 0.0_f);
			    Locale(cosTheta, 0.0_f);
			    Locale(theta_2, 0.0_f);
			    Locale(res, 0.0_f);

			    Locale(v1, L0);
			    Locale(v2, L1);

			    cosTheta = dot(v1, v2);
			    // cosTheta = clamp(cosTheta, -0.9999_f, 0.9999_f);
			    theta_2 = acos(cosTheta);
			    sum += cross(v1, v2).z() * theta_2 / sin(theta_2);

			    v1 = L1;
			    v2 = L2;

			    cosTheta = dot(v1, v2);
			    // cosTheta = clamp(cosTheta, -0.9999_f, 0.9999_f);
			    theta_2 = acos(cosTheta);
			    sum += cross(v1, v2).z() * theta_2 / sin(theta_2);

			    v1 = L2;
			    v2 = L3;

			    cosTheta = dot(v1, v2);
			    // cosTheta = sdw::clamp(cosTheta, -0.9999_f, 0.9999_f);
			    theta_2 = acos(cosTheta);
			    sum += cross(v1, v2).z() * theta_2 / sin(theta_2);

			    IF(writer, numVertices >= 4_u)
			    {
				    v1 = L3;
				    v2 = L4;

				    cosTheta = dot(v1, v2);
				    // cosTheta = clamp(cosTheta, -0.9999_f, 0.9999_f);
				    theta_2 = acos(cosTheta);
				    sum += cross(v1, v2).z() * theta_2 / sin(theta_2);
			    }
			    FI;

			    IF(writer, numVertices == 5_u)
			    {
				    v1 = L4;
				    v2 = L0;

				    cosTheta = dot(v1, v2);
				    // cosTheta = clamp(cosTheta, -0.9999_f, 0.9999_f);
				    theta_2 = acos(cosTheta);
				    sum += cross(v1, v2).z() * theta_2 / sin(theta_2);
			    }
			    FI;

			    // sum = abs(sum);
			    sum = max(0.0_f, -sum);

			    writer.returnStmt(vec3(sum));
	    },
	    InVec3{ writer, "N" }, InVec3{ writer, "V" }, InVec3{ writer, "P" },
	    InMat3{ writer, "Minv" });

	return writer.implementFunction<Vec4>(
	    "combinedMaterialShading",
	    [&writer, &materialFunction, &sceneUbo, buildData](
	        const UInt& inMaterialInstanceIndex, const Vec3& wsPosition_arg,
	        const Vec2& uv_arg, const Vec3& wsNormal_arg,
	        const Vec3& wsTangent_arg) {
		    Locale(pi, *buildData->PI);
		    Locale(pointLightsCount,
		           buildData->shadingModelData->getMember<UInt>(
		               "pointLightsCount"));
		    Locale(directionalLightsCount,
		           buildData->shadingModelData->getMember<UInt>(
		               "directionalLightsCount"));
		    Locale(alpha, sceneUbo.getRoughness());
		    Locale(R, sceneUbo.getR());
		    Locale(albedo, vec4(0.0_f));
		    Locale(wsPosition, wsPosition_arg);
		    Locale(wsNormal, wsNormal_arg);
		    Locale(wsTangent, wsTangent_arg);
		    Locale(wsViewPosition, sceneUbo.getViewPos());

		    // Locale(B, cross(wsNormal, wsTangent));
		    Locale(TBN, transpose(mat3(wsTangent, cross(wsNormal, wsTangent),
		                               wsNormal)));

		    Locale(metalness, 0.0_f);
		    Locale(roughness, 0.3_f);

		    Locale(instanceIndex, inMaterialInstanceIndex);

		    Locale(unused, materialFunction(instanceIndex, albedo, uv_arg,
		                                    wsPosition, wsNormal, wsTangent,
		                                    metalness, roughness));

		    Locale(V, normalize(wsViewPosition - wsPosition));

		    // Locale(T1, normalize(V - wsNormal * dot(V, wsNormal)));
		    //   Locale(T2, cross(wsNormal, T1));

		    // TBN = transpose(mat3(T1, T2, wsNormal));

		    Locale(tsPosition, TBN * wsPosition);
		    Locale(tsNormal, TBN * wsNormal);
		    Locale(tsV, TBN * V);

		    Locale(F, vec4(0.0_f));
		    Locale(F0, mix(vec4(0.04_f), albedo, vec4(metalness)));

		    Locale(Lo, vec4(0.0_f));

		    Locale(kS, vec4(0.0_f));
		    Locale(kD, vec4(0.0_f));
		    Locale(specular, vec4(0.0_f));

		    Locale(shadow, 1.0_f);

		    Locale(M, inverse(sceneUbo.getLTDM()));
		    // mat3(vec3(0.1_f, 0.0_f, 0.0_f), vec3(0.0_f, 0.1_f, 0.0_f),
		    // vec3(0.0_f, 0.0_f, 1.0_f)));

		    // wsNormal = normalize(M * wsNormal);
		    // V = normalize(M * V);
		    // wsNormal = M * wsNormal;

		    Locale(wo, normalize(M * tsV));
		// Locale(wo, tsV);

#pragma region trigonometry o
		    Locale(cosThetao, wo.z());
		    Locale(cos2Thetao, cosThetao * cosThetao);
		    Locale(absCosThetao, abs(cosThetao));
		    Locale(sin2Thetao, max(0.0_f, 1.0_f - cos2Thetao));
		    Locale(sinThetao, sqrt(sin2Thetao));
		    // Locale(absSinThetao, abs(sinThetao));
		    Locale(tanThetao, sinThetao / cosThetao);
		    Locale(tan2Thetao, sin2Thetao / cos2Thetao);
		    Locale(absTanThetao, abs(tanThetao));

		    Locale(cosPhio,
		           TERNARY(writer, Float, sinThetao == 0.0_f, 1.0_f,
		                   sdw::clamp(wo.x() / sinThetao, -1.0_f, 1.0_f)));
		    Locale(sinPhio,
		           TERNARY(writer, Float, sinThetao == 0.0_f, 0.0_f,
		                   sdw::clamp(wo.y() / sinThetao, -1.0_f, 1.0_f)));
		    Locale(cos2Phio, cosPhio * cosPhio);
		    Locale(sin2Phio, sinPhio * sinPhio);
#pragma endregion

		    FOR(writer, UInt, i, 0_u, i < pointLightsCount, ++i)
		    {
			    Locale(wsLightPos,
			           buildData->pointLights->operator[](i).position);
			    Locale(tsLightPos, TBN * wsLightPos);
			    Locale(lightColor,
			           buildData->pointLights->operator[](i).color);

			    Locale(tsL, normalize(tsLightPos - tsPosition));

			    Locale(distance, length(tsLightPos - tsPosition));
			    Locale(attenuation, 1.0_f / (distance * distance));
			    Locale(radiance, lightColor * vec4(attenuation));

			    // Locale(wi, normalize(M * tsL));
			    Locale(wi, tsL);
			    Locale(wh, tsV + tsL);

			    IF(writer,
			       (wh.x() == 0.0_f && wh.y() == 0.0_f && wh.z() == 0.0_f))
			    {
				    writer.loopContinueStmt();
			    }
			    FI;

			    wh = normalize(M * wh);

#pragma region trigonometry i
			    Locale(cosThetai, wi.z());
			    Locale(cos2Thetai, cosThetai * cosThetai);
			    Locale(absCosThetai, abs(cosThetai));
			    Locale(sin2Thetai, max(0.0_f, 1.0_f - cos2Thetai));
			    Locale(sinThetai, sqrt(sin2Thetai));
			    // Locale(absSinThetai, abs(sinThetai));
			    Locale(tanThetai, sinThetai / cosThetai);
			    Locale(tan2Thetai, sin2Thetai / cos2Thetai);
			    Locale(absTanThetai, abs(tanThetai));

			    Locale(cosPhii,
			           TERNARY(writer, Float, sinThetai == 0.0_f, 1.0_f,
			                   sdw::clamp(wi.x() / sinThetai, -1.0_f, 1.0_f)));
			    Locale(sinPhii,
			           TERNARY(writer, Float, sinThetai == 0.0_f, 0.0_f,
			                   sdw::clamp(wi.y() / sinThetai, -1.0_f, 1.0_f)));
			    Locale(cos2Phii, cosPhii * cosPhii);
			    Locale(sin2Phii, sinPhii * sinPhii);
#pragma endregion

#pragma region trigonometry h
			    Locale(cosThetah, wh.z());
			    Locale(cos2Thetah, cosThetah * cosThetah);
			    Locale(absCosThetah, abs(cosThetah));
			    Locale(sin2Thetah, max(0.0_f, 1.0_f - cos2Thetah));
			    Locale(tan2Thetah, sin2Thetah / cos2Thetah);
			    Locale(sinThetah, sqrt(sin2Thetah));
			    // Locale(absSinThetah, abs(sinThetah));
			    Locale(cosPhih,
			           TERNARY(writer, Float, sinThetah == 0.0_f, 1.0_f,
			                   sdw::clamp(wh.x() / sinThetah, -1.0_f, 1.0_f)));
			    Locale(sinPhih,
			           TERNARY(writer, Float, sinThetah == 0.0_f, 0.0_f,
			                   sdw::clamp(wh.y() / sinThetah, -1.0_f, 1.0_f)));
			    Locale(cos2Phih, cosPhih * cosPhih);
			    Locale(sin2Phih, sinPhih * sinPhih);
#pragma endregion

			    //#pragma region Roughness Alpha
			    //			    // Locale(clampledRoughness,
			    // max(roughness, 1.0e-1_f)); Locale(clampledRoughnessX,
			    //			           max(sceneUbo.getRoughness(), 1.0e-1_f));
			    //			    Locale(clampledRoughnessY, clampledRoughnessX);
			    //			    // Locale(clampledRoughnessX,
			    // max(2.0_f, 1.0e-1_f));
			    //			    // Locale(clampledRoughnessY,
			    // max(0.8_f, 1.0e-1_f));
			    //
			    //			    Locale(x, log(clampledRoughnessX));
			    //			    Locale(distributionAlphaX, 1.062142_f +
			    // 0.819955_f
			    //* x + 			                                   0.1734_f
			    //*
			    // x
			    //* x + 0.0171201_f * x
			    //* x * x + 0.000640711_f * x * x * x * x);
			    //
			    //			    Locale(y, log(clampledRoughnessY));
			    //			    Locale(distributionAlphaY, 1.062142_f +
			    // 0.819955_f
			    //* y + 			                                   0.1734_f
			    //*
			    // y
			    //* y + 0.0171201_f * y
			    //* y * y + 0.000640711_f * y * y * y * y);
			    //
			    //			    // Locale(alphaX, distributionAlphaX);
			    //			    // Locale(alphaY, distributionAlphaY);
			    //
			    //			    Locale(alpha, sceneUbo.getRoughness());
			    //			    Locale(alphaX, sceneUbo.getRoughness());
			    //			    Locale(alphaY, sceneUbo.getRoughness());
			    //
			    //			    Locale(alphaX2, alphaX * alphaX);
			    //			    Locale(alphaY2, alphaY * alphaY);
			    //#pragma endregion
			    //#pragma region Oren and Nayar
			    //			    Locale(R, sceneUbo.getR());
			    //			    Locale(sigma, sceneUbo.getSigma());
			    //			    Locale(sigma2, sigma * sigma);
			    //			    Locale(A, 1.0_f - (sigma2 / (2.0_f * (sigma2 +
			    // 0.33_f)))); 			    Locale(B, (0.45_f * sigma2 /
			    // (sigma2
			    // + 0.09_f)));
			    //
			    //			    Locale(maxCos, 0.0_f);
			    //			    IF(writer, sinThetai > 1.0e-4_f && sinThetao
			    //> 1.0e-4_f)
			    //			    {
			    //				    Locale(sinPhii,
			    //				           TERNARY(
			    //				               writer, Float, sinThetai ==
			    // 0.0_f,
			    // 0.0_f, 				               sdw::clamp(wi.y() /
			    // sinThetai,
			    // -1.0_f, 1.0_f))); 				    Locale(sinPhio,
			    // TERNARY( 				               writer, Float,
			    // sinThetao == 0.0_f, 0.0_f,
			    // sdw::clamp(wo.y() / sinThetao, -1.0_f, 1.0_f)));
			    //
			    //				    Locale(cosPhii,
			    //				           TERNARY(
			    //				               writer, Float, sinThetai ==
			    // 0.0_f,
			    // 0.0_f, 				               sdw::clamp(wi.x() /
			    // sinThetai,
			    // -1.0_f, 1.0_f))); 				    Locale(cosPhio,
			    // TERNARY( 				               writer, Float,
			    // sinThetao == 0.0_f, 0.0_f,
			    // sdw::clamp(wo.x() / sinThetao, -1.0_f, 1.0_f)));
			    //
			    //				    Locale(dCos, cosPhii * cosPhio + sinPhii *
			    // sinPhio); 				    maxCos = max(0.0_f, dCos);
			    //			    }
			    //			    FI;
			    //
			    //			    FLOAT(sinAlpha);
			    //			    FLOAT(tanBeta);
			    //			    IF(writer, abs(wi.z()) > abs(wo.z()))
			    //			    {
			    //				    sinAlpha = sinThetao;
			    //				    tanBeta = sinThetai / abs(wi.z());
			    //			    }
			    //			    ELSE
			    //			    {
			    //				    sinAlpha = sinThetai;
			    //				    tanBeta = sinThetao / abs(wo.z());
			    //			    }
			    //			    FI;
			    //
			    //			    Locale(Lambertian, (R / *buildData->PI) *
			    //			                           (A + B * maxCos *
			    // sinAlpha
			    //* tanBeta)); #pragma endregion

			    IF(writer, cosThetai == 0.0_f || cosThetao == 0.0_f)
			    {
				    // Lo += vec4(0.0_f);
			    }
			    ELSE
			    {
				    // Locale(DBeckmann,
				    // buildData->DistributionBeckmann(wh, alpha, alpha));
				    // Locale(DBeckmann, max(0.0_f, normalize(M * wh).z()) /
				    // pi);
				    Locale(DBeckmann, max(0.0_f, wh.z()) / pi);

				    Locale(GBeckmann,
				           buildData->GeometryBeckmann(wo, wi, alpha, alpha));

				    F = buildData->fresnelSchlick(max(cosThetai, 0.0_f), F0);

				    kS = F;
				    kD = vec4(1.0_f) - kS;
				    kD = kD * vec4(1.0_f - metalness);

				    Locale(NdotL, max(dot(tsNormal, tsL), 0.0_f));

				    Lo +=
				        (radiance * NdotL) *
				        (kD * albedo / vec4(pi) + DBeckmann * GBeckmann * R) /
				        (4.0_f * cosThetai * absCosThetao + 0.1_f);
			    }
			    FI;
		    }
		    ROF;
		    FOR(writer, UInt, i, 0_u, i < directionalLightsCount, ++i)
		    {
			    Locale(radiance, (*buildData->directionalLights)[i].color);

			    Locale(
			        tsL,
			        normalize(TBN *
			                  -(*buildData->directionalLights)[i].direction));

			    // Locale(wi, normalize(M * tsL));
			    Locale(wi, tsL);
			    Locale(wh, tsV + tsL);

			    IF(writer,
			       (wh.x() == 0.0_f && wh.y() == 0.0_f && wh.z() == 0.0_f))
			    {
				    writer.loopContinueStmt();
			    }
			    FI;

			    wh = normalize(M * wh);

#pragma region trigonometry i
			    Locale(cosThetai, wi.z());
			    Locale(cos2Thetai, cosThetai * cosThetai);
			    Locale(absCosThetai, abs(cosThetai));
			    Locale(sin2Thetai, max(0.0_f, 1.0_f - cos2Thetai));
			    Locale(sinThetai, sqrt(sin2Thetai));
			    // Locale(absSinThetai, abs(sinThetai));
			    Locale(tanThetai, sinThetai / cosThetai);
			    Locale(tan2Thetai, sin2Thetai / cos2Thetai);
			    Locale(absTanThetai, abs(tanThetai));

			    Locale(cosPhii,
			           TERNARY(writer, Float, sinThetai == 0.0_f, 1.0_f,
			                   sdw::clamp(wi.x() / sinThetai, -1.0_f, 1.0_f)));
			    Locale(sinPhii,
			           TERNARY(writer, Float, sinThetai == 0.0_f, 0.0_f,
			                   sdw::clamp(wi.y() / sinThetai, -1.0_f, 1.0_f)));
			    Locale(cos2Phii, cosPhii * cosPhii);
			    Locale(sin2Phii, sinPhii * sinPhii);
#pragma endregion

#pragma region trigonometry h
			    Locale(cosThetah, wh.z());
			    Locale(cos2Thetah, cosThetah * cosThetah);
			    Locale(absCosThetah, abs(cosThetah));
			    Locale(sin2Thetah, max(0.0_f, 1.0_f - cos2Thetah));
			    Locale(tan2Thetah, sin2Thetah / cos2Thetah);
			    Locale(sinThetah, sqrt(sin2Thetah));
			    // Locale(absSinThetah, abs(sinThetah));
			    Locale(cosPhih,
			           TERNARY(writer, Float, sinThetah == 0.0_f, 1.0_f,
			                   sdw::clamp(wh.x() / sinThetah, -1.0_f, 1.0_f)));
			    Locale(sinPhih,
			           TERNARY(writer, Float, sinThetah == 0.0_f, 0.0_f,
			                   sdw::clamp(wh.y() / sinThetah, -1.0_f, 1.0_f)));
			    Locale(cos2Phih, cosPhih * cosPhih);
			    Locale(sin2Phih, sinPhih * sinPhih);
#pragma endregion

			    IF(writer, cosThetai == 0.0_f || cosThetao == 0.0_f)
			    {
				    // Lo += vec4(0.0_f);
			    }
			    ELSE
			    {
				    // ================= Shadow =================
				    Locale(shadowView, sceneUbo.getShadowView());
				    Locale(shadowProj, sceneUbo.getShadowProj());
				    Locale(shadowBias, sceneUbo.getShadowBias());

				    Locale(wsPositionDepthBiased,
				           wsPosition +
				               normalize(
				                   buildData->directionalLights->operator[](0)
				                       .direction) *
				                   shadowBias);

				    Locale(lsPositionW,
				           shadowProj * shadowView *
				               vec4(wsPositionDepthBiased, 1.0_f));
				    Locale(
				        lsPosition,
				        (lsPositionW.xyz() / lsPositionW.w()) * 0.5_f + 0.5_f);

				    shadow = buildData->shadowmap->sample(lsPosition.xy(),
				                                          lsPositionW.z());
				    // ==========================================

				    Locale(DBeckmann,
				           buildData->DistributionBeckmann(wh, alpha, alpha));

				    Locale(GBeckmann,
				           buildData->GeometryBeckmann(wo, wi, alpha, alpha));

				    F = buildData->fresnelSchlick(max(cosThetai, 0.0_f), F0);

				    kS = F;
				    kD = vec4(1.0_f) - kS;
				    kD = kD * vec4(1.0_f - metalness);

				    Locale(NdotL, max(dot(tsNormal, tsL), 0.0_f));

				    Lo +=
				        shadow * (radiance * NdotL) *
				        (kD * albedo / vec4(pi) + DBeckmann * GBeckmann * R) /
				        (4.0_f * cosThetai * absCosThetao + 0.1_f);
			    }
			    FI;
		    }
		    ROF;

		    // ==================== IBL =====================
		    F = buildData->fresnelSchlickRoughness(max(cosThetao, 0.0_f), F0,
		                                           roughness);

		    kS = F;
		    kD = vec4(1.0_f) - kS;
		    kD = kD * vec4(1.0_f - metalness);

		    Locale(reflected, reflect(-wo, tsNormal));

		    Locale(TBNMinusOne, inverse(TBN));

		    Locale(irradiance,
		           buildData->irradianceMap->sample(TBNMinusOne * tsNormal));
		    Locale(diffuse, irradiance * albedo);

		    Locale(MAX_REFLECTION_LOD,
		           writer.cast<Float>(buildData->prefilteredMap->getLevels()));
		    Locale(prefilteredColor,
		           buildData->prefilteredMap->lod(
		               reflected, roughness * MAX_REFLECTION_LOD));
		    Locale(brdf, buildData->brdfLut
		                     ->sample(vec2(max(cosThetao, 0.0_f), roughness))
		                     .rg());
		    specular = prefilteredColor * (F * brdf.x() + brdf.y());

		    Locale(ambient, kD * diffuse + specular);
		    // ==============================================

		    Locale(color, ambient + Lo);

		    // =============== RTPLS with LTC ===============
		    Locale(Lo_i_diff, vec3(0.0_f));
		    Locale(Lo_i_spec, vec3(0.0_f));
		    Locale(schlick, vec2(0.0_f));

		    Locale(P1, vec3(10.0_f, 0.0_f, 10.0_f));
		    Locale(P2, vec3(-10.0_f, 0.0_f, 10.0_f));
		    Locale(P3, vec3(-10.0_f, 0.0_f, -10.0_f));
		    Locale(P4, vec3(10.0_f, 0.0_f, -10.0_f));

			//*
		    Locale(sigma, sceneUbo.getSigma());

		    Locale(lightRotx,
		           transpose(mat3(vec3(1.0_f, 0.0_f, 0.0_f),
		                          vec3(0.0_f, cos(sigma), -sin(sigma)),
		                          vec3(0.0_f, sin(sigma), cos(sigma)))));

		    // Locale(lightRoty,
		    //       transpose(mat3(vec3(cos(M[0][1]), 0.0_f, sin(M[0][1])),
		    //                      vec3(0.0_f, 1.0_f, 0.0_f),
		    //                      vec3(-sin(M[0][1]), 0.0_f,
		    //                      cos(M[0][1])))));

		    Locale(lightRotz,
		           transpose(mat3(vec3(cos(sigma), -sin(sigma), 0.0_f),
		                          vec3(sin(sigma), cos(sigma), 0.0_f),
		                          vec3(0.0_f, 0.0_f, 1.0_f))));

		    // Locale(lightTr, lightRotz * lightRotx * lightRoty);
		    Locale(lightTr, lightRotz);
			//*/

		    Locale(P, wsPosition);
		    Locale(O, wsViewPosition - P);

		    // Locale(bg, cross(wsNormal, wsTangent));
		    // Locale(t2w, transpose(mat3(wsTangent, bg, wsNormal)));

		    // Locale(n, normalize(t2w * wsNormal));
		    // O = normalize(t2w * O);
		    Locale(n, wsNormal);
		    // IF (writer, dot(n, O) < 0.0_f)
		    //{
		    // n = normalize(n - 1.01_f * O * dot(n, O));
		    //}
		    // FI;


		    //metalness = 0.3_f;

		    Locale(diffColor, albedo * (1.0_f - metalness));
		    Locale(specColor,
		           mix(vec3(0.04_f), albedo.rgb(), vec3(metalness)));

		    // ======= LTC Coords
		    Locale(theta, acos(dot(n, O)));
		    // Locale(theta, acos(dot(wsNormal, O)));
		    Locale(coords,
		           //vec2(roughness, theta / (0.5_f * pi) + sceneUbo.getParam0()));
		           vec2(alpha, theta / (0.5_f * pi) + sceneUbo.getParam0()));
		    //Locale(coords, vec2(roughness, 1.0_f - (dot(n, O) / (0.5_f * pi))));
		    //Locale(coords, vec2(alpha / 0.7_f, theta / (0.5_f * pi)));

		    // Locale(LUTSize, 32.0_f);
		    // coords = coords * (LUTSize - 1.0_f) / LUTSize + 0.5_f / LUTSize;

		     Locale(Minv,
		          mat3(vec3(1.0_f, 0.0_f, 0.0_f), vec3(0.0_f, 1.0_f, 0.0_f),
		               vec3(0.0_f, 0.0_f, 1.0_f)));

			Lo_i_diff = buildData->LTC_Evaluate(n, O, P, Minv);
		     Lo_i_diff *= R;
		    Lo_i_diff *= diffColor.rgb();
		     Lo_i_diff /= (2.0_f * pi);

		    Locale(t, buildData->ltcMat->sample(coords));
		    Minv = transpose(mat3(vec3(1.0_f, 0.0_f, t.y()),
		                          vec3(0.0_f, t.z(), 0.0_f),
		                          vec3(t.w(), 0.0_f, t.x())));

		    Lo_i_spec = buildData->LTC_Evaluate(n, O, P, Minv);
		    Lo_i_spec *= R;

		    schlick = buildData->ltcAmp->sample(coords).rg();
		    Lo_i_spec *= specColor * schlick.r();//		    +(1.0_f - specColor) * schlick.g();
		    Lo_i_spec /= (2.0_f * pi);
			// ==============================================

		    // writer.returnStmt(color);
		    // writer.returnStmt(vec4(wsTangent, 1.0_f));
		    //writer.returnStmt(vec4(Lo_i, 1.0_f));
		    //writer.returnStmt(vec4(Lo_i_diff + Lo_i_spec, 1.0_f));
		    writer.returnStmt(vec4(Lo_i_spec + color.rgb(), 1.0_f));
		     //writer.returnStmt(vec4(n, 1.0_f));
		    //writer.returnStmt(vec4(
		        //vec3(dot(n, normalize(wsViewPosition - wsPosition))), 1.0_f));
		    // writer.returnStmt(vec4(schlick, 0.0_f, 1.0_f));
		     //writer.returnStmt(vec4(vec3(coords.y()), 1.0_f));
		    // writer.returnStmt(vec4(I));
	    },
	    InUInt{ writer, "inMaterialInstanceIndex" },
	    InVec3{ writer, "wsPosition_arg" }, InVec2{ writer, "uv_arg" },
	    InVec3{ writer, "wsNormal_arg" }, InVec3{ writer, "wsTangent_arg" });
}

std::unique_ptr<PbrShadingModel::FragmentShaderBuildDataBase>
PbrShadingModel::instantiateFragmentShaderBuildData()
{
	return std::make_unique<FragmentShaderBuildData>();
}
}  // namespace cdm
