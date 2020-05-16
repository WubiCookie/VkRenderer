#pragma once

#include "VulkanDevice.hpp"

#include "Buffer.hpp"
#include "RenderWindow.hpp"
#include "Texture2D.hpp"

#include <string_view>

namespace cdm
{
class BrdfLutGenerator final
{
	std::reference_wrapper<RenderWindow> rw;

	UniqueRenderPass m_renderPass;

	UniqueShaderModule m_vertexModule;
	UniqueShaderModule m_fragmentModule;

	UniqueDescriptorPool m_descriptorPool;
	UniqueDescriptorSetLayout m_descriptorSetLayout;
	Movable<VkDescriptorSet> m_descriptorSet;
	UniquePipelineLayout m_pipelineLayout;
	UniquePipeline m_pipeline;

	Buffer m_vertexBuffer;

	uint32_t m_resolution;

public:
	BrdfLutGenerator(RenderWindow& renderWindow, uint32_t resolution);
	~BrdfLutGenerator();

	Texture2D computeBrdfLut();
};
}  // namespace cdm
