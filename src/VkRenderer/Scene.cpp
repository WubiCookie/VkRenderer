#include "Scene.hpp"

#include "CommandBuffer.hpp"
#include "RenderWindow.hpp"
#include "SceneObject.hpp"
#include "TextureFactory.hpp"

#include <iostream>

namespace cdm
{
Scene::Scene(RenderWindow& renderWindow) : rw(renderWindow)
{
	auto& vk = renderWindow.device();

	m_sceneUniformBuffer =
	    Buffer(vk, sizeof(SceneUboStruct), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	           VMA_MEMORY_USAGE_CPU_ONLY,
	           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_sceneUniformBuffer.setName("Scene UBO");

	m_modelUniformBuffer =
	    Buffer(vk, sizeof(ModelUboStruct), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	           VMA_MEMORY_USAGE_CPU_ONLY,
	           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_modelUniformBuffer.setName("Models UBO");

#pragma region shadowmap
	TextureFactory f(vk);

	f.setExtent(m_shadowmapResolution);
	f.setFormat(VK_FORMAT_D32_SFLOAT);
	f.setAspectMask(VK_IMAGE_ASPECT_DEPTH_BIT);
	f.setUsage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
	           VK_IMAGE_USAGE_SAMPLED_BIT);
	f.setSamplerCompareEnable(true);
	f.setSamplerCompareOp(VK_COMPARE_OP_LESS);

	m_shadowmap = f.createTexture2D();
	m_shadowmap.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	vk.debugMarkerSetObjectName(m_shadowmap.image(), "Scene shadowmap image");
	vk.debugMarkerSetObjectName(m_shadowmap.view(),
	                            "Scene shadowmap imageView");
	vk.debugMarkerSetObjectName(m_shadowmap.sampler(),
	                            "Scene shadowmap sampler");

	VkDescriptorImageInfo shadowmapImageInfo{};
	shadowmapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	shadowmapImageInfo.imageView = m_shadowmap.view();
	shadowmapImageInfo.sampler = m_shadowmap.sampler();
#pragma endregion

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
	layoutBindingShadowMap.descriptorType =
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindingShadowMap.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

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

	vk::WriteDescriptorSet shadowmapWrite;
	shadowmapWrite.descriptorCount = 1;
	shadowmapWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	shadowmapWrite.dstArrayElement = 0;
	shadowmapWrite.dstBinding = 2;
	shadowmapWrite.dstSet = m_descriptorSet;
	shadowmapWrite.pImageInfo = &shadowmapImageInfo;

	vk.updateDescriptorSets({ sceneUboWrite, modelUboWrite, shadowmapWrite });
#pragma endregion

#pragma region render pass
	vk::AttachmentDescription attachment;
	attachment.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	attachment.format = VK_FORMAT_D32_SFLOAT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;

	VkAttachmentReference depthAttachment{};
	depthAttachment.attachment = 0;
	depthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	vk::SubpassDescription subpass;
	subpass.pDepthStencilAttachment = &depthAttachment;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	vk::RenderPassCreateInfo rpInfo;
	rpInfo.attachmentCount = 1;
	rpInfo.pAttachments = &attachment;
	rpInfo.subpassCount = 1;
	rpInfo.pSubpasses = &subpass;

	m_shadowmapRenderPass = vk.create(rpInfo);
#pragma endregion

#pragma region framebuffer
	vk::FramebufferCreateInfo fbInfo;
	fbInfo.attachmentCount = 1;
	fbInfo.pAttachments = &m_shadowmap.view();
	fbInfo.width = m_shadowmap.width();
	fbInfo.height = m_shadowmap.height();
	fbInfo.renderPass = m_shadowmapRenderPass;
	fbInfo.layers = 1;

	m_shadowmapFramebuffer = vk.create(fbInfo);
#pragma endregion
}

SceneObject& Scene::instantiateSceneObject()
{
	if (m_sceneObjects.size() >= MaxSceneObjectCountPerPool)
		throw std::runtime_error("can not instanciate more SceneObjects");

	m_sceneObjects.push_back(std::make_unique<SceneObject>(*this));
	m_sceneObjects.back()->id = uint32_t(m_sceneObjects.size() - 1);
	return *m_sceneObjects.back();
}

void Scene::removeSceneObject(SceneObject& sceneObject)
{
	std::remove_if(m_sceneObjects.begin(), m_sceneObjects.end(),
	               [&](const std::unique_ptr<SceneObject>& soptr) {
		               return &sceneObject == soptr.get();
	               });
}

void Scene::drawShadowmapPass(CommandBuffer& cb)
{
	VkClearValue clearDepth{};
	clearDepth.depthStencil.depth = 1.0f;

	std::array clearValues = { clearDepth };

	vk::RenderPassBeginInfo rpInfo;
	rpInfo.framebuffer = m_shadowmapFramebuffer;
	rpInfo.renderPass = m_shadowmapRenderPass;
	rpInfo.renderArea.extent = m_shadowmap.extent2D();
	rpInfo.clearValueCount = uint32_t(clearValues.size());
	rpInfo.pClearValues = clearValues.data();

	vk::SubpassBeginInfo subpassBeginInfo;
	subpassBeginInfo.contents = VK_SUBPASS_CONTENTS_INLINE;

	vk::SubpassEndInfo subpassEndInfo;

	cb.beginRenderPass2(rpInfo, subpassBeginInfo);

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = float(m_shadowmap.width());
	viewport.height = float(m_shadowmap.height());
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	cb.setViewport(viewport);

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent.width = m_shadowmap.width();
	scissor.extent.height = m_shadowmap.height();
	cb.setScissor(scissor);

	for (auto& sceneObject : m_sceneObjects)
	{
		sceneObject->drawShadowmapPass(cb, m_shadowmapRenderPass, viewport,
		                               scissor);
	}

	cb.endRenderPass2(subpassEndInfo);
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

void Scene::uploadTransformMatrices(const transform3d& cameraTr,
                                    const matrix4& proj,
                                    const transform3d& lightTr)
{
	SceneUboStruct* sceneUBOPtr = sceneUniformBuffer().map<SceneUboStruct>();
	sceneUBOPtr->lightPos = { 0, 0, 0 };
	sceneUBOPtr->view = matrix4(cameraTr).get_transposed().get_inversed();
	sceneUBOPtr->proj = proj;
	sceneUBOPtr->viewPos = cameraTr.position;
	sceneUBOPtr->shadowView = matrix4(lightTr).get_transposed().get_inversed();
	sceneUBOPtr->shadowProj =
	    matrix4::orthographic(-150, 150, 150, -150, 0.01f, 1000.0f)
	        .get_transposed();
	sceneUBOPtr->shadowBias = shadowBias;
	sceneUBOPtr->R = R;
	sceneUBOPtr->sigma = sigma;
	sceneUBOPtr->roughness = roughness;
	sceneUBOPtr->LTDM = LTDM;
	sceneUBOPtr->param0 = param0;
	sceneUBOPtr->param1 = param1;
	sceneUBOPtr->param2 = param2;
	sceneUBOPtr->param3 = param3;

	sceneUniformBuffer().unmap();

	ModelUboStruct* modelUBOPtr = modelUniformBuffer().map<ModelUboStruct>();

	for (size_t i = 0; i < m_sceneObjects.size(); i++)
	{
		modelUBOPtr->model[i] =
		    matrix4(m_sceneObjects[i]->transform).get_transposed();
	}

	modelUniformBuffer().unmap();
}

Scene::SceneUbo::SceneUbo(sdw::ShaderWriter& writer)
    : sdw::Ubo(writer, "SceneUBO", 0, 0)
{
	declMember<sdw::Mat4>("view");
	declMember<sdw::Mat4>("proj");
	declMember<sdw::Vec3>("viewPos");
	declMember<sdw::Vec3>("lightPos");
	declMember<sdw::Mat4>("shadowView");
	declMember<sdw::Mat4>("shadowProj");
	declMember<sdw::Float>("shadowBias");
	declMember<sdw::Float>("R");
	declMember<sdw::Float>("sigma");
	declMember<sdw::Float>("roughness");
	declMember<sdw::Mat3>("LTDM");
	declMember<sdw::Float>("param0");
	declMember<sdw::Float>("param1");
	declMember<sdw::Float>("param2");
	declMember<sdw::Float>("param3");
	end();
}

sdw::Mat4 Scene::SceneUbo::getView() { return getMember<sdw::Mat4>("view"); }
sdw::Mat4 Scene::SceneUbo::getShadowView()
{
	return getMember<sdw::Mat4>("shadowView");
}
sdw::Mat4 Scene::SceneUbo::getProj() { return getMember<sdw::Mat4>("proj"); }
sdw::Mat4 Scene::SceneUbo::getShadowProj()
{
	return getMember<sdw::Mat4>("shadowProj");
}
sdw::Vec3 Scene::SceneUbo::getViewPos()
{
	return getMember<sdw::Vec3>("viewPos");
}
sdw::Vec3 Scene::SceneUbo::getLightPos()
{
	return getMember<sdw::Vec3>("lightPos");
}
sdw::Float Scene::SceneUbo::getShadowBias()
{
	return getMember<sdw::Float>("shadowBias");
}
sdw::Float Scene::SceneUbo::getR() { return getMember<sdw::Float>("R"); }
sdw::Float Scene::SceneUbo::getSigma()
{
	return getMember<sdw::Float>("sigma");
}
sdw::Float Scene::SceneUbo::getRoughness()
{
	return getMember<sdw::Float>("roughness");
}
sdw::Mat3 Scene::SceneUbo::getLTDM() { return getMember<sdw::Mat3>("LTDM"); }
sdw::Float Scene::SceneUbo::getParam0()
{
	return getMember<sdw::Float>("param0");
}
sdw::Float Scene::SceneUbo::getParam1()
{
	return getMember<sdw::Float>("param1");
}
sdw::Float Scene::SceneUbo::getParam2()
{
	return getMember<sdw::Float>("param2");
}
sdw::Float Scene::SceneUbo::getParam3()
{
	return getMember<sdw::Float>("param3");
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
