#pragma once

#include "MyShaderWriter.hpp"
#include "VulkanDevice.hpp"

#include <memory>

namespace cdm
{
class Material;
class RenderWindow;

class PbrShadingModel final
{
	Movable<RenderWindow*> rw;

public:
	struct FragmentShaderBuildDataBase {
		virtual ~FragmentShaderBuildDataBase() {}
	};

	UniqueDescriptorPool m_descriptorPool;
	UniqueDescriptorSetLayout m_descriptorSetLayout;
	Movable<VkDescriptorSet> m_descriptorSet;

	PbrShadingModel() = default;
	PbrShadingModel(RenderWindow& renderWindow);
	PbrShadingModel(const PbrShadingModel&) = delete;
	PbrShadingModel(PbrShadingModel&&) = default;
	~PbrShadingModel() = default;

	PbrShadingModel& operator=(const PbrShadingModel&) = delete;
	PbrShadingModel& operator=(PbrShadingModel&&) = default;

	CombinedMaterialShadingFragmentFunction combinedMaterialFragmentFunction(
	    sdw::FragmentWriter& writer,
	    MaterialFragmentFunction& materialFunction,
	    FragmentShaderBuildDataBase* shaderBuildData);

	std::unique_ptr<FragmentShaderBuildDataBase>
		instantiateFragmentShaderBuildData();
};
}  // namespace cdm
