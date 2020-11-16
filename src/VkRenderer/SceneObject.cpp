#include "SceneObject.hpp"

#include "CommandBuffer.hpp"
#include "Material.hpp"
#include "RenderWindow.hpp"
#include "Scene.hpp"
#include "StandardMesh.hpp"

#include <iostream>

namespace cdm
{
SceneObject::Pipeline::Pipeline(Scene& s, StandardMesh& mesh,
                                MaterialInterface& material,
                                VkRenderPass renderPass)
    : scene(&s),
      mesh(&mesh),
      material(&material),
      renderPass(renderPass)
{
	auto& rw = material.material().renderWindow();
	auto& vk = rw.device();

#pragma region vertexShader
	{
		using namespace sdw;
		VertexWriter writer;

		Scene::SceneUbo sceneUbo(writer);
		Scene::ModelUbo modelUbo(writer);
		Scene::ModelPcb modelPcb(writer);

		auto shaderVertexInput = mesh.shaderVertexInput(writer);

		auto fragPosition = writer.declOutput<Vec3>("fragPosition", 0);
		auto fragUV = writer.declOutput<Vec2>("fragUV", 1);
		auto fragNormal = writer.declOutput<Vec3>("fragNormal", 2);
		auto fragTangent = writer.declOutput<Vec3>("fragTangent", 3);
		auto fragDistance = writer.declOutput<sdw::Float>("fragDistance", 4);

		auto out = writer.getOut();

		auto materialVertexShaderBuildData =
		    material.material().instantiateVertexShaderBuildData();
		auto materialVertexFunction = material.material().vertexFunction(
		    writer, materialVertexShaderBuildData.get());

		writer.implementMain([&]() {
			auto model = modelUbo.getModel()[modelPcb.getModelId()];
			auto view = sceneUbo.getView();
			auto proj = sceneUbo.getProj();

			fragPosition =
			    (model * vec4(shaderVertexInput.inPosition, 1.0_f)).xyz();
			fragUV = shaderVertexInput.inUV;

			Locale(model3, mat3(vec3(model[0][0], model[0][1], model[0][2]),
			                    vec3(model[1][0], model[1][1], model[1][2]),
			                    vec3(model[2][0], model[2][1], model[2][2])));

			Locale(normalMatrix, transpose(inverse(model3)));
			// Locale(normalMatrix, transpose((model3)));

			fragNormal = shaderVertexInput.inNormal;

			materialVertexFunction(fragPosition, fragNormal);

			fragNormal = normalize(normalMatrix * fragNormal);
			fragTangent =
			    normalize(normalMatrix * shaderVertexInput.inTangent);

			// fragNormal = normalize((model * vec4(fragNormal, 0.0_f)).xyz());
			// fragTangent = normalize(
			//    (model * vec4(shaderVertexInput.inTangent, 0.0_f)).xyz());

			fragTangent = normalize(fragTangent -
			                        dot(fragTangent, fragNormal) * fragNormal);

			// Locale(B, cross(fragNormal, fragTangent));

			// Locale(TBN, transpose(mat3(fragTangent, B, fragNormal)));

			// fragTanLightPos = TBN * sceneUbo.getLightPos();
			// fragTanViewPos = TBN * sceneUbo.getViewPos();
			// fragTanFragPos = TBN * fragPosition;

			fragDistance =
			    (view * model * vec4(shaderVertexInput.inPosition, 1.0_f)).z();

			out.vtx.position = proj * view * model *
			                   vec4(shaderVertexInput.inPosition, 1.0_f);
		});

		std::vector<uint32_t> bytecode =
		    spirv::serialiseSpirv(writer.getShader());

		vk::ShaderModuleCreateInfo createInfo;
		createInfo.codeSize = bytecode.size() * sizeof(*bytecode.data());
		createInfo.pCode = bytecode.data();

		vertexModule = vk.create(createInfo);
		if (!vertexModule)
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

		Scene::SceneUbo sceneUbo(writer);
		Scene::ModelPcb modelPcb(writer);

		auto fragPosition = writer.declInput<sdw::Vec3>("fragPosition", 0);
		auto fragUV = writer.declInput<sdw::Vec2>("fragUV", 1);
		auto fragNormal = writer.declInput<sdw::Vec3>("fragNormal", 2);
		auto fragTangent = writer.declInput<sdw::Vec3>("fragTangent", 3);
		auto fragDistance = writer.declInput<sdw::Float>("fragDistance", 4);

		auto fragColor = writer.declOutput<Vec4>("fragColor", 0);
		auto fragID = writer.declOutput<UInt>("fragID", 1);
		auto fragNormalDepth = writer.declOutput<Vec4>("fragNormalDepth", 2);
		auto fragPos = writer.declOutput<Vec3>("fragPos", 3);

		auto fragmentShaderBuildData =
		    material.material().instantiateFragmentShaderBuildData();
		auto materialFragmentFunction = material.material().fragmentFunction(
		    writer, fragmentShaderBuildData.get());

		auto shadingModelFragmentShaderBuildData =
		    material.material()
		        .shadingModel()
		        .instantiateFragmentShaderBuildData();
		auto combinedMaterialFragmentFunction =
		    material.material()
		        .shadingModel()
		        .combinedMaterialFragmentFunction(
		            writer, materialFragmentFunction,
		            shadingModelFragmentShaderBuildData.get(), sceneUbo);

		writer.implementMain([&]() {
			Locale(materialInstanceId, modelPcb.getMaterialInstanceId() + 1_u);
			materialInstanceId -= 1_u;
			Locale(normal, normalize(fragNormal));
			Locale(tangent, normalize(fragTangent));
			fragColor = combinedMaterialFragmentFunction(
			    materialInstanceId, fragPosition, fragUV, normal, tangent);
			fragID = modelPcb.getModelId();
			fragNormalDepth.xyz() = fragNormal;
			fragNormalDepth.w() = fragDistance;
			fragPos = fragPosition;
		});

		std::vector<uint32_t> bytecode =
		    spirv::serialiseSpirv(writer.getShader());

		vk::ShaderModuleCreateInfo createInfo;
		createInfo.codeSize = bytecode.size() * sizeof(*bytecode.data());
		createInfo.pCode = bytecode.data();

		fragmentModule = vk.create(createInfo);
		if (!fragmentModule)
		{
			std::cerr << "error: failed to create fragment shader module"
			          << std::endl;
			abort();
		}
	}
#pragma endregion

#pragma region pipeline layout
	VkPushConstantRange pcRange{};
	pcRange.size = sizeof(PcbStruct);
	pcRange.stageFlags =
	    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array descriptorSetLayouts{
		scene.get()->descriptorSetLayout(),
		material.material().shadingModel().m_descriptorSetLayout.get(),
		material.material().descriptorSetLayout(),
	};

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.setLayoutCount = uint32_t(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	// pipelineLayoutInfo.setLayoutCount = 0;
	// pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pcRange;
	// pipelineLayoutInfo.pushConstantRangeCount = 0;
	// pipelineLayoutInfo.pPushConstantRanges = nullptr;

	pipelineLayout = vk.create(pipelineLayoutInfo);
	if (!pipelineLayout)
	{
		std::cerr << "error: failed to create pipeline layout" << std::endl;
		abort();
	}
#pragma endregion

#pragma region pipeline
	vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertexModule;
	vertShaderStageInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragmentModule;
	fragShaderStageInfo.pName = "main";

	std::array shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

	auto vertexInputState = mesh.vertexInputState();

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	// inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	inputAssembly.primitiveRestartEnable = false;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	/// TODO get from render pass
	viewport.width = float(1280);
	viewport.height = float(720);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	/// TODO get from render pass
	scissor.extent.width = 1280;
	scissor.extent.height = 720;

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
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = false;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	/// TODO get from render pass
	vk::PipelineMultisampleStateCreateInfo multisampling;
	multisampling.sampleShadingEnable = false;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_4_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = false;
	multisampling.alphaToOneEnable = false;

	/// TODO get from render pass and material
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

	// colorBlendAttachment.blendEnable = true;
	// colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	// colorBlendAttachment.dstColorBlendFactor =
	//    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	// colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	// colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	// colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	// colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	/// TODO get from render pass and material
	VkPipelineColorBlendAttachmentState objectIDBlendAttachment = {};
	objectIDBlendAttachment.colorWriteMask =
	    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
	    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	objectIDBlendAttachment.blendEnable = false;
	objectIDBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	objectIDBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	objectIDBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	objectIDBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	objectIDBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	objectIDBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendAttachmentState normalDepthBlendAttachment = {};
	normalDepthBlendAttachment.colorWriteMask =
	    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
	    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	normalDepthBlendAttachment.blendEnable = false;
	normalDepthBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	normalDepthBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	normalDepthBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	normalDepthBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	normalDepthBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	normalDepthBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendAttachmentState positionhBlendAttachment = {};
	positionhBlendAttachment.colorWriteMask =
	    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
	    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	positionhBlendAttachment.blendEnable = false;
	positionhBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	positionhBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	positionhBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	positionhBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	positionhBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	positionhBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	// colorHDRBlendAttachment.blendEnable = true;
	// colorHDRBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	// colorHDRBlendAttachment.dstColorBlendFactor =
	//    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	// colorHDRBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	// colorHDRBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	// colorHDRBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	// colorHDRBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	std::array colorBlendAttachments{ colorBlendAttachment,
		                              objectIDBlendAttachment,
		                              normalDepthBlendAttachment,
		                              positionhBlendAttachment };

	/// TODO get from render pass
	vk::PipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.logicOpEnable = false;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = uint32_t(colorBlendAttachments.size());
	colorBlending.pAttachments = colorBlendAttachments.data();
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	vk::PipelineDepthStencilStateCreateInfo depthStencil;
	depthStencil.depthTestEnable = true;
	depthStencil.depthWriteEnable = true;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = false;
	depthStencil.minDepthBounds = 0.0f;  // Optional
	depthStencil.maxDepthBounds = 1.0f;  // Optional
	depthStencil.stencilTestEnable = false;
	// depthStencil.front; // Optional
	// depthStencil.back; // Optional

	std::array dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	vk::PipelineDynamicStateCreateInfo dynamicState;
	dynamicState.dynamicStateCount = uint32_t(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	vk::GraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.stageCount = uint32_t(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputState.vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = nullptr;
	pipelineInfo.basePipelineIndex = -1;

	pipeline = vk.create(pipelineInfo);
	if (!pipeline)
	{
		std::cerr << "error: failed to create graphics pipeline" << std::endl;
		abort();
	}
#pragma endregion
}

void SceneObject::Pipeline::bindPipeline(CommandBuffer& cb)
{
	cb.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void SceneObject::Pipeline::bindDescriptorSet(CommandBuffer& cb)
{
	std::array<VkDescriptorSet, 3> descriptorSets{
		scene.get()->descriptorSet(),
		material.get()->material().shadingModel().m_descriptorSet,
		material.get()->material().descriptorSet(),
	};
	cb.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0,
	                      uint32_t(descriptorSets.size()),
	                      descriptorSets.data());
}

void SceneObject::Pipeline::draw(CommandBuffer& cb) { mesh.get()->draw(cb); }

SceneObject::ShadowmapPipeline::ShadowmapPipeline(Scene& s, StandardMesh& mesh,
                                                  MaterialInterface& material,
                                                  VkRenderPass renderPass)
    : scene(&s),
      mesh(&mesh),
      material(&material),
      renderPass(renderPass)
{
	auto& rw = material.material().renderWindow();
	auto& vk = rw.device();

#pragma region vertexShader
	{
		using namespace sdw;
		VertexWriter writer;

		Scene::SceneUbo sceneUbo(writer);
		Scene::ModelUbo modelUbo(writer);
		Scene::ModelPcb modelPcb(writer);

		auto inPosition = writer.declInput<Vec4>("fragPosition", 0);

		auto out = writer.getOut();

		auto materialVertexShaderBuildData =
		    material.material().instantiateVertexShaderBuildData();
		auto materialVertexFunction = material.material().vertexFunction(
		    writer, materialVertexShaderBuildData.get());

		writer.implementMain([&]() {
			auto model = modelUbo.getModel()[modelPcb.getModelId()];
			auto view = sceneUbo.getShadowView();
			auto proj = sceneUbo.getShadowProj();

			out.vtx.position = proj * view * model * inPosition;
		});

		std::vector<uint32_t> bytecode =
		    spirv::serialiseSpirv(writer.getShader());

		vk::ShaderModuleCreateInfo createInfo;
		createInfo.codeSize = bytecode.size() * sizeof(*bytecode.data());
		createInfo.pCode = bytecode.data();

		vertexModule = vk.create(createInfo);
		if (!vertexModule)
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

		writer.implementMain([&]() {

		});

		std::vector<uint32_t> bytecode =
		    spirv::serialiseSpirv(writer.getShader());

		vk::ShaderModuleCreateInfo createInfo;
		createInfo.codeSize = bytecode.size() * sizeof(*bytecode.data());
		createInfo.pCode = bytecode.data();

		fragmentModule = vk.create(createInfo);
		if (!fragmentModule)
		{
			std::cerr << "error: failed to create fragment shader module"
			          << std::endl;
			abort();
		}
	}
#pragma endregion

#pragma region pipeline layout
	VkPushConstantRange pcRange{};
	pcRange.size = sizeof(PcbStruct);
	pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	std::array descriptorSetLayouts{
		scene.get()->descriptorSetLayout(),
		material.material().shadingModel().m_descriptorSetLayout.get(),
		material.material().descriptorSetLayout(),
	};

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.setLayoutCount = uint32_t(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	// pipelineLayoutInfo.setLayoutCount = 0;
	// pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pcRange;
	// pipelineLayoutInfo.pushConstantRangeCount = 0;
	// pipelineLayoutInfo.pPushConstantRanges = nullptr;

	pipelineLayout = vk.create(pipelineLayoutInfo);
	if (!pipelineLayout)
	{
		std::cerr << "error: failed to create pipeline layout" << std::endl;
		abort();
	}
#pragma endregion

#pragma region pipeline
	vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertexModule;
	vertShaderStageInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragmentModule;
	fragShaderStageInfo.pName = "main";

	std::array shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

	VertexInputState vertexInputState = mesh.positionOnlyVertexInputState();

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	// inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	inputAssembly.primitiveRestartEnable = false;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	/// TODO get from render pass
	viewport.width = float(1280);
	viewport.height = float(720);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	/// TODO get from render pass
	scissor.extent.width = 1280;
	scissor.extent.height = 720;

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
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = false;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	/// TODO get from render pass
	vk::PipelineMultisampleStateCreateInfo multisampling;
	multisampling.sampleShadingEnable = false;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = false;
	multisampling.alphaToOneEnable = false;

	/// TODO get from render pass and material
	/*
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

	// colorBlendAttachment.blendEnable = true;
	// colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	// colorBlendAttachment.dstColorBlendFactor =
	//    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	// colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	// colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	// colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	// colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	/// TODO get from render pass and material
	VkPipelineColorBlendAttachmentState objectIDBlendAttachment = {};
	objectIDBlendAttachment.colorWriteMask =
	    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
	    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	objectIDBlendAttachment.blendEnable = false;
	objectIDBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	objectIDBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	objectIDBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	objectIDBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	objectIDBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	objectIDBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	// colorHDRBlendAttachment.blendEnable = true;
	// colorHDRBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	// colorHDRBlendAttachment.dstColorBlendFactor =
	//    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	// colorHDRBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	// colorHDRBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	// colorHDRBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	// colorHDRBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	std::array colorBlendAttachments{ colorBlendAttachment,
	                                  objectIDBlendAttachment };
	//*/

	/// TODO get from render pass
	vk::PipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.logicOpEnable = false;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	// colorBlending.attachmentCount = uint32_t(colorBlendAttachments.size());
	// colorBlending.pAttachments = colorBlendAttachments.data();
	colorBlending.attachmentCount = 0;
	colorBlending.pAttachments = nullptr;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	vk::PipelineDepthStencilStateCreateInfo depthStencil;
	depthStencil.depthTestEnable = true;
	depthStencil.depthWriteEnable = true;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = false;
	depthStencil.minDepthBounds = 0.0f;  // Optional
	depthStencil.maxDepthBounds = 1.0f;  // Optional
	depthStencil.stencilTestEnable = false;
	// depthStencil.front; // Optional
	// depthStencil.back; // Optional

	std::array dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	vk::PipelineDynamicStateCreateInfo dynamicState;
	dynamicState.dynamicStateCount = uint32_t(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	vk::GraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.stageCount = uint32_t(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputState.vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = nullptr;
	pipelineInfo.basePipelineIndex = -1;

	pipeline = vk.create(pipelineInfo);
	if (!pipeline)
	{
		std::cerr << "error: failed to create graphics pipeline" << std::endl;
		abort();
	}
#pragma endregion
}

void SceneObject::ShadowmapPipeline::bindPipeline(CommandBuffer& cb)
{
	cb.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void SceneObject::ShadowmapPipeline::bindDescriptorSet(CommandBuffer& cb)
{
	std::array<VkDescriptorSet, 3> descriptorSets{
		scene.get()->descriptorSet(),
		material.get()->material().shadingModel().m_descriptorSet,
		material.get()->material().descriptorSet(),
	};
	cb.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0,
	                      uint32_t(descriptorSets.size()),
	                      descriptorSets.data());
}

void SceneObject::ShadowmapPipeline::draw(CommandBuffer& cb)
{
	mesh.get()->drawPositions(cb);
}

SceneObject::SceneObject(Scene& s) : m_scene(&s) {}

void SceneObject::draw(CommandBuffer& cb, VkRenderPass renderPass,
                       std::optional<VkViewport> viewport,
                       std::optional<VkRect2D> scissor)
{
	if (m_scene && m_mesh && m_material)
	{
		Pipeline* pipeline;
		auto foundPipeline = m_pipelines.find(renderPass);
		if (foundPipeline == m_pipelines.end())
		{
			auto it = m_pipelines
			              .insert(std::make_pair(
			                  renderPass, Pipeline(*m_scene, *m_mesh,
			                                       *m_material, renderPass)))
			              .first;
			pipeline = &it->second;
		}
		else
		{
			pipeline = &foundPipeline->second;
		}

		pipeline->bindPipeline(cb);

		if (viewport.has_value())
			cb.setViewport(viewport.value());

		if (scissor.has_value())
			cb.setScissor(scissor.value());

		pipeline->bindDescriptorSet(cb);

		PcbStruct pcbStruct;
		pcbStruct.modelIndex = id;
		pcbStruct.materialInstanceIndex = m_material.get()->index();

		cb.pushConstants(
		    pipeline->pipelineLayout,
		    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
		    &pcbStruct);

		pipeline->draw(cb);
	}
}

void SceneObject::drawShadowmapPass(CommandBuffer& cb, VkRenderPass renderPass,
                                    std::optional<VkViewport> viewport,
                                    std::optional<VkRect2D> scissor)
{
	if (m_scene && m_mesh && m_material)
	{
		ShadowmapPipeline* pipeline;
		auto foundPipeline = m_shadowmapPipelines.find(renderPass);
		if (foundPipeline == m_shadowmapPipelines.end())
		{
			auto it = m_shadowmapPipelines
			              .insert(std::make_pair(
			                  renderPass,
			                  ShadowmapPipeline(*m_scene, *m_mesh, *m_material,
			                                    renderPass)))
			              .first;
			pipeline = &it->second;
		}
		else
		{
			pipeline = &foundPipeline->second;
		}

		pipeline->bindPipeline(cb);

		if (viewport.has_value())
			cb.setViewport(viewport.value());

		if (scissor.has_value())
			cb.setScissor(scissor.value());

		pipeline->bindDescriptorSet(cb);

		PcbStruct pcbStruct;
		pcbStruct.modelIndex = id;
		pcbStruct.materialInstanceIndex = m_material.get()->index();

		cb.pushConstants(pipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
		                 0, &pcbStruct);

		pipeline->draw(cb);
	}
}
}  // namespace cdm
