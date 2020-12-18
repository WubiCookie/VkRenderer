#include "PipelineFactory.hpp"

#include "MyShaderWriter.hpp"

#include <iostream>

namespace cdm
{
GraphicsPipelineFactory::GraphicsPipelineFactory(
	const VulkanDevice& vulkanDevice)
	: m_vulkanDevice(vulkanDevice)
{
	m_inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	m_inputAssembly.primitiveRestartEnable = false;

	m_viewportState.viewportCount = 1;
	m_viewportState.pViewports = &m_viewport;
	m_viewportState.scissorCount = 1;
	m_viewportState.pScissors = &m_scissor;

	m_rasterizer.depthClampEnable = false;
	m_rasterizer.rasterizerDiscardEnable = false;
	m_rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	m_rasterizer.lineWidth = 1.0f;
	m_rasterizer.cullMode = VK_CULL_MODE_NONE;
	m_rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	m_rasterizer.depthBiasEnable = false;
	m_rasterizer.depthBiasConstantFactor = 0.0f;
	m_rasterizer.depthBiasClamp = 0.0f;
	m_rasterizer.depthBiasSlopeFactor = 0.0f;

	// rasterizer.depthClampEnable = false;
	// rasterizer.rasterizerDiscardEnable = false;
	// rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	//// rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
	// rasterizer.lineWidth = 1.0f;
	// rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	// rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	// rasterizer.depthBiasEnable = false;
	// rasterizer.depthBiasConstantFactor = 0.0f;
	// rasterizer.depthBiasClamp = 0.0f;
	// rasterizer.depthBiasSlopeFactor = 0.0f;

	m_multisampling.sampleShadingEnable = false;
	m_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	m_multisampling.minSampleShading = 1.0f;
	m_multisampling.pSampleMask = nullptr;
	m_multisampling.alphaToCoverageEnable = false;
	m_multisampling.alphaToOneEnable = false;

	m_colorBlending.logicOpEnable = false;
	m_colorBlending.logicOp = VK_LOGIC_OP_COPY;
	m_colorBlending.blendConstants[0] = 0.0f;
	m_colorBlending.blendConstants[1] = 0.0f;
	m_colorBlending.blendConstants[2] = 0.0f;
	m_colorBlending.blendConstants[3] = 0.0f;

	m_depthStencil.depthTestEnable = false;
	m_depthStencil.depthWriteEnable = false;
	m_depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	m_depthStencil.depthBoundsTestEnable = false;
	m_depthStencil.minDepthBounds = 0.0f;  // Optional
	m_depthStencil.maxDepthBounds = 1.0f;  // Optional
	m_depthStencil.stencilTestEnable = false;
}

void GraphicsPipelineFactory::setShaderModules(VkShaderModule vertexModule,
	VkShaderModule fragmentModule)
{
	m_vertexModule = vertexModule;
	m_fragmentModule = fragmentModule;
}

void GraphicsPipelineFactory::setVertexInputHelper(
	VertexInputHelper vertexInputHelper)
{
	m_vertexInputHelper = std::move(vertexInputHelper);
}

void GraphicsPipelineFactory::setTopology(VkPrimitiveTopology topology)
{
	m_inputAssembly.topology = topology;
}

void GraphicsPipelineFactory::setPrimitiveRestartEnable(bool enable)
{
	m_inputAssembly.primitiveRestartEnable = enable;
}

void GraphicsPipelineFactory::setDepthClampEnable(bool enable)
{
	m_rasterizer.depthClampEnable = enable;
}

void GraphicsPipelineFactory::setRasterizeDiscardEnable(bool enable)
{
	m_rasterizer.rasterizerDiscardEnable = enable;
}

void GraphicsPipelineFactory::setPolygonMode(VkPolygonMode mode)
{
	m_rasterizer.polygonMode = mode;
}

void GraphicsPipelineFactory::setLineWidth(float width)
{
	m_rasterizer.lineWidth = width;
}

void GraphicsPipelineFactory::setCullMode(VkCullModeFlags mode)
{
	m_rasterizer.cullMode = mode;
}

void GraphicsPipelineFactory::setFrontFace(VkFrontFace frontFace)
{
	m_rasterizer.frontFace = frontFace;
}

void GraphicsPipelineFactory::setDepthBiasEnable(bool enable)
{
	m_rasterizer.depthBiasEnable = enable;
}

void GraphicsPipelineFactory::setDepthBiasConstantFactor(float factor)
{
	m_rasterizer.depthBiasConstantFactor = factor;
}

void GraphicsPipelineFactory::setDepthBiasClamp(float clamp)
{
	m_rasterizer.depthBiasClamp = clamp;
}

void GraphicsPipelineFactory::setDepthBiasSlopeFactor(float factor)
{
	m_rasterizer.depthBiasSlopeFactor = factor;
}

void GraphicsPipelineFactory::setSampleShadingEnable(bool enable)
{
	m_multisampling.sampleShadingEnable = enable;
}

void GraphicsPipelineFactory::setRasterizationSamples(
	VkSampleCountFlagBits sample)
{
	m_multisampling.rasterizationSamples = sample;
}

void GraphicsPipelineFactory::setMinSampleShading(float minSampleShading)
{
	m_multisampling.minSampleShading = minSampleShading;
}

void GraphicsPipelineFactory::setAlphaToCoverageEnable(bool enable)
{
	m_multisampling.alphaToCoverageEnable = enable;
}

void GraphicsPipelineFactory::setAlphaToOneEnable(bool enable)
{
	m_multisampling.alphaToOneEnable = enable;
}

void GraphicsPipelineFactory::addColorBlendAttachmentState(
	const vk::PipelineColorBlendAttachmentState& state)
{
	m_colorBlendAttachments.push_back(state);
}

void GraphicsPipelineFactory::clearColorBlendAttachmentStates()
{
	m_colorBlendAttachments.clear();
}

void GraphicsPipelineFactory::setBlendLogicOpEnable(bool enable)
{
	m_colorBlending.logicOpEnable = enable;
}

void GraphicsPipelineFactory::setBlendLogicOp(VkLogicOp op)
{
	m_colorBlending.logicOp = op;
}

void GraphicsPipelineFactory::setBlendConstant(float constant[4])
{
	setBlendConstant(constant[0], constant[1], constant[2], constant[3]);
}

void GraphicsPipelineFactory::setBlendConstant(float constantR,
	float constantG,
	float constantB,
	float constantA)
{
	m_colorBlending.blendConstants[0] = constantR;
	m_colorBlending.blendConstants[1] = constantG;
	m_colorBlending.blendConstants[2] = constantB;
	m_colorBlending.blendConstants[3] = constantA;
}

void GraphicsPipelineFactory::setViewport(const vk::Viewport& viewport)
{
	m_viewport = viewport;
}

void GraphicsPipelineFactory::setViewport(float x, float y, float width,
	float height, float minDepth,
	float maxDepth)
{
	m_viewport.x = x;
	m_viewport.y = y;
	m_viewport.width = width;
	m_viewport.height = height;
	m_viewport.minDepth = minDepth;
	m_viewport.maxDepth = maxDepth;
}

void GraphicsPipelineFactory::setScissor(const vk::Rect2D& scissor)
{
	m_scissor = scissor;
}

void GraphicsPipelineFactory::setScissor(int32_t x, int32_t y, uint32_t width,
	uint32_t height)
{
	m_scissor.offset.x = x;
	m_scissor.offset.y = y;
	m_scissor.extent.width = width;
	m_scissor.extent.height = height;
}

void GraphicsPipelineFactory::addDynamicState(VkDynamicState state)
{
	m_dynamicStates.push_back(state);
}

void GraphicsPipelineFactory::clearDynamicStates() { m_dynamicStates.clear(); }

void GraphicsPipelineFactory::setRenderPass(VkRenderPass renderPass)
{
	m_renderPass = renderPass;
}

void GraphicsPipelineFactory::setLayout(VkPipelineLayout layout)
{
	m_layout = layout;
}

std::pair<UniquePipelineLayout, std::vector<UniqueDescriptorSetLayout>>
GraphicsPipelineFactory::createLayout(
	const VertexShaderHelperResult& vertexHelperResult,
	const FragmentShaderHelperResult& fragmentHelperResult,
	const std::vector<VkPushConstantRange>& pushConstants)
{
	const auto& vk = m_vulkanDevice.get();

	std::pair<UniquePipelineLayout, std::vector<UniqueDescriptorSetLayout>>
		res;

	const auto& vertexDescriptors = vertexHelperResult.descriptors;
	const auto& fragmentDescriptors = fragmentHelperResult.descriptors;

	std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>>
		mapped;

	for (const auto& vertexDescriptor : vertexDescriptors)
		mapped[vertexDescriptor.first].push_back(vertexDescriptor.second);

	for (const auto& fragmentDescriptor : fragmentDescriptors)
	{
		auto& v = mapped[fragmentDescriptor.first];

		bool added = false;
		for (auto& d : v)
		{
			if (fragmentDescriptor.second.binding == d.binding)
			{
				d.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
				added = true;
				break;
			}
		}

		if (!added)
			mapped[fragmentDescriptor.first].push_back(
				fragmentDescriptor.second);
	}

	for (auto& pair : mapped)
	{
		res.second.resize(size_t(pair.first) + 1);

		vk::DescriptorSetLayoutCreateInfo info;
		info.bindingCount = uint32_t(pair.second.size());
		info.pBindings = pair.second.data();

		res.second[pair.first] = vk.createDescriptorSetLayout(info);
	}

	std::vector<VkDescriptorSetLayout> layouts;
	layouts.reserve(res.second.size());
	for (auto& setLayout : res.second)
		layouts.push_back(setLayout.get());

	vk::PipelineLayoutCreateInfo info;
	info.setLayoutCount = uint32_t(layouts.size());
	info.pSetLayouts = layouts.data();
	info.pushConstantRangeCount = uint32_t(pushConstants.size());
	info.pPushConstantRanges =
		pushConstants.empty() ? nullptr : pushConstants.data();

	res.first = vk.createPipelineLayout(info);

	return res;
}

UniqueGraphicsPipeline GraphicsPipelineFactory::createPipeline()
{
	const auto& vk = m_vulkanDevice.get();

	m_viewportState.viewportCount = 1;
	m_viewportState.pViewports = &m_viewport;
	m_viewportState.scissorCount = 1;
	m_viewportState.pScissors = &m_scissor;

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = m_vertexModule;
	vertShaderStageInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = m_fragmentModule;
	fragShaderStageInfo.pName = "main";

	std::array shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

	// std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages;
	// shaderStages[0].module = m_vertexModule;
	// shaderStages[0].pName = "main";
	// shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	// shaderStages[1].module = m_fragmentModule;
	// shaderStages[1].pName = "main";
	// shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;

	auto vertexInputInfo = m_vertexInputHelper.getCreateInfo();

	m_colorBlending.attachmentCount = uint32_t(m_colorBlendAttachments.size());
	m_colorBlending.pAttachments = m_colorBlendAttachments.data();

	vk::PipelineDynamicStateCreateInfo dynamicState;
	dynamicState.dynamicStateCount = uint32_t(m_dynamicStates.size());
	dynamicState.pDynamicStates = m_dynamicStates.data();

	vk::GraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.stageCount = uint32_t(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &m_inputAssembly;
	pipelineInfo.pViewportState = &m_viewportState;
	pipelineInfo.pRasterizationState = &m_rasterizer;
	pipelineInfo.pMultisampleState = &m_multisampling;
	pipelineInfo.pDepthStencilState = &m_depthStencil;
	pipelineInfo.pColorBlendState = &m_colorBlending;
	pipelineInfo.pDynamicState =
		m_dynamicStates.empty() ? nullptr : &dynamicState;
	pipelineInfo.layout = m_layout;
	pipelineInfo.renderPass = m_renderPass;
	pipelineInfo.subpass = m_subpass;
	pipelineInfo.basePipelineHandle = nullptr;
	pipelineInfo.basePipelineIndex = -1;

	return vk.create(pipelineInfo);
}

ComputePipelineFactory::ComputePipelineFactory(
	const VulkanDevice& vulkanDevice) : m_vulkanDevice(vulkanDevice)
{}

void ComputePipelineFactory::setShaderModule(VkShaderModule computeModule)
{
	m_computeModule = computeModule;
}

void ComputePipelineFactory::setLayout(VkPipelineLayout layout)
{
	m_layout = layout;
}

std::pair<UniquePipelineLayout, std::vector<UniqueDescriptorSetLayout>>
ComputePipelineFactory::createLayout(const ComputeShaderHelperResult& computeHelperResult,
	const std::vector<VkPushConstantRange>& pushConstants)
{
	const auto& vk = m_vulkanDevice.get();

	std::pair<UniquePipelineLayout, std::vector<UniqueDescriptorSetLayout>>
		res;

	const auto& descriptors = computeHelperResult.descriptors;

	std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>>
		mapped;

	for (const auto& d : descriptors)
		mapped[d.first].push_back(d.second);

	//for (const auto& fragmentDescriptor : fragmentDescriptors)
	//{
	//	auto& v = mapped[fragmentDescriptor.first];

	//	bool added = false;
	//	for (auto& d : v)
	//	{
	//		if (fragmentDescriptor.second.binding == d.binding)
	//		{
	//			d.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
	//			added = true;
	//			break;
	//		}
	//	}

	//	if (!added)
	//		mapped[fragmentDescriptor.first].push_back(
	//			fragmentDescriptor.second);
	//}

	for (auto& pair : mapped)
	{
		res.second.resize(size_t(pair.first) + 1);

		vk::DescriptorSetLayoutCreateInfo info;
		info.bindingCount = uint32_t(pair.second.size());
		info.pBindings = pair.second.data();

		res.second[pair.first] = vk.createDescriptorSetLayout(info);
	}

	std::vector<VkDescriptorSetLayout> layouts;
	layouts.reserve(res.second.size());
	for (auto& setLayout : res.second)
		layouts.push_back(setLayout.get());

	vk::PipelineLayoutCreateInfo info;
	info.setLayoutCount = uint32_t(layouts.size());
	info.pSetLayouts = layouts.data();
	info.pushConstantRangeCount = uint32_t(pushConstants.size());
	info.pPushConstantRanges =
		pushConstants.empty() ? nullptr : pushConstants.data();

	res.first = vk.createPipelineLayout(info);

	return res;
}

UniqueComputePipeline ComputePipelineFactory::createPipeline()
{
	const auto& vk = m_vulkanDevice.get();

	// VkStructureType                    sType;
	// const void*                        pNext;
	// VkPipelineCreateFlags              flags;
	// VkPipelineShaderStageCreateInfo    stage;
	// VkPipelineLayout                   layout;
	// VkPipeline                         basePipelineHandle;
	// int32_t                            basePipelineIndex;
	vk::ComputePipelineCreateInfo pipelineInfo;
	pipelineInfo.stage.module = m_computeModule;
	pipelineInfo.stage.pName = "main";
	pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	pipelineInfo.layout = m_layout;

	return vk.create(pipelineInfo);
}
}  // namespace cdm
