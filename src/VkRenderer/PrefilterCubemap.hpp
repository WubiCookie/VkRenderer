#pragma once

#include "VulkanDevice.hpp"

#include "Buffer.hpp"
#include "Cubemap.hpp"
#include "RenderWindow.hpp"

#include <string_view>

namespace cdm
{
class PrefilterCubemap final
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

	uint32_t m_cubemapWidth;
	uint32_t m_mipLevels;

public:
	PrefilterCubemap(RenderWindow& renderWindow,
	                                uint32_t cubemapWidth, uint32_t mipLevels);
	~PrefilterCubemap();

	Cubemap computeCubemap(Cubemap& cubemap);
};
}  // namespace cdm
