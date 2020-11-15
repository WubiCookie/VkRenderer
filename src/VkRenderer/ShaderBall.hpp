#pragma once

#include "VulkanDevice.hpp"

#include "BrdfLut.hpp"
#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include "Cubemap.hpp"
#include "DepthTexture.hpp"
#include "IrradianceMap.hpp"
#include "Materials/DefaultMaterial.hpp"
#include "Model.hpp"
#include "PbrShadingModel.hpp"
#include "PrefilteredCubemap.hpp"
#include "RenderWindow.hpp"
#include "Skybox.hpp"
#include "Scene.hpp"
#include "SceneObject.hpp"
#include "StandardMesh.hpp"
#include "Texture2D.hpp"

#include "cdm_maths.hpp"

#include <memory>
#include <random>
#include <functional>

namespace cdm
{
struct ShaderBallVertex
{
	vector3 position;
	vector3 normal;
	vector2 uv;
	vector3 tangent;
};

struct ShaderBallMesh
{
	struct MaterialData
	{
		uint32_t objectID = -1;
		std::array<uint32_t, 5> textureIndices;
		float uShift = 0.0f;
		float uScale = 1.0f;
		float vShift = 0.0f;
		float vScale = 1.0f;
		float metalnessShift = 0.0f;
		float metalnessScale = 1.0f;
		float roughnessShift = 0.0f;
		float roughnessScale = 1.0f;
	};

	MaterialData materialData;

	Movable<RenderWindow*> rw;

	std::vector<ShaderBallVertex> vertices;
	std::vector<uint32_t> indices;

	Buffer vertexBuffer;
	Buffer indexBuffer;

	void init();
	void draw(CommandBuffer& cb);
};

class ShaderBall final
{
	struct alignas(16) UBOStruct
	{
		vector4 samples[64];
		matrix4 proj = matrix4::identity();
	};

	Buffer m_uniformBuffer;
	UBOStruct m_uboStruct;

	std::reference_wrapper<RenderWindow> rw;

	UniqueRenderPass m_renderPass;
	UniqueRenderPass m_highlightRenderPass;

	UniqueFramebuffer m_framebuffer;
	UniqueFramebuffer m_highlightFramebuffer;

	UniqueDescriptorPool m_descriptorPool;

	UniqueShaderModule m_vertexModule;
	UniqueShaderModule m_fragmentModule;

	UniqueDescriptorSetLayout m_descriptorSetLayout;
	Movable<VkDescriptorSet> m_descriptorSet;
	UniquePipelineLayout m_pipelineLayout;
	UniquePipeline m_pipeline;

	UniqueShaderModule m_highlightVertexModule;
	UniqueShaderModule m_highlightFragmentModule;

	UniqueDescriptorSetLayout m_highlightDescriptorSetLayout;
	Movable<VkDescriptorSet> m_highlightDescriptorSet;
	UniquePipelineLayout m_highlightPipelineLayout;
	UniquePipeline m_highlightPipeline;

	PbrShadingModel m_shadingModel;
	DefaultMaterial m_defaultMaterial;
	MaterialInstance* m_materialInstance1;
	MaterialInstance* m_materialInstance2;
	MaterialInstance* m_materialInstance3;
	StandardMesh m_bunnyMesh;
	std::vector<StandardMesh> m_sponzaMeshes;
	//Model m_bunnyModel;
	//Pipeline m_bunnyPipeline;

	Scene m_scene;
	SceneObject* m_bunnySceneObject;
	SceneObject* m_bunnySceneObject2;
	SceneObject* m_bunnySceneObject3;
	std::vector<SceneObject*> m_sponzaSceneObjects;

	// Buffer m_vertexBuffer;
	// Buffer m_indexBuffer;
	//Buffer m_matricesUBO;
	//Buffer m_materialUBO;

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
	Texture2D m_singleTexture;
	Texture2D m_singleTexture2;

	Texture2D m_colorAttachmentTexture;
	Texture2D m_objectIDAttachmentTexture;
	DepthTexture m_depthTexture;
	Texture2D m_colorResolveTexture;
	Texture2D m_objectIDResolveTexture;

	Texture2D m_highlightColorAttachmentTexture;
	Texture2D m_normalDepthTexture;
	Texture2D m_normalDepthResolveTexture;
	Texture2D m_positionTexture;
	Texture2D m_positionResolveTexture;
	Texture2D m_noiseTexture;

	std::unique_ptr<Skybox> m_skybox;

	uint32_t m_highlightID = 4294967294;
	uint32_t m_lastSelectedHighlightID = 4294967294;

	bool m_showMaterialWindow = false;

	double m_creationTime = 0.0;

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
	void imgui(CommandBuffer& cb);

	void standaloneDraw();

private:
	bool mustRebuild() const;
	void rebuild();
};
}  // namespace cdm
