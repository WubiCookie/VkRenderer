#pragma once

#include "Buffer.hpp"
#include "MyShaderWriter.hpp"
#include "Texture2D.hpp"
#include "cdm_maths.hpp"

#include <array>
#include <memory>
#include <vector>

namespace cdm
{
class CommandBuffer;
class SceneObject;

class Scene final
{
public:
	static constexpr uint32_t MaxSceneObjectCountPerPool = 256;

private:
	std::reference_wrapper<RenderWindow> rw;

	std::vector<std::unique_ptr<SceneObject>> m_sceneObjects;

	struct alignas(16) SceneUboStruct
	{
		matrix4 view;

		matrix4 proj;

		vector3 viewPos;
		float _0 = 0xcccc;

		vector3 lightPos;
		float _1 = 0xcccc;

		matrix4 shadowView;

		matrix4 shadowProj;

		float shadowBias;
		float R;
		float sigma;
		float roughness;

		matrix3 LTDM;
	};

	struct alignas(16) ModelUboStruct
	{
		std::array<matrix4, MaxSceneObjectCountPerPool> model;
	};

	SceneUboStruct m_sceneUbo;
	std::vector<ModelUboStruct> m_modelUbo;

	Buffer m_sceneUniformBuffer;
	Buffer m_modelUniformBuffer;

	UniqueDescriptorPool m_descriptorPool;
	UniqueDescriptorSetLayout m_descriptorSetLayout;
	Movable<VkDescriptorSet> m_descriptorSet;

	UniqueRenderPass m_shadowmapRenderPass;
	UniqueFramebuffer m_shadowmapFramebuffer;
	VkExtent2D m_shadowmapResolution{ 4096, 4096 };
	Texture2D m_shadowmap;

	/// TODO: `RenderQueue`s

public:
	Scene(RenderWindow& renderWindow);
	Scene(const Scene&) = delete;
	Scene(Scene&&) = default;
	~Scene() = default;

	Scene& operator=(const Scene&) = delete;
	Scene& operator=(Scene&&) = default;

	float shadowBias = -0.4096f;
	float R = 2.0f;
	float sigma = 0.0f;
	float roughness = 0.1f;
	matrix3 LTDM = matrix3::identity();

	const VkDescriptorSetLayout& descriptorSetLayout() const
	{
		return m_descriptorSetLayout;
	}
	const VkDescriptorSet& descriptorSet() const { return m_descriptorSet; }

	SceneObject& instantiateSceneObject();

	void removeSceneObject(SceneObject& sceneObject);

	void drawShadowmapPass(CommandBuffer& cb);

	void draw(CommandBuffer& cb, VkRenderPass renderPass,
	          std::optional<VkViewport> viewport = std::nullopt,
	          std::optional<VkRect2D> scissor = std::nullopt);

	class SceneUbo : private sdw::Ubo
	{
	public:
		SceneUbo(sdw::ShaderWriter& writer);

		sdw::Mat4 getView();
		sdw::Mat4 getShadowView();
		sdw::Mat4 getProj();
		sdw::Mat4 getShadowProj();
		sdw::Float getShadowBias();
		sdw::Vec3 getViewPos();
		sdw::Vec3 getLightPos();
		sdw::Float getR();
		sdw::Float getSigma();
		sdw::Float getRoughness();
		sdw::Mat3 getLTDM();
	};

	class ModelUbo : private sdw::Ubo
	{
	public:
		ModelUbo(sdw::ShaderWriter& writer);

		sdw::Array<sdw::Mat4> getModel();
	};

	class ModelPcb : private sdw::Pcb
	{
	public:
		ModelPcb(sdw::ShaderWriter& writer);

		sdw::UInt getModelId();
		sdw::UInt getMaterialInstanceId();
	};

	// static sdw::Ubo buildSceneUbo(sdw::ShaderWriter& writer, uint32_t
	// binding,
	//        uint32_t set);
	// static sdw::Ubo buildModelUbo(sdw::ShaderWriter& writer, uint32_t
	// binding,
	//        uint32_t set);

	void uploadTransformMatrices(const transform3d& cameraTr,
	                             const matrix4& proj,
	                             const transform3d& lightTr);

	Buffer& sceneUniformBuffer() noexcept { return m_sceneUniformBuffer; }
	Buffer& modelUniformBuffer() noexcept { return m_modelUniformBuffer; }

	Texture2D& shadowmap() { return m_shadowmap; }
};
}  // namespace cdm
