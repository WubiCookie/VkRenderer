#pragma once

#include "VulkanDevice.hpp"

#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include "RenderWindow.hpp"
#include "Texture2D.hpp"
#include "DepthTexture.hpp"

#include "cdm_maths.hpp"

#include <memory>
#include <random>

namespace cdm
{
struct Vertex
{
	vector3 position;
	vector3 normal;
	vector2 uv;
};

struct Mesh
{
	Moveable<RenderWindow*> rw;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	Buffer vertexBuffer;
	Buffer indexBuffer;

	void init();
	void draw(CommandBuffer& cb);
};

class ShaderBall final
{
	std::reference_wrapper<RenderWindow> rw;

	UniqueRenderPass m_renderPass;

	UniqueFramebuffer m_framebuffer;

	UniqueShaderModule m_vertexModule;
	UniqueShaderModule m_fragmentModule;

	UniqueDescriptorPool m_descriptorPool;
	UniqueDescriptorSetLayout m_descriptorSetLayout;
	Moveable<VkDescriptorSet> m_descriptorSet;
	UniquePipelineLayout m_pipelineLayout;
	UniquePipeline m_pipeline;

	std::vector<Mesh> m_meshes;
	//std::vector<Vertex> vertices;
	//std::vector<uint32_t> indices;

	//Buffer m_vertexBuffer;
	//Buffer m_indexBuffer;
	Buffer m_ubo;

	Texture2D m_colorAttachmentTexture;
	DepthTexture m_depthTexture;

	std::random_device rd;
	std::mt19937 gen;
	std::normal_distribution<float> dis;
	std::uniform_real_distribution<float> udis;

public:
	struct Config
	{
		matrix4 model = matrix4::identity();
		matrix4 view = matrix4::identity();
		matrix4 proj = matrix4::identity();

		void copyTo(void* ptr);
	};

	transform3d cameraTr;
	transform3d modelTr;

private:
	Config m_config;

	CommandBuffer computeCB;// (vk, rw.oneTimeCommandPool());
	CommandBuffer imguiCB;// (vk, rw.oneTimeCommandPool());
	CommandBuffer copyHDRCB;// (vk, rw.oneTimeCommandPool());
	CommandBuffer cb;// (vk, rw.oneTimeCommandPool());

public:
	ShaderBall(RenderWindow& renderWindow);
	~ShaderBall();

	void renderOpaque(CommandBuffer& cb);
	//void renderTransparent(CommandBuffer& cb);
	//void renderOverlay(CommandBuffer& cb);

	void standaloneDraw();
};
}  // namespace cdm
