#pragma once

#include "VulkanDevice.hpp"

#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include "Cubemap.hpp"
#include "DepthTexture.hpp"
#include "RenderWindow.hpp"
#include "Texture2D.hpp"

#include "cdm_maths.hpp"

#include <memory>
#include <random>

namespace cdm
{
class Skybox final
{
	Movable<RenderWindow*> rw;

	VkRenderPass m_renderPass;
	VkViewport m_viewport;

	UniqueShaderModule m_vertexModule;
	UniqueShaderModule m_fragmentModule;

	UniqueDescriptorPool m_descriptorPool;
	UniqueDescriptorSetLayout m_descriptorSetLayout;
	Movable<VkDescriptorSet> m_descriptorSet;
	UniquePipelineLayout m_pipelineLayout;
	UniquePipeline m_pipeline;

	Buffer m_vertexBuffer;

	Buffer m_ubo;

	Movable<Cubemap*> m_cubemap;

public:
	struct Config
	{
		matrix4 view = matrix4::identity();
		matrix4 proj = matrix4::identity();

		void copyTo(void* ptr);
	};

private:
	Config m_config;

	// CommandBuffer computeCB;// (vk, rw.oneTimeCommandPool());
	// CommandBuffer imguiCB;// (vk, rw.oneTimeCommandPool());
	// CommandBuffer copyHDRCB;// (vk, rw.oneTimeCommandPool());
	// CommandBuffer cb;// (vk, rw.oneTimeCommandPool());

public:
	Skybox() = default;
	Skybox(RenderWindow& renderWindow, VkRenderPass renderPass,
	       VkViewport viewport, Cubemap& m_cubemap);
	Skybox(const Skybox&) = default;
	Skybox(Skybox&&) = default;
	~Skybox();

	Skybox& operator=(const Skybox&) = default;
	Skybox& operator=(Skybox&&) = default;

	void setMatrices(matrix4 projection, matrix4 view);

	void render(CommandBuffer& cb);
};
}  // namespace cdm
