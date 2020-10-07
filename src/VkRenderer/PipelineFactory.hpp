#pragma once

#include "MyShaderWriter.hpp"
#include "VertexInputHelper.hpp"
#include "VulkanDevice.hpp"

namespace cdm
{
class GraphicsPipelineFactory
{
	std::reference_wrapper<const VulkanDevice> m_vulkanDevice;

	VkShaderModule m_vertexModule = nullptr;
	VkShaderModule m_fragmentModule = nullptr;

	VertexInputHelper m_vertexInputHelper;
	vk::PipelineInputAssemblyStateCreateInfo m_inputAssembly;

	vk::Viewport m_viewport;
	vk::Rect2D m_scissor;
	vk::PipelineViewportStateCreateInfo m_viewportState;

	vk::PipelineRasterizationStateCreateInfo m_rasterizer;

	vk::PipelineMultisampleStateCreateInfo m_multisampling;

	std::vector<vk::PipelineColorBlendAttachmentState> m_colorBlendAttachments;
	vk::PipelineColorBlendStateCreateInfo m_colorBlending;

	vk::PipelineDepthStencilStateCreateInfo m_depthStencil;

	std::vector<VkDynamicState> m_dynamicStates;

	VkRenderPass m_renderPass = nullptr;
	uint32_t m_subpass = 0;
	VkPipelineLayout m_layout = nullptr;

	vk::GraphicsPipelineCreateInfo m_pipelineInfo;

public:
	GraphicsPipelineFactory(const VulkanDevice& vulkanDevice);
	~GraphicsPipelineFactory() = default;

	GraphicsPipelineFactory(const GraphicsPipelineFactory&) = default;
	GraphicsPipelineFactory(GraphicsPipelineFactory&&) = default;

	GraphicsPipelineFactory& operator=(const GraphicsPipelineFactory&) =
	    default;
	GraphicsPipelineFactory& operator=(GraphicsPipelineFactory&&) = default;

	void setShaderModules(VkShaderModule vertexModule,
	                      VkShaderModule fragmentModule);
	void setVertexInputHelper(VertexInputHelper vertexInputHelper);
	void setTopology(VkPrimitiveTopology topology);
	void setPrimitiveRestartEnable(bool enable);
	void setDepthClampEnable(bool enable);
	void setRasterizeDiscardEnable(bool enable);
	void setPolygonMode(VkPolygonMode mode);
	void setLineWidth(float width);
	void setCullMode(VkCullModeFlags mode);
	void setFrontFace(VkFrontFace frontFace);
	void setDepthBiasEnable(bool enable);
	void setDepthBiasConstantFactor(float factor);
	void setDepthBiasClamp(float clamp);
	void setDepthBiasSlopeFactor(float factor);

	void setSampleShadingEnable(bool enable);
	void setRasterizationSamples(VkSampleCountFlagBits sample);
	void setMinSampleShading(float minSampleShading);
	void setAlphaToCoverageEnable(bool enable);
	void setAlphaToOneEnable(bool enable);

	void addColorBlendAttachmentState(
	    const vk::PipelineColorBlendAttachmentState& state);
	void clearColorBlendAttachmentStates();
	void setBlendLogicOpEnable(bool enable);
	void setBlendLogicOp(VkLogicOp op);
	void setBlendConstant(float constant[4]);
	void setBlendConstant(float constantR, float constantG, float constantB,
	                      float constantA);

	void setViewport(const vk::Viewport& viewport);
	void setViewport(float x, float y, float width, float height,
	                 float minDepth = 0.0f, float maxDepth = 1.0f);
	void setScissor(const vk::Rect2D& scissor);
	void setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height);

	void addDynamicState(VkDynamicState state);
	void clearDynamicStates();

	void setRenderPass(VkRenderPass renderPass);
	void setLayout(VkPipelineLayout layout);

	std::pair<UniquePipelineLayout, std::vector<UniqueDescriptorSetLayout>>
	createLayout(const VertexShaderHelperResult& vertexHelperResult,
	             const FragmentShaderHelperResult& fragmentHelperResult);

	UniqueGraphicsPipeline createPipeline();
};

class ComputePipelineFactory
{
	UniqueShaderModule m_computeModule;

public:
	ComputePipelineFactory() = default;
	~ComputePipelineFactory() = default;

	ComputePipelineFactory(const ComputePipelineFactory&) = default;
	ComputePipelineFactory(ComputePipelineFactory&&) = default;

	ComputePipelineFactory& operator=(const ComputePipelineFactory&) = default;
	ComputePipelineFactory& operator=(ComputePipelineFactory&&) = default;

	void setShaderWriter(const ComputeWriter& computeWriter);
};
}  // namespace cdm
