#pragma once

#include "VulkanDevice.hpp"
#include "VulkanHelperStructs.hpp"

#include "cdm_maths.hpp"

#include <optional>
#include <unordered_map>

namespace cdm
{
class CommandBuffer;
class Scene;
class StandardMesh;
class MaterialInterface;

class SceneObject
{
	struct Pipeline
	{
		Movable<Scene*> scene;
		Movable<StandardMesh*> mesh;
		Movable<MaterialInterface*> material;
		Movable<VkRenderPass> renderPass;

		// UniqueDescriptorPool descriptorPool;

		// UniqueDescriptorSetLayout descriptorSetLayout;
		// Movable<VkDescriptorSet> descriptorSet;

		UniqueShaderModule vertexModule;
		UniqueShaderModule fragmentModule;

		UniquePipelineLayout pipelineLayout;
		UniquePipeline pipeline;

		Pipeline() = default;
		Pipeline(Scene& s, StandardMesh& mesh, MaterialInterface& material,
		         VkRenderPass renderPass);
		Pipeline(const Pipeline&) = delete;
		Pipeline(Pipeline&&) = default;
		~Pipeline() = default;

		Pipeline& operator=(const Pipeline&) = delete;
		Pipeline& operator=(Pipeline&&) = default;

		void bindPipeline(CommandBuffer& cb);
		void bindDescriptorSet(CommandBuffer& cb);
		void draw(CommandBuffer& cb);
	};

	struct PcbStruct
	{
		uint32_t modelIndex;
		uint32_t materialInstanceIndex;
	};

protected:
	Movable<Scene*> m_scene;
	Movable<StandardMesh*> m_mesh;
	Movable<MaterialInterface*> m_material;
	std::unordered_map<VkRenderPass, Pipeline> m_pipelines;

public:
	transform3d transform;
	uint32_t id = -1;

	SceneObject() = default;
	SceneObject(Scene& s);
	SceneObject(const SceneObject&) = delete;
	SceneObject(SceneObject&&) = default;
	~SceneObject() = default;

	SceneObject& operator=(const SceneObject&) = delete;
	SceneObject& operator=(SceneObject&&) = default;

	void setMesh(StandardMesh& m) noexcept { m_mesh = &m; }
	StandardMesh* mesh() const noexcept { return m_mesh; }
	void setMaterial(MaterialInterface& m) noexcept { m_material = &m; }
	MaterialInterface* material() const noexcept { return m_material; }

	virtual void draw(CommandBuffer& cb, VkRenderPass renderPass,
	                  std::optional<VkViewport> viewport = std::nullopt,
	                  std::optional<VkRect2D> scissor = std::nullopt);
};
}  // namespace cdm
