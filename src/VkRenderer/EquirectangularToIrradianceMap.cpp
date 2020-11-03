#include "EquirectangularToIrradianceMap.hpp"

#include "CommandBuffer.hpp"
#include "CommandBufferPool.hpp"
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
struct EquirectangularToIrradianceMapVertex
{
	vector3 position;
};

struct EquirectangularToIrradianceMapPushConstant
{
	matrix4 matrix;
};

EquirectangularToIrradianceMap::EquirectangularToIrradianceMap(
    RenderWindow& renderWindow, uint32_t resolution)
    : rw(renderWindow),
      m_resolution(resolution)
{
	auto& vk = rw.get().device();

	LogRRID log(vk);

#pragma region renderPass
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	m_renderPass = vk.create(renderPassInfo);
	if (!m_renderPass)
	{
		std::cerr << "error: failed to create render pass" << std::endl;
		abort();
	}
#pragma endregion

#define Locale(name, value) auto name = writer.declLocale(#name, value);
#define Constant(name, value) auto name = writer.declConstant(#name, value);

#pragma region vertexShader
	{
		using namespace sdw;
		VertexWriter writer;

		auto inPosition = writer.declInput<Vec3>("inPosition", 0);
		auto fragPosition = writer.declOutput<Vec3>("fragPosition", 0);

		auto pc = Pcb(writer, "pc");
		pc.declMember<Mat4>("matrix");
		pc.end();

		auto out = writer.getOut();

		writer.implementMain([&]() {
			fragPosition = inPosition;
			out.vtx.position =
			    pc.getMember<Mat4>("matrix") * vec4(inPosition, 1.0_f);
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

		auto fragPosition = writer.declInput<Vec3>("fragPosition", 0);
		auto fragColor = writer.declOutput<Vec4>("fragColor", 0);

		auto equirectangularMap =
		    writer.declSampledImage<FImg2DRgba32>("equirectangularMap", 0, 0);

		Constant(PI, 3.14159265359_f);
		Constant(invAtan, vec2(0.1591_f, 0.3183_f));

		auto SampleSphericalMap = writer.implementFunction<Vec2>(
		    "SampleSphericalMap",
		    [&](const Vec3& v) {
			    Locale(uv, vec2(atan2(v.z(), v.x()), asin(v.y())));
			    uv = uv * invAtan;
			    uv = uv + vec2(0.5_f);

			    writer.returnStmt(uv);
		    },
		    InVec3{ writer, "v" });

		writer.implementMain([&]() {
			Locale(N, normalize(fragPosition));

			Locale(irradiance, vec3(0.0_f));
			Locale(up, vec3(0.0_f, 1.0_f, 0.0_f));
			Locale(right, cross(up, N));
			up = cross(N, right);

			Locale(sampleDelta, 0.025_f);
			Locale(nrSamples, 0.0_f);

			auto tangentSample = writer.declLocale<Vec3>("tangentSample");
			auto sampleVec = writer.declLocale<Vec3>("sampleVec");

			FOR(writer, Float, phi, 0.0_f, phi < 2.0_f * PI,
			    phi += sampleDelta)
			{
				FOR(writer, Float, theta, 0.0_f, theta < 0.5_f * PI,
				    theta += sampleDelta)
				{
					tangentSample = vec3(sin(theta) * cos(phi),
					                     sin(theta) * sin(phi), cos(theta));
					sampleVec = normalize(vec3(tangentSample.x()) * right +
					                      vec3(tangentSample.y()) * up +
					                      vec3(tangentSample.z()) * N);

					irradiance +=
					    equirectangularMap.sample(
					                      SampleSphericalMap(sampleVec))
					                  .rgb() *
					              cos(theta) * sin(theta);
					nrSamples = nrSamples + 1.0_f;
				}
				ROF;
			}
			ROF;
			irradiance = PI * irradiance * vec3(1.0_f / nrSamples);

			fragColor = vec4(irradiance, 1.0_f);
			// fragColor = vec4(1.0_f);
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
	std::vector<vector3> vertices{
		// back face
		vector3{ -1.0f, -1.0f, -1.0f },
		vector3{ 1.0f, 1.0f, -1.0f },
		vector3{ 1.0f, -1.0f, -1.0f },
		vector3{ 1.0f, 1.0f, -1.0f },
		vector3{ -1.0f, -1.0f, -1.0f },
		vector3{ -1.0f, 1.0f, -1.0f },
		// front face
		vector3{ -1.0f, -1.0f, 1.0f },
		vector3{ 1.0f, -1.0f, 1.0f },
		vector3{ 1.0f, 1.0f, 1.0f },
		vector3{ 1.0f, 1.0f, 1.0f },
		vector3{ -1.0f, 1.0f, 1.0f },
		vector3{ -1.0f, -1.0f, 1.0f },
		// left face
		vector3{ -1.0f, 1.0f, 1.0f },
		vector3{ -1.0f, 1.0f, -1.0f },
		vector3{ -1.0f, -1.0f, -1.0f },
		vector3{ -1.0f, -1.0f, -1.0f },
		vector3{ -1.0f, -1.0f, 1.0f },
		vector3{ -1.0f, 1.0f, 1.0f },
		// right face
		vector3{ 1.0f, 1.0f, 1.0f },
		vector3{ 1.0f, -1.0f, -1.0f },
		vector3{ 1.0f, 1.0f, -1.0f },
		vector3{ 1.0f, -1.0f, -1.0f },
		vector3{ 1.0f, 1.0f, 1.0f },
		vector3{ 1.0f, -1.0f, 1.0f },
		// bottom face
		vector3{ -1.0f, -1.0f, -1.0f },
		vector3{ 1.0f, -1.0f, -1.0f },
		vector3{ 1.0f, -1.0f, 1.0f },
		vector3{ 1.0f, -1.0f, 1.0f },
		vector3{ -1.0f, -1.0f, 1.0f },
		vector3{ -1.0f, -1.0f, -1.0f },
		// top face
		vector3{ -1.0f, 1.0f, -1.0f },
		vector3{ 1.0f, 1.0f, 1.0f },
		vector3{ 1.0f, 1.0f, -1.0f },
		vector3{ 1.0f, 1.0f, 1.0f },
		vector3{ -1.0f, 1.0f, -1.0f },
		vector3{ -1.0f, 1.0f, 1.0f },
	};

	m_vertexBuffer = Buffer(
	    vk, vertices.size() * sizeof(*vertices.data()),
	    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	StagingBuffer verticesStagingBuffer(vk, vertices.data(), vertices.size());

	CommandBuffer copyCB(vk, rw.get().oneTimeCommandPool());
	copyCB.begin();
	copyCB.copyBuffer(verticesStagingBuffer, m_vertexBuffer,
	                  sizeof(*vertices.data()) * vertices.size());
	copyCB.end();

	if (vk.queueSubmit(vk.graphicsQueue(), copyCB.get()) != VK_SUCCESS)
		throw std::runtime_error("could not copy vertex buffer");
	vk.wait(vk.graphicsQueue());
#pragma endregion

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
		std::cerr << "error: failed to create descriptor pool" << std::endl;
		abort();
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
		std::cerr << "error: failed to allocate descriptor set" << std::endl;
		abort();
	}
#pragma endregion

#pragma region pipeline layout
	VkPushConstantRange pcRange{};
	pcRange.size = sizeof(EquirectangularToIrradianceMapPushConstant);
	pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout.get();
	// pipelineLayoutInfo.setLayoutCount = 0;
	// pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pcRange;
	// pipelineLayoutInfo.pushConstantRangeCount = 0;
	// pipelineLayoutInfo.pPushConstantRanges = nullptr;

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
	binding.stride = sizeof(EquirectangularToIrradianceMapVertex);

	VkVertexInputAttributeDescription positionAttribute = {};
	positionAttribute.binding = 0;
	positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
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

EquirectangularToIrradianceMap::~EquirectangularToIrradianceMap() = default;

Cubemap EquirectangularToIrradianceMap::computeCubemap(
    Texture2D& equirectangularTexture)
{
	auto& vk = rw.get().device();

#pragma region equirectangularHDR
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = equirectangularTexture.view();
	imageInfo.sampler = equirectangularTexture.sampler();

	vk::WriteDescriptorSet textureWrite;
	textureWrite.descriptorCount = 1;
	textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureWrite.dstArrayElement = 0;
	textureWrite.dstBinding = 0;
	textureWrite.dstSet = m_descriptorSet;
	textureWrite.pImageInfo = &imageInfo;

	vk.updateDescriptorSets(textureWrite);
#pragma endregion

#pragma region cubemap
	Cubemap cubemap(
	    rw, m_resolution, m_resolution, VK_FORMAT_R32G32B32A32_SFLOAT,
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
	        VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vk.debugMarkerSetObjectName(cubemap.image(),
	                            VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
	                            "irradiance cubemap");

	cubemap.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
#pragma endregion

#pragma region framebuffer
	vk::FramebufferCreateInfo framebufferInfo;
	framebufferInfo.renderPass = m_renderPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.pAttachments = &cubemap.viewFace0();
	framebufferInfo.width = m_resolution;
	framebufferInfo.height = m_resolution;
	framebufferInfo.layers = 1;

	framebufferInfo.pAttachments = &cubemap.viewFace0();
	UniqueFramebuffer framebuffer0 = vk.create(framebufferInfo);
	if (!framebuffer0)
	{
		std::cerr << "error: failed to create framebuffer" << std::endl;
		abort();
	}

	framebufferInfo.pAttachments = &cubemap.viewFace1();
	UniqueFramebuffer framebuffer1 = vk.create(framebufferInfo);
	if (!framebuffer1)
	{
		std::cerr << "error: failed to create framebuffer" << std::endl;
		abort();
	}

	framebufferInfo.pAttachments = &cubemap.viewFace2();
	UniqueFramebuffer framebuffer2 = vk.create(framebufferInfo);
	if (!framebuffer2)
	{
		std::cerr << "error: failed to create framebuffer" << std::endl;
		abort();
	}

	framebufferInfo.pAttachments = &cubemap.viewFace3();
	UniqueFramebuffer framebuffer3 = vk.create(framebufferInfo);
	if (!framebuffer3)
	{
		std::cerr << "error: failed to create framebuffer" << std::endl;
		abort();
	}

	framebufferInfo.pAttachments = &cubemap.viewFace4();
	UniqueFramebuffer framebuffer4 = vk.create(framebufferInfo);
	if (!framebuffer4)
	{
		std::cerr << "error: failed to create framebuffer" << std::endl;
		abort();
	}

	framebufferInfo.pAttachments = &cubemap.viewFace5();
	UniqueFramebuffer framebuffer5 = vk.create(framebufferInfo);
	if (!framebuffer5)
	{
		std::cerr << "error: failed to create framebuffer" << std::endl;
		abort();
	}

	std::array framebuffers{ framebuffer0.get(), framebuffer1.get(),
		                     framebuffer2.get(), framebuffer3.get(),
		                     framebuffer4.get(), framebuffer5.get() };
#pragma endregion

	auto pool = CommandBufferPool(vk, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

	matrix4 proj = matrix4::perspective(90_deg, 1.0f, 0.1f, 10.0f);

	//std::array views{
	//	matrix4::look_at({ 0, 0, 0 }, { 1, 0, 0 }, { 0, -1, 0 }),
	//	matrix4::look_at({ 0, 0, 0 }, { -1, 0, 0 }, { 0, -1, 0 }),
	//	matrix4::look_at({ 0, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }),
	//	matrix4::look_at({ 0, 0, 0 }, { 0, -1, 0 }, { 0, 0, -1 }),
	//	matrix4::look_at({ 0, 0, 0 }, { 0, 0, 1 }, { 0, -1, 0 }),
	//	matrix4::look_at({ 0, 0, 0 }, { 0, 0, -1 }, { 0, -1, 0 })
	//};
	std::array mvps{
		(proj * matrix4::look_at({ 0, 0, 0 }, { 1, 0, 0 }, { 0, -1, 0 }) ).get_transposed(),
		(proj * matrix4::look_at({ 0, 0, 0 }, { -1, 0, 0 }, { 0, -1, 0 })).get_transposed(),
		(proj * matrix4::look_at({ 0, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 })  ).get_transposed(),
		(proj * matrix4::look_at({ 0, 0, 0 }, { 0, -1, 0 }, { 0, 0, -1 })).get_transposed(),
		(proj * matrix4::look_at({ 0, 0, 0 }, { 0, 0, 1 }, { 0, -1, 0 }) ).get_transposed(),
		(proj * matrix4::look_at({ 0, 0, 0 }, { 0, 0, -1 }, { 0, -1, 0 })).get_transposed()
	};

	constexpr size_t RowCount = 4;
	constexpr size_t ColumnCount = 4;
	std::vector<vk::Rect2D> scissors(RowCount * ColumnCount);
	for (size_t y = 0; y < RowCount; y++)
	{
		vk::Rect2D scissor;
		scissor.extent.width = m_resolution / ColumnCount;
		scissor.extent.height = m_resolution / RowCount;
		scissor.offset.y = int32_t(y * scissor.extent.height);

		for (size_t x = 0; x < ColumnCount; x++)
		{
			scissor.offset.x = int32_t(x * scissor.extent.width);

			scissors[x + ColumnCount * y] = scissor;
		}
	}

	//vk::SubpassBeginInfo subpassBeginInfo;
	//subpassBeginInfo.contents = VK_SUBPASS_CONTENTS_INLINE;

	//vk::SubpassEndInfo subpassEndInfo;


	for (const auto& scissor : scissors)
	for (uint32_t i = 0; i < 6; i++)
	//for (uint32_t i = 0; i < 1; i++)
	{
		std::cout << i << std::endl;

		EquirectangularToIrradianceMapPushConstant pc;

		VkClearValue clearColor{};

		std::array clearValues = { clearColor };

		vk::RenderPassBeginInfo rpInfo;
		rpInfo.renderPass = m_renderPass;
		//rpInfo.renderArea.extent.width = m_resolution;
		//rpInfo.renderArea.extent.height = m_resolution;
		rpInfo.renderArea = scissor;
		rpInfo.clearValueCount = uint32_t(clearValues.size());
		rpInfo.pClearValues = clearValues.data();
		rpInfo.framebuffer = framebuffers[i];
		//rpInfo.framebuffer = framebuffers[3];

		auto& frame = pool.getAvailableCommandBuffer();
		auto& cb = frame.commandBuffer;

		cb.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		cb.debugMarkerBegin("render cubemap", 0.2f, 0.4f, 1.0f);

		//cb.beginRenderPass2(rpInfo, subpassBeginInfo);
		cb.beginRenderPass(rpInfo);

		cb.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
		cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
		                     0, m_descriptorSet);
		cb.bindVertexBuffer(m_vertexBuffer);

		//pc.matrix = proj * views[i];
		//pc.matrix.transpose();
		pc.matrix = mvps[i];
		//pc.matrix = mvps[0];

		cb.pushConstants(m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, &pc);
		cb.draw(36);

		//cb.endRenderPass2(subpassEndInfo);
		cb.endRenderPass();

		cb.debugMarkerEnd();
		cb.end();

		//if (vk.queueSubmit(vk.graphicsQueue(), cb, frame.fence) != VK_SUCCESS)
		if (frame.submit(vk.graphicsQueue()) != VK_SUCCESS)
		{
			std::cerr << "error: failed to submit imgui command buffer"
			          << std::endl;
			abort();
		}
		//vk.wait(frame.fence);
		//frame.reset();
	}

	//vk::ImageMemoryBarrier barrier;
	//barrier.image = cubemap.image();
	//barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	//barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	//barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	//barrier.subresourceRange.baseMipLevel = 0;
	//barrier.subresourceRange.levelCount = cubemap.mipLevels();
	//barrier.subresourceRange.baseArrayLayer = 0;
	//barrier.subresourceRange.layerCount = 6;
	//barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	//barrier.dstAccessMask = 0;
	//cb.pipelineBarrier(//VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
	//	VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	//                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	//                             //VK_DEPENDENCY_DEVICE_GROUP_BIT, barrier);
	//                             0,
	//	//VK_DEPENDENCY_BY_REGION_BIT,
	//	barrier);


	//frame.reset();

	pool.waitForAllCommandBuffers();

	cubemap.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	return cubemap;
}
}  // namespace cdm
