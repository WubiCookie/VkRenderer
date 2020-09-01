#include "PbrShadingModel.hpp"

#include "Material.hpp"
#include "RenderWindow.hpp"

#include <iostream>

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

	std::unique_ptr<sdw::Float> PI;

	DistributionGGX_Signature DistributionGGX;
	GeometrySchlickGGX_Signature GeometrySchlickGGX;
	GeometrySmith_Signature GeometrySmith;
	fresnelSchlick_Signature fresnelSchlick;
	fresnelSchlickRoughness_Signature fresnelSchlickRoughness;
};

PbrShadingModel::PbrShadingModel(RenderWindow& renderWindow)
    : rw(&renderWindow)
{
	auto& vk = rw.get()->device();

#pragma region descriptor pool
	std::array poolSizes{
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
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

		std::array layoutBindings{ layoutBindingIrradianceMapImages,
			                       layoutBindingPrefilteredMapImages,
			                       layoutBindingBrdfLutImages };

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
	        const UInt& inMaterialInstanceIndex, const Vec2& UV,
	        const Vec3& wsPosition_arg, const Vec3& wsNormal_arg,
	        const Vec3& wsTangent_arg, const Vec3& wsViewPosition_arg) {
		    Locale(albedo, vec4(0.0_f));
		    Locale(wsPosition, wsPosition_arg);
		    Locale(wsNormal, wsNormal_arg);
		    Locale(wsTangent, wsTangent_arg);
		    Locale(wsViewPosition, wsViewPosition_arg);

		    Locale(metalness, 0.0_f);
		    Locale(roughness, 0.3_f);

		    Locale(instanceIndex, inMaterialInstanceIndex);

		    Locale(unused, materialFunction(instanceIndex, UV, albedo,
		                                    wsPosition, wsNormal, wsTangent,
		                                    metalness, roughness));

		    Locale(V, normalize(wsViewPosition - wsPosition));

		    Locale(F0, mix(vec4(0.04_f), albedo, vec4(metalness)));

		    Locale(Lo, vec4(0.0_f));

		    Locale(F, vec4(0.0_f));
		    Locale(kS, vec4(0.0_f));
		    Locale(kD, vec4(0.0_f));
		    Locale(specular, vec4(0.0_f));

		    // FOR
		    // Locale(L, normalize(fragTanLightPos - fragTanFragPos));
		    // Locale(H, normalize(V + L));
		    // Locale(distance, length(fragTanLightPos - fragTanFragPos));
		    Locale(L, normalize(vec3(0.0_f, 20.0_f, 0.0_f) - wsPosition));
		    Locale(H, normalize(V + L));
		    Locale(distance, length(vec3(0.0_f, 20.0_f, 0.0_f) - wsPosition));

		    Locale(attenuation, 1.0_f / (distance * distance));
		    Locale(radiance, vec4( 1000.0_f * attenuation));

		    // Locale(NDF, DistributionGGX(N, H, roughness));
		    // Locale(G, GeometrySmith(N, V, L, roughness));
		    // F = fresnelSchlick(max(dot(H, V), 0.0_f), F0);
		    Locale(NDF, buildData->DistributionGGX(wsNormal, H, roughness));
		    Locale(G, buildData->GeometrySmith(wsNormal, V, L, roughness));
		    F = buildData->fresnelSchlick(max(dot(H, V), 0.0_f), F0);

		    Locale(nominator, NDF * G * F);
		    Locale(denominator, 4.0_f * max(dot(wsNormal, V), 0.0_f) *
		                                max(dot(wsNormal, L), 0.0_f) +
		                            0.001_f);
		    specular = nominator / vec4(denominator);

		    kS = F;
		    kD = vec4(1.0_f) - kS;
		    kD = kD * vec4(1.0_f - metalness);

		    Locale(NdotL, max(dot(wsNormal, L), 0.0_f));

		    Lo += (kD * albedo / vec4(*buildData->PI) + specular) * radiance *
		          vec4(NdotL);
		    // ROF

		    F = buildData->fresnelSchlickRoughness(
		        max(dot(wsNormal, V), 0.0_f), F0, roughness);

		    kS = F;
		    kD = vec4(1.0_f) - kS;
		    kD = kD * vec4(1.0_f - metalness);

		    Locale(R, reflect(-V, wsNormal));

		    Locale(irradiance, texture(*buildData->irradianceMap, wsNormal));
		    Locale(diffuse, irradiance * albedo);

		    Locale(MAX_REFLECTION_LOD, writer.cast<Float>(textureQueryLevels(
		                                   *buildData->prefilteredMap)));
		    Locale(prefilteredColor,
		           textureLod(*buildData->prefilteredMap, R,
		                      roughness * MAX_REFLECTION_LOD));
		    Locale(brdf, texture(*buildData->brdfLut,
		                         vec2(max(dot(wsNormal, V), 0.0_f), roughness))
		                     .rg());
		    specular = prefilteredColor * (F * brdf.x() + brdf.y());

		    Locale(ambient, kD * diffuse + specular);

		    Locale(color, ambient + Lo);

			//color.xyz() = wsNormal;

		    writer.returnStmt(color);
	    },
	    InUInt{ writer, "inMaterialInstanceIndex" }, InVec2{ writer, "UV" },
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
