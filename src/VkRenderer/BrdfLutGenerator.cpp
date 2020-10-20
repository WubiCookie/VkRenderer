#include "BrdfLutGenerator.hpp"

#include "CommandBuffer.hpp"
#include "StagingBuffer.hpp"

#include <CompilerSpirV/compileSpirV.hpp>
#include <ShaderWriter/Intrinsics/Intrinsics.hpp>
#include <ShaderWriter/Source.hpp>

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include "cdm_maths.hpp"

#include "stb_image.h"

#include <array>
#include <iostream>
#include <stdexcept>

namespace cdm
{
BrdfLutGenerator::BrdfLutGenerator(RenderWindow& renderWindow,
                                   uint32_t resolution)
    : rw(renderWindow),
      m_resolution(resolution)
{
	auto& vk = rw.get().device();

	LogRRID log(vk);

#pragma region renderPass
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = VK_FORMAT_R32G32_SFLOAT;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	std::array colorAttachments{ colorAttachment };

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::array colorAttachmentRefs{ colorAttachmentRef };

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = uint32_t(colorAttachmentRefs.size());
	subpass.pColorAttachments = colorAttachmentRefs.data();
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.pResolveAttachments = nullptr;
	subpass.pDepthStencilAttachment = nullptr;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;

	VkSubpassDependency dependency = {};
	dependency.dependencyFlags = 0;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
	                           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.attachmentCount = uint32_t(colorAttachments.size());
	renderPassInfo.pAttachments = colorAttachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	// renderPassInfo.dependencyCount = 1;
	// renderPassInfo.pDependencies = &dependency;

	m_renderPass = vk.create(renderPassInfo);
	if (!m_renderPass)
	{
		std::cerr << "error: failed to create render pass" << std::endl;
		abort();
	}
#pragma endregion

#define Locale(name, value) auto name = writer.declLocale(#name, value);
#define FLOAT(name) auto name = writer.declLocale<Float>(#name);
#define VEC2(name) auto name = writer.declLocale<Vec2>(#name);
#define VEC3(name) auto name = writer.declLocale<Vec3>(#name);
#define Constant(name, value) auto name = writer.declConstant(#name, value);

#pragma region vertexShader
	{
		using namespace sdw;
		VertexWriter writer;

		auto inPosition = writer.declInput<Vec2>("inPosition", 0);
		auto fragPosition = writer.declOutput<Vec2>("fragPosition", 0);

		auto out = writer.getOut();

		writer.implementMain([&]() {
			fragPosition = inPosition;
			out.vtx.position = vec4(inPosition, 0.0_f, 1.0_f);
		});

		std::vector<uint32_t> bytecode =
		    spirv::serialiseSpirv(writer.getShader());

		vk::ShaderModuleCreateInfo createInfo;
		createInfo.codeSize = bytecode.size() * sizeof(*bytecode.data());
		createInfo.pCode = bytecode.data();

		m_vertexModule = vk.create(createInfo);
		if (!m_vertexModule)
		{
			std::cerr << "error: failed to create vertex shader module"
			          << std::endl;
			abort();
		}
	}
#pragma endregion

#pragma region fragmentShader
	{
		using namespace sdw;
		FragmentWriter writer;

		auto in = writer.getIn();

		auto fragPosition = writer.declInput<Vec2>("fragPosition", 0);
		auto fragColor = writer.declOutput<Vec2>("fragColor", 0);

		//*
		Constant(PI, 3.14159265359_f);

		auto RadicalInverse_VdC = writer.implementFunction<Float>(
		    "RadicalInverse_VdC",
		    [&](const UInt& bits_arg) {
			    Locale(bits, bits_arg);

			    bits = (bits << 16_u) | (bits >> 16_u);
			    bits = ((bits & 0x55555555_u) << 1_u) |
			           ((bits & 0xAAAAAAAA_u) >> 1_u);
			    bits = ((bits & 0x33333333_u) << 2_u) |
			           ((bits & 0xCCCCCCCC_u) >> 2_u);
			    bits = ((bits & 0x0F0F0F0F_u) << 4_u) |
			           ((bits & 0xF0F0F0F0_u) >> 4_u);
			    bits = ((bits & 0x00FF00FF_u) << 8_u) |
			           ((bits & 0xFF00FF00_u) >> 8_u);

			    writer.returnStmt(writer.cast<Float>(bits) *
			                      2.3283064365386963e-10_f);
		    },
		    InUInt{ writer, "bits_arg" });

		auto Hammersley = writer.implementFunction<Vec2>(
		    "Hammersley",
		    [&](const UInt& i, const UInt& N) {
			    writer.returnStmt(
			        vec2(writer.cast<Float>(i) / writer.cast<Float>(N),
			             RadicalInverse_VdC(i)));
		    },
		    InUInt{ writer, "i" }, InUInt{ writer, "N" });

		auto ImportanceSampleGGX = writer.implementFunction<Vec3>(
		    "ImportanceSampleGGX",
		    [&](const Vec2& Xi, const Vec3& N, const Float& roughness) {
			    Locale(a, roughness * roughness);

			    Locale(phi, 2.0_f * PI * Xi.x());
			    Locale(cosTheta, sqrt((1.0_f - Xi.y()) /
			                          (1.0_f + (a * a - 1.0_f) * Xi.y())));
			    Locale(sinTheta, sqrt(1.0_f - cosTheta * cosTheta));

			    Locale(H, vec3(cos(phi) * sinTheta, sin(phi) * sinTheta,
			                   cosTheta));

			    Locale(up, TERNARY(writer, Vec3, abs(N.z()) < 0.999_f,
			                       vec3(0.0_f, 0.0_f, 1.0_f),
			                       vec3(1.0_f, 0.0_f, 0.0_f)));
			    Locale(tangent, normalize(cross(up, N)));
			    Locale(bitangent, cross(N, tangent));

			    Locale(sampleVec,
			           tangent * H.x() + bitangent * H.y() + N * H.z());
			    writer.returnStmt(normalize(sampleVec));
		    },
		    InVec2{ writer, "Xi" }, InVec3{ writer, "N" },
		    InFloat{ writer, "roughness" });

		auto GeometrySchlickGGX = writer.implementFunction<Float>(
		    "GeometrySchlickGGX",
		    [&](const Float& NdotV, const Float& roughness) {
			    Locale(a, roughness);
			    Locale(k, (a * a) / 2.0_f);

			    Locale(denom, NdotV * (1.0_f - k) + k);

			    writer.returnStmt(NdotV / denom);
		    },
		    InFloat{ writer, "NdotV" }, InFloat{ writer, "roughness" });

		auto GeometrySmith = writer.implementFunction<Float>(
		    "GeometrySmith",
		    [&](const Vec3& N, const Vec3& V, const Vec3& L,
		        const Float& roughness) {
			    Locale(NdotV, max(dot(N, V), 0.0_f));
			    Locale(NdotL, max(dot(N, L), 0.0_f));
			    Locale(ggx1, GeometrySchlickGGX(NdotV, roughness));
			    Locale(ggx2, GeometrySchlickGGX(NdotL, roughness));

			    writer.returnStmt(ggx1 * ggx2);
		    },
		    InVec3{ writer, "N" }, InVec3{ writer, "V" },
		    InVec3{ writer, "L" }, InFloat{ writer, "roughness" });
		//*/

		uint32_t m_sampleCount = 2048;

		auto IntegrateBRDF = writer.implementFunction<Vec2>(
		    "IntegrateBRDF",
		    [&](const Float& NdotV, const Float& roughness) {
			    Locale(V, vec3(sqrt(1.0_f - NdotV * NdotV), 0.0_f, NdotV));

			    Locale(A, 0.0_f);
			    Locale(B, 0.0_f);

			    Locale(N, vec3(0.0_f, 0.0_f, 1.0_f));

			    Locale(SAMPLE_COUNT, UInt(m_sampleCount));
			    Locale(SAMPLE_COUNTf, Float(float(m_sampleCount)));

			    VEC2(Xi);
			    VEC3(H);
			    VEC3(L);
			    FLOAT(NdotL);
			    FLOAT(NdotH);
			    FLOAT(VdotH);
			    FLOAT(G);
			    FLOAT(G_Vis);
			    FLOAT(Fc);

			    FOR(writer, UInt, i, 0_u, i < SAMPLE_COUNT, i++)
			    {
				    Xi = Hammersley(i, SAMPLE_COUNT);
				    H = ImportanceSampleGGX(Xi, N, roughness);
				    L = normalize(2.0_f * dot(V, H) * H - V);

				    NdotL = max(L.z(), 0.0_f);
				    NdotH = max(H.z(), 0.0_f);
				    VdotH = max(dot(V, H), 0.0_f);

				    IF(writer, NdotL > 0.0_f)
				    {
					    G = GeometrySmith(N, V, L, roughness);
					    G_Vis = (G * VdotH) / (NdotH * NdotV);
					    Fc = pow(1.0_f - VdotH, 5.0_f);

					    A += (1.0_f - Fc) * G_Vis;
					    B += Fc * G_Vis;
				    }
				    FI;
			    }
			    ROF;

			    A /= SAMPLE_COUNTf;
			    B /= SAMPLE_COUNTf;

			    writer.returnStmt(vec2(A, B));
		    },
		    InFloat{ writer, "NdotV" }, InFloat{ writer, "roughness" });

		writer.implementMain([&]() {
			fragColor = IntegrateBRDF(fragPosition.x() / 2.0_f + 0.5_f,
			                          fragPosition.y() / 2.0_f + 0.5_f);
		});

		std::vector<uint32_t> bytecode =
		    spirv::serialiseSpirv(writer.getShader());

		vk::ShaderModuleCreateInfo createInfo;
		createInfo.codeSize = bytecode.size() * sizeof(*bytecode.data());
		createInfo.pCode = bytecode.data();

		m_fragmentModule = vk.create(createInfo);
		if (!m_fragmentModule)
		{
			std::cerr << "error: failed to create fragment shader module"
			          << std::endl;
			abort();
		}
	}
#pragma endregion

#pragma region vertex buffer
	std::vector<vector2> vertices{
		vector2{ -1.0f, -1.0f }, vector2{ 1.0f, 1.0f },
		vector2{ 1.0f, -1.0f },  vector2{ 1.0f, 1.0f },
		vector2{ -1.0f, -1.0f }, vector2{ -1.0f, 1.0f }
	};

	m_vertexBuffer = Buffer(
	    rw, vertices.size() * sizeof(*vertices.data()),
	    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	StagingBuffer verticesStagingBuffer(rw, vertices.data(), vertices.size());

	CommandBuffer copyCB(vk, rw.get().oneTimeCommandPool());
	copyCB.begin();
	copyCB.copyBuffer(verticesStagingBuffer, m_vertexBuffer,
	                  sizeof(*vertices.data()) * vertices.size());
	copyCB.end();

	if (vk.queueSubmit(vk.graphicsQueue(), copyCB.get()) != VK_SUCCESS)
		throw std::runtime_error("could not copy vertex buffer");
	vk.wait(vk.graphicsQueue());
#pragma endregion

	/*
	#pragma region descriptor pool
	    std::array poolSizes{ VkDescriptorPoolSize{
	        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 } };

	    vk::DescriptorPoolCreateInfo poolInfo;
	    poolInfo.maxSets = 1;
	    poolInfo.poolSizeCount = uint32_t(poolSizes.size());
	    poolInfo.pPoolSizes = poolSizes.data();

	    m_descriptorPool = vk.create(poolInfo);
	    if (!m_descriptorPool)
	    {
	        std::cerr << "error: failed to create descriptor pool" <<
	std::endl; abort();
	    }
	#pragma endregion

	#pragma region descriptor set layout
	    VkDescriptorSetLayoutBinding layoutBindingUbo{};
	    layoutBindingUbo.binding = 0;
	    layoutBindingUbo.descriptorCount = 1;
	    layoutBindingUbo.descriptorType =
	        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	    layoutBindingUbo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	    std::array layoutBindings{ layoutBindingUbo };

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
	        std::cerr << "error: failed to allocate descriptor set" <<
	std::endl; abort();
	    }
	#pragma endregion
	//*/

#pragma region pipeline layout
	// VkPushConstantRange pcRange{};
	// pcRange.size = sizeof(BrdfLutGeneratorPushConstant);
	// pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	// pipelineLayoutInfo.setLayoutCount = 1;
	// pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout.get();
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	// pipelineLayoutInfo.pushConstantRangeCount = 1;
	// pipelineLayoutInfo.pPushConstantRanges = &pcRange;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	m_pipelineLayout = vk.create(pipelineLayoutInfo);
	if (!m_pipelineLayout)
	{
		std::cerr << "error: failed to create pipeline layout" << std::endl;
		abort();
	}
#pragma endregion

#pragma region pipeline
	vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = m_vertexModule;
	vertShaderStageInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = m_fragmentModule;
	fragShaderStageInfo.pName = "main";

	std::array shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

	VkVertexInputBindingDescription binding = {};
	binding.binding = 0;
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	binding.stride = sizeof(vector2);

	VkVertexInputAttributeDescription positionAttribute = {};
	positionAttribute.binding = 0;
	positionAttribute.format = VK_FORMAT_R32G32_SFLOAT;
	positionAttribute.location = 0;
	positionAttribute.offset = 0;

	std::array attributes = { positionAttribute };

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &binding;
	vertexInputInfo.vertexAttributeDescriptionCount =
	    uint32_t(attributes.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	// inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	inputAssembly.primitiveRestartEnable = false;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = float(m_resolution);
	viewport.height = float(m_resolution);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent.width = m_resolution;
	scissor.extent.height = m_resolution;

	vk::PipelineViewportStateCreateInfo viewportState;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	vk::PipelineRasterizationStateCreateInfo rasterizer;
	rasterizer.depthClampEnable = false;
	rasterizer.rasterizerDiscardEnable = false;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	// rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;  // VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = false;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	vk::PipelineMultisampleStateCreateInfo multisampling;
	multisampling.sampleShadingEnable = false;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = false;
	multisampling.alphaToOneEnable = false;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask =
	    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
	    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = false;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	std::array colorBlendAttachments{ colorBlendAttachment };

	vk::PipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.logicOpEnable = false;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = uint32_t(colorBlendAttachments.size());
	colorBlending.pAttachments = colorBlendAttachments.data();
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	std::array dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT,
		                         VK_DYNAMIC_STATE_LINE_WIDTH };

	vk::PipelineDynamicStateCreateInfo dynamicState;
	dynamicState.dynamicStateCount = uint32_t(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	vk::GraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.stageCount = uint32_t(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.renderPass = m_renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = nullptr;
	pipelineInfo.basePipelineIndex = -1;

	m_pipeline = vk.create(pipelineInfo);
	if (!m_pipeline)
	{
		std::cerr << "error: failed to create graphics pipeline" << std::endl;
		abort();
	}
#pragma endregion
}

BrdfLutGenerator::~BrdfLutGenerator() {}

Texture2D BrdfLutGenerator::computeBrdfLut()
{
	auto& vk = rw.get().device();

#pragma region lut
	Texture2D lut(
	    rw, m_resolution, m_resolution, VK_FORMAT_R32G32_SFLOAT,
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
	        VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vk.debugMarkerSetObjectName(
	    lut.image(), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "BRDF LUT");

	lut.transitionLayoutImmediate(VK_IMAGE_LAYOUT_UNDEFINED,
	                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
#pragma endregion

#pragma region framebuffer
	vk::FramebufferCreateInfo framebufferInfo;
	framebufferInfo.renderPass = m_renderPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.pAttachments = &lut.view();
	framebufferInfo.width = m_resolution;
	framebufferInfo.height = m_resolution;
	framebufferInfo.layers = 1;

	UniqueFramebuffer framebuffer = vk.create(framebufferInfo);
	if (!framebuffer)
	{
		std::cerr << "error: failed to create framebuffer" << std::endl;
		abort();
	}
#pragma endregion

	vk.wait(vk.graphicsQueue());

	//CommandBuffer cb(vk, rw.get().oneTimeCommandPool());
	auto& frame = rw.get().getAvailableCommandBuffer();
	frame.reset();
	CommandBuffer& cb = frame.commandBuffer;

	VkClearValue clearColor{};

	std::array clearValues = { clearColor };

	vk::RenderPassBeginInfo rpInfo;
	rpInfo.renderPass = m_renderPass;
	rpInfo.renderArea.extent.width = m_resolution;
	rpInfo.renderArea.extent.height = m_resolution;
	rpInfo.clearValueCount = uint32_t(clearValues.size());
	rpInfo.pClearValues = clearValues.data();
	rpInfo.framebuffer = framebuffer;

	vk::SubpassBeginInfo subpassBeginInfo;
	subpassBeginInfo.contents = VK_SUBPASS_CONTENTS_INLINE;

	vk::SubpassEndInfo subpassEndInfo;

	constexpr size_t RowCount = 4;
	constexpr size_t ColumnCount = 4;
	std::vector<vk::Rect2D> scissors(RowCount * ColumnCount);
	for (size_t y = 0; y < RowCount; y++)
	{
		vk::Rect2D scissor;
		scissor.extent.width = m_resolution / ColumnCount;
		scissor.extent.height = m_resolution / RowCount;
		scissor.offset.y = y * scissor.extent.height;

		for (size_t x = 0; x < ColumnCount; x++)
		{
			scissor.offset.x = x * scissor.extent.width;

			scissors[x + ColumnCount * y] = scissor;
		}
	}

	for (const auto& scissor : scissors)
	{
		cb.begin();
		cb.debugMarkerBegin("generate BRDF LUT", 0.2f, 0.8f, 0.8f);
		rpInfo.renderArea = scissor;
		cb.beginRenderPass2(rpInfo, subpassBeginInfo);

		cb.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
		// cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS,
		// m_pipelineLayout, 0,
		//                     m_descriptorSet);
		cb.bindVertexBuffer(m_vertexBuffer);
		cb.draw(6);

		cb.endRenderPass2(subpassEndInfo);

		cb.debugMarkerEnd();
		cb.end();
		if (vk.queueSubmit(vk.graphicsQueue(), cb, frame.fence) != VK_SUCCESS)
		{
			std::cerr << "error: failed to submit imgui command buffer"
			          << std::endl;
			abort();
		}
		vk.wait(frame.fence);
		frame.reset();
	}

	lut.transitionLayoutImmediate(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	return lut;
}
}  // namespace cdm
