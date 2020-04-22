#include "Material.hpp"
#include "CommandBuffer.hpp"
#include "RenderPass.hpp"
#include "RenderWindow.hpp"

#include <array>
#include <iostream>

namespace cdm
{
Material::Material(RenderWindow& renderWindow, RenderPass& renderPass)
    : VulkanDeviceObject(renderWindow.device()),
      rw(renderWindow),
      m_renderPass(renderPass)
{
}

Material::~Material()
{
	auto& vk = device();

	vk.destroy(m_pipeline.get());
	vk.destroy(m_pipelineLayout.get());
	vk.destroy(m_vertexModule.get());
	vk.destroy(m_fragmentModule.get());
}

bool Material::setVertexShaderBytecode(std::vector<uint32_t> bytecode)
{
	auto& vk = this->device();

	vk::ShaderModuleCreateInfo createInfo;
	createInfo.codeSize = bytecode.size() * sizeof(*bytecode.data());
	createInfo.pCode = bytecode.data();

	VkShaderModule shaderModule = nullptr;
	if (vk.create(createInfo, shaderModule) != VK_SUCCESS)
	{
		std::cerr << "error: failed to create vertex shader module"
		          << std::endl;
		return false;
	}

	m_vertexShaderBytecode = std::move(bytecode);
	m_vertexShaderGLSL.clear();

	vk.destroy(m_vertexModule.get());
	m_vertexModule = shaderModule;

	return true;
}

bool Material::setFragmentShaderBytecode(std::vector<uint32_t> bytecode)
{
	auto& vk = this->device();

	vk::ShaderModuleCreateInfo createInfo;
	createInfo.codeSize = bytecode.size() * sizeof(*bytecode.data());
	createInfo.pCode = bytecode.data();

	VkShaderModule shaderModule = nullptr;
	if (vk.create(createInfo, shaderModule) != VK_SUCCESS)
	{
		std::cerr << "error: failed to create fragment shader module"
		          << std::endl;
		return false;
	}

	m_fragmentShaderBytecode = std::move(bytecode);
	m_fragmentShaderGLSL.clear();

	vk.destroy(m_fragmentModule.get());
	m_fragmentModule = shaderModule;

	return true;
}

bool Material::setVertexShaderGLSL(std::string code)
{
	auto vertexCompileResult = m_compiler.CompileGlslToSpv(
	    code, shaderc_shader_kind::shaderc_vertex_shader, "Vertex Shader");

	shaderc_compilation_status vertexStatus =
	    vertexCompileResult.GetCompilationStatus();
	if (vertexStatus !=
	    shaderc_compilation_status::shaderc_compilation_status_success)
	{
		std::cerr << vertexCompileResult.GetErrorMessage() << std::endl;
		return false;
	}

	std::vector<uint32_t> vertexBytecode;
	for (uint32_t i : vertexCompileResult)
		vertexBytecode.push_back(i);

	if (setVertexShaderBytecode(std::move(vertexBytecode)))
	{
		m_vertexShaderGLSL = std::move(code);
		return true;
	}

	return false;
}

bool Material::setFragmentShaderGLSL(std::string code)
{
	auto fragmentCompileResult = m_compiler.CompileGlslToSpv(
	    code, shaderc_shader_kind::shaderc_fragment_shader, "Fragment Shader");

	shaderc_compilation_status fragmentStatus =
	    fragmentCompileResult.GetCompilationStatus();
	if (fragmentStatus !=
	    shaderc_compilation_status::shaderc_compilation_status_success)
	{
		std::cerr << fragmentCompileResult.GetErrorMessage() << std::endl;
		return false;
	}

	std::vector<uint32_t> fragmentBytecode;
	for (uint32_t i : fragmentCompileResult)
		fragmentBytecode.push_back(i);

	if (setFragmentShaderBytecode(std::move(fragmentBytecode)))
	{
		m_fragmentShaderGLSL = std::move(code);
		return true;
	}

	return false;
}

const std::vector<uint32_t>& Material::vertexShaderBytecode() const
{
	return m_vertexShaderBytecode;
}

const std::vector<uint32_t>& Material::fragmentShaderBytecode() const
{
	return m_fragmentShaderBytecode;
}

std::string Material::vertexShaderGLSL() const { return m_vertexShaderGLSL; }

std::string Material::fragmentShaderGLSL() const
{
	return m_fragmentShaderGLSL;
}

void Material::setRenderPass(RenderPass& renderPass)
{
	m_renderPass = renderPass;

	setCreationTime();
}

bool Material::buildPipeline()
{
	if (!m_vertexModule.get() || !m_fragmentModule.get())
	{
		std::cerr
		    << "error: missing fragment shader module or vertex shader module"
		    << std::endl;
		return false;
	}

	auto& vk = this->device();

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = m_vertexModule.get();
	vertShaderStageInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = m_fragmentModule.get();
	fragShaderStageInfo.pName = "main";

	std::array shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;  // Optional
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;  // Optional

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = false;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = float(rw.get().swapchainExtent().width);
	viewport.height = float(rw.get().swapchainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = rw.get().swapchainExtent();

	vk::PipelineViewportStateCreateInfo viewportState;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	vk::PipelineRasterizationStateCreateInfo rasterizer;
	rasterizer.depthClampEnable = false;
	rasterizer.rasterizerDiscardEnable = false;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = false;
	rasterizer.depthBiasConstantFactor = 0.0f;  // Optional
	rasterizer.depthBiasClamp = 0.0f;           // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f;     // Optional

	vk::PipelineMultisampleStateCreateInfo multisampling;
	multisampling.sampleShadingEnable = false;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;        // Optional
	multisampling.pSampleMask = nullptr;          // Optional
	multisampling.alphaToCoverageEnable = false;  // Optional
	multisampling.alphaToOneEnable = false;       // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask =
	    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
	    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = false;
	colorBlendAttachment.srcColorBlendFactor =
	    VK_BLEND_FACTOR_ONE;  // Optional
	colorBlendAttachment.dstColorBlendFactor =
	    VK_BLEND_FACTOR_ZERO;                             // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;  // Optional
	colorBlendAttachment.srcAlphaBlendFactor =
	    VK_BLEND_FACTOR_ONE;  // Optional
	colorBlendAttachment.dstAlphaBlendFactor =
	    VK_BLEND_FACTOR_ZERO;                             // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;  // Optional

	// colorBlendAttachment.blendEnable = true;
	// colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	// colorBlendAttachment.dstColorBlendFactor =
	// VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; colorBlendAttachment.colorBlendOp =
	// VK_BLEND_OP_ADD; colorBlendAttachment.srcAlphaBlendFactor =
	// VK_BLEND_FACTOR_ONE; colorBlendAttachment.dstAlphaBlendFactor =
	// VK_BLEND_FACTOR_ZERO; colorBlendAttachment.alphaBlendOp =
	// VK_BLEND_OP_ADD;

	vk::PipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;  // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;  // Optional
	colorBlending.blendConstants[1] = 0.0f;  // Optional
	colorBlending.blendConstants[2] = 0.0f;  // Optional
	colorBlending.blendConstants[3] = 0.0f;  // Optional

	std::array dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT,
		                         VK_DYNAMIC_STATE_LINE_WIDTH };

	vk::PipelineDynamicStateCreateInfo dynamicState;
	dynamicState.dynamicStateCount = uint32_t(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.setLayoutCount = 0;             // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr;          // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0;     // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr;  // Optional

	if (vk.create(pipelineLayoutInfo, m_pipelineLayout.get()) != VK_SUCCESS)
	{
		std::cerr << "error: failed to create pipeline layout" << std::endl;
		return false;
	}

	vk::GraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.stageCount = uint32_t(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;  // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;  // Optional
	pipelineInfo.layout = m_pipelineLayout.get();
	pipelineInfo.renderPass = m_renderPass.get().renderPass();
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = nullptr;  // Optional
	pipelineInfo.basePipelineIndex = -1;        // Optional

	VkResult res = vk.create(pipelineInfo, m_pipeline.get());
	if (res != VK_SUCCESS)
	{
		std::cerr << "error: failed to create graphics pipeline" << std::endl;
		return false;
	}

	return true;
}

void Material::bind(CommandBuffer& commandBuffer)
{
	commandBuffer.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline());
}

VkPipelineLayout Material::pipelineLayout() const
{
	return m_pipelineLayout.get();
}

VkPipeline Material::pipeline()
{
	if (outdated())
		recreate();

	return m_pipeline.get();
}

bool Material::outdated() const
{
	return m_renderPass.get().creationTime() > creationTime();
}

void Material::recreate()
{
	auto& vk = this->device();

	vk.destroy(m_pipeline.get());
	vk.destroy(m_pipelineLayout.get());

	buildPipeline();
}
}  // namespace cdm
