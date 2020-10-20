#pragma once

#include "Buffer.hpp"
#include "MyShaderWriter.hpp"

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
		vector3 lightPos;
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

	/// TODO: `RenderQueue`s

public:
	Scene(RenderWindow& renderWindow);
	Scene(const Scene&) = delete;
	Scene(Scene&&) = default;
	~Scene() = default;

	Scene& operator=(const Scene&) = delete;
	Scene& operator=(Scene&&) = default;

	const VkDescriptorSetLayout& descriptorSetLayout() const
	{
		return m_descriptorSetLayout;
	}
	const VkDescriptorSet& descriptorSet() const { return m_descriptorSet; }

	SceneObject& instantiateSceneObject();

	void removeSceneObject(SceneObject& sceneObject);

	void draw(CommandBuffer& cb, VkRenderPass renderPass,
	          std::optional<VkViewport> viewport = std::nullopt,
	          std::optional<VkRect2D> scissor = std::nullopt);

	class SceneUbo : private sdw::Ubo
	{
	public:
		SceneUbo(sdw::ShaderWriter& writer);

		sdw::Mat4 getView();
		sdw::Mat4 getProj();
		sdw::Vec3 getViewPos();
		sdw::Vec3 getLightPos();
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

	Buffer& sceneUniformBuffer() noexcept { return m_sceneUniformBuffer; }
	Buffer& modelUniformBuffer() noexcept { return m_modelUniformBuffer; }
};
}  // namespace cdm
