#pragma once

#include "VulkanDevice.hpp"

#include "BrdfLut.hpp"
#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include "Cubemap.hpp"
#include "DepthTexture.hpp"
#include "IrradianceMap.hpp"
#include "PrefilteredCubemap.hpp"
#include "RenderWindow.hpp"
#include "Skybox.hpp"
#include "Texture2D.hpp"

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
	vector3 tangent;
};

struct Mesh
{
	struct MaterialData
	{
		std::array<uint32_t, 5> textureIndices;
		float uShift = 0.0f;
		float uScale = 1.0f;
		float vShift = 0.0f;
		float vScale = 1.0f;
	};

	MaterialData materialData;

	Movable<RenderWindow*> rw;

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
	Movable<VkDescriptorSet> m_descriptorSet;
	UniquePipelineLayout m_pipelineLayout;
	UniquePipeline m_pipeline;

	std::vector<Mesh> m_meshes;
	// std::vector<Vertex> vertices;
	// std::vector<uint32_t> indices;

	// Buffer m_vertexBuffer;
	// Buffer m_indexBuffer;
	Buffer m_matricesUBO;
	Buffer m_materialUBO;

	Texture2D m_equirectangularTexture;

	Cubemap m_environmentMap;

	IrradianceMap m_irradianceMap;
	PrefilteredCubemap m_prefilteredMap;
	BrdfLut m_brdfLut;

	Texture2D m_defaultTexture;

	std::array<Texture2D, 16> m_albedos;
	std::array<Texture2D, 16> m_displacements;
	std::array<Texture2D, 16> m_metalnesses;
	std::array<Texture2D, 16> m_normals;
	std::array<Texture2D, 16> m_roughnesses;

	Texture2D m_colorAttachmentTexture;
	DepthTexture m_depthTexture;

	std::unique_ptr<Skybox> m_skybox;

public:
	struct Config
	{
		matrix4 model = matrix4::identity();
		matrix4 view = matrix4::identity();
		matrix4 proj = matrix4::identity();

		vector3 viewPos;
		float _0 = 0.0f;

		vector3 lightPos{ 0, 15, 0 };
		float _1 = 0.0f;

		void copyTo(void* ptr);
	};

	transform3d cameraTr;
	transform3d modelTr;

private:
	Config m_config;

	CommandBuffer imguiCB;
	CommandBuffer copyHDRCB;

public:
	ShaderBall(RenderWindow& renderWindow);
	ShaderBall(const ShaderBall&) = delete;
	ShaderBall(ShaderBall&&) = default;
	~ShaderBall();

	ShaderBall& operator=(const ShaderBall&) = delete;
	ShaderBall& operator=(ShaderBall&&) = default;

	void renderOpaque(CommandBuffer& cb);

	void standaloneDraw();
};
}  // namespace cdm
