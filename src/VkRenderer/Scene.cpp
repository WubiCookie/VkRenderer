#include "Scene.hpp"

#include "TextureFactory.hpp"
#include "RenderWindow.hpp"
#include "SceneObject.hpp"
//#include "CommandBuffer.hpp"

#include <iostream>

namespace cdm
{
Scene::Scene(RenderWindow& renderWindow) : rw(renderWindow)
{
	auto& vk = renderWindow.device();

	m_sceneUniformBuffer =
	    Buffer(vk, sizeof(SceneUboStruct),
	           VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
	           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_sceneUniformBuffer.setName("Scene UBO");

	m_modelUniformBuffer =
	    Buffer(vk, sizeof(ModelUboStruct), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	           VMA_MEMORY_USAGE_CPU_ONLY,
	           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_modelUniformBuffer.setName("Models UBO");

	TextureFactory f(vk);

	f.setFormat(VK_FORMAT_D32_SFLOAT);
	f.setAspectMask(VK_IMAGE_ASPECT_DEPTH_BIT);
	f.setUsage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
	           VK_IMAGE_USAGE_SAMPLED_BIT);

	m_shadowmap = f.createTexture2D();

#pragma region descriptor pool
	std::array poolSizes{
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
	};

	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.maxSets = 1;
	poolInfo.poolSizeCount = uint32_t(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();

	m_descriptorPool = vk.create(poolInfo);
	if (!m_descriptorPool)
	{
		std::cerr << "error: failed to create descriptor pool" << std::endl;
		abort();
	}
#pragma endregion

#pragma region descriptor set layout
	VkDescriptorSetLayoutBinding layoutBindingSceneUbo{};
	layoutBindingSceneUbo.binding = 0;
	layoutBindingSceneUbo.descriptorCount = 1;
	layoutBindingSceneUbo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBindingSceneUbo.stageFlags =
	    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding layoutBindingModelUbo{};
	layoutBindingModelUbo.binding = 1;
	layoutBindingModelUbo.descriptorCount = 1;
	layoutBindingModelUbo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBindingModelUbo.stageFlags =
	    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding layoutBindingShadowMap{};
	layoutBindingShadowMap.binding = 2;
	layoutBindingShadowMap.descriptorCount = 1;
	layoutBindingShadowMap.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindingShadowMap.stageFlags =
	    VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array layoutBindings{
		layoutBindingSceneUbo,
		layoutBindingModelUbo,
		                       layoutBindingShadowMap,
	};

	vk::DescriptorSetLayoutCreateInfo setLayoutInfo;
	setLayoutInfo.bindingCount = uint32_t(layoutBindings.size());
	setLayoutInfo.pBindings = layoutBindings.data();

	m_descriptorSetLayout = vk.create(setLayoutInfo);
	if (!m_descriptorSetLayout)
	{
		std::cerr << "error: failed to create descriptor set layout"
		          << std::endl;
		abort();
	}
#pragma endregion

#pragma region descriptor set
	m_descriptorSet = vk.allocate(m_descriptorPool, m_descriptorSetLayout);

	if (!m_descriptorSet)
	{
		std::cerr << "error: failed to allocate descriptor set" << std::endl;
		abort();
	}
#pragma endregion

#pragma region update descriptor set
	VkDescriptorBufferInfo sceneSetBufferInfo{};
	sceneSetBufferInfo.buffer = m_sceneUniformBuffer;
	sceneSetBufferInfo.range = sizeof(SceneUboStruct);
	sceneSetBufferInfo.offset = 0;

	VkDescriptorBufferInfo modelSetBufferInfo{};
	modelSetBufferInfo.buffer = m_modelUniformBuffer;
	modelSetBufferInfo.range = sizeof(ModelUboStruct);
	modelSetBufferInfo.offset = 0;

	vk::WriteDescriptorSet sceneUboWrite;
	sceneUboWrite.descriptorCount = 1;
	sceneUboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	sceneUboWrite.dstArrayElement = 0;
	sceneUboWrite.dstBinding = 0;
	sceneUboWrite.dstSet = m_descriptorSet;
	sceneUboWrite.pBufferInfo = &sceneSetBufferInfo;

	vk::WriteDescriptorSet modelUboWrite;
	modelUboWrite.descriptorCount = 1;
	modelUboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	modelUboWrite.dstArrayElement = 0;
	modelUboWrite.dstBinding = 1;
	modelUboWrite.dstSet = m_descriptorSet;
	modelUboWrite.pBufferInfo = &modelSetBufferInfo;

	vk.updateDescriptorSets({ sceneUboWrite, modelUboWrite });
#pragma endregion
}

SceneObject& Scene::instantiateSceneObject()
{
	if (m_sceneObjects.size() >= MaxSceneObjectCountPerPool)
		throw std::runtime_error("can not instanciate more SceneObjects");

	m_sceneObjects.push_back(std::make_unique<SceneObject>(*this));
	m_sceneObjects.back()->id = m_sceneObjects.size() - 1;
	return *m_sceneObjects.back();
}

void Scene::removeSceneObject(SceneObject& sceneObject)
{
	std::remove_if(m_sceneObjects.begin(), m_sceneObjects.end(),
	               [&](const std::unique_ptr<SceneObject>& soptr) {
		               return &sceneObject == soptr.get();
	               });
}

void Scene::draw(CommandBuffer& cb, VkRenderPass renderPass,
                 std::optional<VkViewport> viewport,
                 std::optional<VkRect2D> scissor)
{
	for (auto& sceneObject : m_sceneObjects)
	{
		sceneObject->draw(cb, renderPass, viewport, scissor);
	}
}

void Scene::uploadTransformMatrices(const transform3d& cameraTr, const matrix4& proj)
{
	struct alignas(16) SceneUboStruct
	{
		matrix4 view;
		matrix4 proj;
		vector3 viewPos;
		vector3 lightPos;
	};

	struct alignas(16) ModelUboStruct
	{
		std::array<matrix4, Scene::MaxSceneObjectCountPerPool> model;
	};

	SceneUboStruct* sceneUBOPtr =
	    sceneUniformBuffer().map<SceneUboStruct>();
	sceneUBOPtr->lightPos = { 0, 0, 0 };
	sceneUBOPtr->view = matrix4(cameraTr).get_transposed().get_inversed();
	sceneUBOPtr->proj = proj;
	sceneUBOPtr->viewPos = cameraTr.position;
	sceneUniformBuffer().unmap();

	ModelUboStruct* modelUBOPtr =
	    modelUniformBuffer().map<ModelUboStruct>();

	for (size_t i = 0; i < m_sceneObjects.size(); i++)
	{
		modelUBOPtr->model[i] =
		    matrix4(m_sceneObjects[i]->transform).get_transposed();
	}

	modelUniformBuffer().unmap();
}

    // sdw::Ubo Scene::buildSceneUbo(sdw::ShaderWriter& writer, uint32_t binding,
//                              uint32_t set)
//{
//	sdw::Ubo ubo(writer, "SceneUBO", binding, set);
//	ubo.declMember<sdw::Mat4>("view");
//	ubo.declMember<sdw::Mat4>("proj");
//	ubo.declMember<sdw::Vec3>("viewPos");
//	ubo.declMember<sdw::Vec3>("lightPos");
//	ubo.end();
//	return ubo;
//}
//
// sdw::Ubo Scene::buildModelUbo(sdw::ShaderWriter& writer, uint32_t binding,
//                              uint32_t set)
//{
//	sdw::Ubo ubo(writer, "ModelUBO", binding, set);
//	ubo.declMember<sdw::Mat4>("model", uint32_t(MaxSceneObjectCountPerPool));
//	ubo.end();
//	return ubo;
//}

Scene::SceneUbo::SceneUbo(sdw::ShaderWriter& writer)
    : sdw::Ubo(writer, "SceneUBO", 0, 0)
{
	declMember<sdw::Mat4>("view");
	declMember<sdw::Mat4>("proj");
	declMember<sdw::Vec3>("viewPos");
	declMember<sdw::Vec3>("lightPos");
	end();
}

sdw::Mat4 Scene::SceneUbo::getView() { return getMember<sdw::Mat4>("view"); }

sdw::Mat4 Scene::SceneUbo::getProj() { return getMember<sdw::Mat4>("proj"); }

sdw::Vec3 Scene::SceneUbo::getViewPos()
{
	return getMember<sdw::Vec3>("viewPos");
}

sdw::Vec3 Scene::SceneUbo::getLightPos()
{
	return getMember<sdw::Vec3>("lightPos");
}

Scene::ModelUbo::ModelUbo(sdw::ShaderWriter& writer)
    : sdw::Ubo(writer, "ModelUBO", 1, 0)
{
	declMember<sdw::Mat4>("model", uint32_t(MaxSceneObjectCountPerPool));
	end();
}

sdw::Array<sdw::Mat4> Scene::ModelUbo::getModel()
{
	return getMemberArray<sdw::Mat4>("model");
}

Scene::ModelPcb::ModelPcb(sdw::ShaderWriter& writer)
    : sdw::Pcb(writer, "ModelPCB")
{
	declMember<sdw::UInt>("modelId");
	declMember<sdw::UInt>("materialInstanceId");
	end();
}

sdw::UInt Scene::ModelPcb::getModelId()
{
	return getMember<sdw::UInt>("modelId");
}

sdw::UInt Scene::ModelPcb::getMaterialInstanceId()
{
	return getMember<sdw::UInt>("materialInstanceId");
}
}  // namespace cdm
