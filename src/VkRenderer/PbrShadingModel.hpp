#pragma once

#include "MyShaderWriter.hpp"
#include "VulkanDevice.hpp"
#include "StagingBuffer.hpp"
#include "cdm_maths.hpp"

#include <memory>

namespace cdm
{
class Material;

class PbrShadingModel final
{
	Movable<const VulkanDevice*> m_vulkanDevice;

	Buffer m_shadingModelUbo;

	struct ShadingModelUboStruct
	{
		uint32_t pointLightsCount = 0;
		float _0 = float(0xcccc);
		float _1 = float(0xcccc);
		float _2 = float(0xcccc);
	};

	uint32_t m_maxPointLights = 0;
	Buffer m_pointLightsUbo;
public:
	struct FragmentShaderBuildDataBase {
		virtual ~FragmentShaderBuildDataBase() {}
	};

	StagingBuffer m_pointLightsStaging;

	struct PointLightUboStruct
	{
		vector3 position;
		float _0 = float(0xcccc);

		vector4 color;

		float intensity = 1.0f;
		float _1 = float(0xcccc);
		float _2 = float(0xcccc);
		float _3 = float(0xcccc);
	};

	UniqueDescriptorPool m_descriptorPool;
	UniqueDescriptorSetLayout m_descriptorSetLayout;
	Movable<VkDescriptorSet> m_descriptorSet;

	PbrShadingModel() = default;
	PbrShadingModel(const VulkanDevice& vulkanDevice, uint32_t maxPointLights);
	PbrShadingModel(const PbrShadingModel&) = delete;
	PbrShadingModel(PbrShadingModel&&) = default;
	~PbrShadingModel() = default;

	PbrShadingModel& operator=(const PbrShadingModel&) = delete;
	PbrShadingModel& operator=(PbrShadingModel&&) = default;

	void uploadPointLightsStagin();

	CombinedMaterialShadingFragmentFunction combinedMaterialFragmentFunction(
	    sdw::FragmentWriter& writer,
	    MaterialFragmentFunction& materialFunction,
	    FragmentShaderBuildDataBase* shaderBuildData);

	std::unique_ptr<FragmentShaderBuildDataBase>
		instantiateFragmentShaderBuildData();
};
}  // namespace cdm
