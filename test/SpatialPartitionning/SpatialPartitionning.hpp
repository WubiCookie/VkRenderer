#pragma once

#include "RenderApplication.hpp"
#include "VulkanDevice.hpp"
#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include "Texture2D.hpp"
#include "cdm_maths.hpp"

#include <vector>
#include <random>

namespace cdm
{
class SpatialPartitionning final : public RenderApplication
{
	struct Vertex
	{
		vector4 position;
		vector3 normal;
	};
	
	LogRRID log;

	CommandBufferPool m_cbPool;

	UniqueRenderPass m_renderPass;

	Texture2D m_outputImage;

	UniqueFramebuffer m_framebuffer;

	UniqueShaderModule m_vertexModule;
	UniqueShaderModule m_fragmentModule;

	UniqueDescriptorPool m_dPool;
	UniqueDescriptorSetLayout m_dSetLayout;
	Movable<VkDescriptorSet> m_dSet;
	UniquePipelineLayout m_pipelineLayout;

	UniquePipeline m_meshPipeline;
	UniquePipeline m_debogBoxPipeline;

	Buffer m_uniformBuffer;
	Buffer m_vertexBuffer;
	//Buffer m_indexBuffer;

	std::random_device rd;
	std::mt19937 gen;
	std::normal_distribution<float> dis;
	std::uniform_real_distribution<float> udis;

	std::vector<vector3> m_positions;
	std::vector<Vertex> m_vertices;
	//std::vector<uint32_t> m_indices;
	//uint32_t m_indicesCount = 0;

	bool m_swapchainRecreated = false;

public:
	SpatialPartitionning(RenderWindow& renderWindow);
	~SpatialPartitionning();

	void render(CommandBuffer& cb);
	void imgui(CommandBuffer& cb);

	void update() override;
	void draw() override;
};
}  // namespace cdm
