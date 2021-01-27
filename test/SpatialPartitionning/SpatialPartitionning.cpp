#include "SpatialPartitionning.hpp"
#include "RenderWindow.hpp"

#include "MyShaderWriter.hpp"
#include "TextureFactory.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "my_imgui_impl_vulkan.h"

//#define TINYOBJLOADER_IMPLEMENTATION
//#include <tiny_obj_loader.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace cdm
{
void OrbitCamera::Update()
{
	m_transform = unscaled_transform3d();
	m_transform.rotate(m_rotation);
	m_transform.translate_relative(
	    vector3{ 0, 0.0f, std::exp(m_distance) + 0.5f });
	m_transform.translate_absolute(m_center);
}

void OrbitCamera::Rotate(radian pitch, radian yaw)
{
	m_rotation = quaternion(m_rotation * vector3(1, 0, 0), pitch) *
	             quaternion(m_rotation * vector3(0, 1, 0), yaw) * m_rotation;
	Update();
}

void OrbitCamera::AddDistance(float offset)
{
	m_distance -= offset;
	Update();
}

void OrbitCamera::SetCenter(vector3 center)
{
	m_center = center;
	Update();
}

struct UniformBuffer
{
	matrix4 mvp;
	matrix4 modelMatrix;
	matrix4 normalMatrix;
	vector3 cameraPos;
};

struct TriangleI
{
	uint32_t a, b, c;
};

struct TriangleP
{
	vector3 a, b, c;

	Box computeBox() const
	{
		Box resa;
		resa.init = true;
		resa.min = a;
		resa.max = a;

		Box resb;
		resb.init = true;
		resb.min = b;
		resb.max = b;

		Box resc;
		resc.init = true;
		resc.min = c;
		resc.max = c;

		return box_union(box_union(resa, resb), resc);
	}

	aabb computeAABB() const
	{
		aabb resa;
		resa.min = a;
		resa.max = a;

		aabb resb;
		resb.min = b;
		resb.max = b;

		aabb resc;
		resc.min = c;
		resc.max = c;

		return resa + resb + resc;
	}
};

//std::unique_ptr<KDTree<int>> tree;
std::unique_ptr<Octree<int>> octree;

SpatialPartitionning::SpatialPartitionning(RenderWindow& rw)
    : RenderApplication(rw),
      log(vk),
      m_cbPool(vk, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
      gen(rd()),
      dis(0.0f, 0.3f)
{
	LogRRID log(vk);

#pragma region renderPass
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = renderWindow.depthImageFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout =
	    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
	depthAttachment.finalLayout =
	    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;

	std::array colorAttachments{ colorAttachment, depthAttachment };

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout =
	    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;

	std::array colorAttachmentRefs{

		colorAttachmentRef
	};

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = uint32_t(colorAttachmentRefs.size());
	subpass.pColorAttachments = colorAttachmentRefs.data();
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.pResolveAttachments = nullptr;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
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

	VertexInputHelper vertexHelper;
#pragma region vertexShader
	{
		using namespace sdw;

		cdm::VertexWriter writer;

		auto inPosition = writer.declInput<Vec4>("inPosition", 0);
		auto inNormal = writer.declInput<Vec3>("inNormal", 1);

		auto outPosition = writer.declOutput<Vec3>("outPosition", 0);
		auto outNormal = writer.declOutput<Vec3>("outNormal", 1);

		auto out = writer.getOut();

		auto ubo = writer.declUniformBuffer("ubo", 0, 0);
		ubo.declMember<Mat4>("mvp");
		ubo.declMember<Mat4>("modelMatrix");
		ubo.declMember<Mat4>("normalMatrix");
		ubo.declMember<Vec3>("cameraPos");
		ubo.end();

		writer.implementMain([&]() {
			out.vtx.position = ubo.getMember<Mat4>("mvp") * inPosition;
			outNormal = normalize(
			    (ubo.getMember<Mat4>("normalMatrix") * vec4(inNormal, 0.0_f))
			        .xyz());
			outPosition = normalize(
			    (ubo.getMember<Mat4>("modelMatrix") * inPosition).xyz());
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

	FragmentShaderHelperResult fragmentHelper;
#pragma region fragmentShader
	{
		using namespace sdw;
		cdm::FragmentWriter writer;

		auto in = writer.getIn();

		auto inPosition = writer.declInput<Vec3>("inPosition", 0u);
		auto inNormal = writer.declInput<Vec3>("inNormal", 1u);

		auto fragColor = writer.declOutput<Vec4>("fragColor", 0u);

		auto ubo = writer.declUniformBuffer("ubo", 0, 0);
		ubo.declMember<Mat4>("mvp");
		ubo.declMember<Mat4>("modelMatrix");
		ubo.declMember<Mat4>("normalMatrix");
		ubo.declMember<Vec3>("cameraPos");
		ubo.end();

		/*
		auto ACESFilm = writer.implementFunction<Vec3>(
		    "ACESFilm",
		    [&](const Vec3& x_arg) {
		        auto ACESMatrix = writer.declLocale<Mat3>(
		            "ACESMatrix", mat3(vec3(0.59719_f, 0.35458_f, 0.04823_f),
		                               vec3(0.07600_f, 0.90834_f, 0.01566_f),
		                               vec3(0.02840_f, 0.13383_f, 0.83777_f)));
		        ACESMatrix = transpose(ACESMatrix);
		        auto ACESMatrixInv = writer.declLocale<Mat3>(
		            "ACESMatrixInv", inverse(ACESMatrix));
		        auto x = writer.declLocale("x", x_arg);

		        // RGB to ACES conversion
		        x = ACESMatrix * x;

		        // ACES system tone scale (RTT+ODT)
		        auto a = writer.declLocale("a", 0.0245786_f);
		        auto b = writer.declLocale("b", -0.000090537_f);
		        auto c = writer.declLocale("c", 0.983729_f);
		        auto d = writer.declLocale("d", 0.4329510_f);
		        auto e = writer.declLocale("e", 0.238081_f);
		        x = (x * (x + a) + b) / (x * (x * c + d) + e);

		        // ACES to RGB conversion
		        x = ACESMatrixInv * x;

		        writer.returnStmt(x);
		    },
		    InVec3{ writer, "x_arg" });

		auto bloom = writer.implementFunction<Vec3>(
		    "bloom",
		    [&](const Float& scale, const Float& threshold,
		        const Vec2& fragCoord) {
		        auto logScale =
		            writer.declLocale("logScale", log2(scale) + 1.0_f);

		        auto bloomV3 = writer.declLocale("bloomV3", vec3(0.0_f));

		        FOR(writer, Int, y, -1_i, y <= 1_i, ++y)
		        {
		            FOR(writer, Int, x, -1_i, x <= 1_i, ++x)
		            {
		                bloomV3 +=
		                    kernelImage
		                        .lod(

		                            (fragCoord + vec2(x, y) * vec2(scale)) /
		                                vec2(writer.Width, writer.Height),
		                            logScale)
		                        .rgb();
		            }
		            ROF;
		        }
		        ROF;

		        writer.returnStmt(
		            max(bloomV3 / vec3(9.0_f) - vec3(threshold), vec3(0.0_f)));
		    },
		    InFloat{ writer, "scale" }, InFloat{ writer, "threshold" },
		    InVec2{ writer, "fragCoord" });
		//*/

		writer.implementMain([&]() {
			auto cameraPos = ubo.getMember<Vec3>("cameraPos");

			Locale(V, normalize(inPosition - cameraPos));
			Locale(N, normalize(inNormal));

			fragColor = vec4(abs(dot(V, N)));
			// fragColor = inColor;
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

#pragma region descriptor pool
	std::array poolSizes{
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
	};

	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.maxSets = 1;
	poolInfo.poolSizeCount = uint32_t(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();

	m_dPool = vk.create(poolInfo);
	if (!m_dPool)
	{
		std::cerr << "error: failed to create descriptor pool" << std::endl;
		abort();
	}
#pragma endregion

#pragma region descriptor set layout
	VkDescriptorSetLayoutBinding layoutBindingUbo{};
	layoutBindingUbo.binding = 0;
	layoutBindingUbo.descriptorCount = 1;
	layoutBindingUbo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBindingUbo.stageFlags =
	    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array layoutBindings{
		layoutBindingUbo,
	};

	vk::DescriptorSetLayoutCreateInfo setLayoutInfo;
	setLayoutInfo.bindingCount = uint32_t(layoutBindings.size());
	setLayoutInfo.pBindings = layoutBindings.data();

	m_dSetLayout = vk.create(setLayoutInfo);
	if (!m_dSetLayout)
	{
		std::cerr << "error: failed to create set layout" << std::endl;
		abort();
	}
#pragma endregion

#pragma region descriptor set
	vk::DescriptorSetAllocateInfo setAlloc;
	setAlloc.descriptorPool = m_dPool.get();
	setAlloc.descriptorSetCount = 1;
	setAlloc.pSetLayouts = &m_dSetLayout.get();

	vk.allocate(setAlloc, &m_dSet.get());

	if (!m_dSet)
	{
		std::cerr << "error: failed to allocate descriptor set" << std::endl;
		abort();
	}
#pragma endregion

#pragma region pipeline layout
	// VkPushConstantRange pcRange{};
	// pcRange.size = sizeof(float);
	// pcRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_dSetLayout.get();
	// computePipelineLayoutInfo.pushConstantRangeCount = 1;
	// computePipelineLayoutInfo.pPushConstantRanges = &pcRange;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	m_pipelineLayout = vk.create(pipelineLayoutInfo);
	if (!m_pipelineLayout)
	{
		std::cerr << "error: failed to create pipeline layout" << std::endl;
		abort();
	}
#pragma endregion

#pragma region mesh pipeline
	vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = m_vertexModule.get();
	vertShaderStageInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = m_fragmentModule.get();
	fragShaderStageInfo.pName = "main";

	std::array shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

	VkVertexInputBindingDescription binding = {};
	binding.binding = 0;
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	binding.stride = sizeof(Vertex);

	VkVertexInputAttributeDescription attribute0 = {};
	attribute0.binding = 0;
	attribute0.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attribute0.location = 0;
	attribute0.offset = 0;

	VkVertexInputAttributeDescription attribute1 = {};
	attribute1.binding = 0;
	attribute1.format = VK_FORMAT_R32G32B32_SFLOAT;
	attribute1.location = 1;
	attribute1.offset = sizeof(vector4);

	std::array attributes = { attribute0, attribute1 };

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &binding;
	vertexInputInfo.vertexAttributeDescriptionCount =
	    uint32_t(attributes.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = false;

	vk::Viewport viewport;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = float(renderWindow.width());
	viewport.height = float(renderWindow.height());
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor;
	scissor.offset = { 0, 0 };
	scissor.extent.width = uint32_t(renderWindow.width());
	scissor.extent.height = uint32_t(renderWindow.height());

	vk::PipelineViewportStateCreateInfo viewportState;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	vk::PipelineRasterizationStateCreateInfo rasterizer;
	rasterizer.depthClampEnable = false;
	rasterizer.rasterizerDiscardEnable = false;
	// rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = false;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	vk::PipelineDepthStencilStateCreateInfo depthStencil;
	depthStencil.depthTestEnable = true;
	depthStencil.depthWriteEnable = true;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencil.depthBoundsTestEnable = false;
	depthStencil.stencilTestEnable = false;
	depthStencil.front;
	depthStencil.back;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;

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

	// colorBlendAttachment.blendEnable = true;
	// colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	// colorBlendAttachment.dstColorBlendFactor =
	//    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	// colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	// colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	// colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	// colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendAttachmentState colorHDRBlendAttachment = {};
	colorHDRBlendAttachment.colorWriteMask =
	    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
	    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	// colorHDRBlendAttachment.blendEnable = false;
	// colorHDRBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	// colorHDRBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	// colorHDRBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	// colorHDRBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	// colorHDRBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	// colorHDRBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	colorHDRBlendAttachment.blendEnable = true;
	colorHDRBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorHDRBlendAttachment.dstColorBlendFactor =
	    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorHDRBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorHDRBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorHDRBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorHDRBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	std::array colorBlendAttachments{
		colorBlendAttachment,
		// colorHDRBlendAttachment
	};

	vk::PipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.logicOpEnable = VK_FALSE;
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
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = m_pipelineLayout.get();
	pipelineInfo.renderPass = m_renderPass.get();
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = nullptr;
	pipelineInfo.basePipelineIndex = -1;

	m_meshPipeline = vk.create(pipelineInfo);
	if (!m_meshPipeline)
	{
		std::cerr << "error: failed to create graphics mesh pipeline"
		          << std::endl;
		abort();
	}
#pragma endregion

	m_debugBox = DebugBox(vk, m_renderPass, viewport, scissor);

#pragma region bunny mesh
	Assimp::Importer importer;

	const aiScene* bunnyScene = importer.ReadFile(
	    "../resources/bunny.obj", aiProcess_Triangulate | aiProcess_FlipUVs |
	                                  aiProcess_GenNormals |
	                                  aiProcess_CalcTangentSpace);

	if (!bunnyScene || bunnyScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
	    !bunnyScene->mRootNode)
	{
		std::cerr << "assimp error: " << importer.GetErrorString()
		          << std::endl;
		throw std::runtime_error(std::string("assimp error: ") +
		                         importer.GetErrorString());
	}

	assert(bunnyScene->mNumMeshes == 1);

	{
		m_positions.clear();
		m_vertices.clear();
		m_indices.clear();

		auto& mesh = bunnyScene->mMeshes[0];
		m_vertices.reserve(mesh->mNumVertices);
		m_positions.reserve(mesh->mNumVertices);
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			vertex.position.x = mesh->mVertices[i].x;
			vertex.position.y = mesh->mVertices[i].y;
			vertex.position.z = mesh->mVertices[i].z;
			vertex.position.w = 1.0f;
			if (mesh->HasNormals())
			{
				vertex.normal.x = mesh->mNormals[i].x;
				vertex.normal.y = mesh->mNormals[i].y;
				vertex.normal.z = mesh->mNormals[i].z;
			}
			else
			{
				vertex.normal = { 1, 0, 0 };
			}

			m_vertices.push_back(vertex);
			m_positions.push_back(vertex.position.xyz());
		}

		m_indices.reserve(mesh->mNumFaces * 3ull);
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			const aiFace& face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				m_indices.push_back(face.mIndices[j]);
		}

		m_indicesCount = uint32_t(m_indices.size());

		auto& frame = m_cbPool.getAvailableCommandBuffer();
		auto& copyCB = frame.commandBuffer;

#pragma region vertexBuffer
		auto vertexStagingBuffer = Buffer(
		    vk, sizeof(Vertex) * m_vertices.size(),
		    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
		    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

		m_vertexBuffer = Buffer(vk, sizeof(Vertex) * m_vertices.size(),
		                        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		                            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		                        VMA_MEMORY_USAGE_GPU_ONLY,
		                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		Vertex* vertexData = vertexStagingBuffer.map<Vertex>();
		std::copy(m_vertices.begin(), m_vertices.end(), vertexData);
		vertexStagingBuffer.unmap();

		copyCB.reset();
		copyCB.begin();
		copyCB.copyBuffer(vertexStagingBuffer.get(), m_vertexBuffer.get(),
		                  sizeof(Vertex) * m_vertices.size());
		// copyCB.end();

		// if (frame.submit(vk.graphicsQueue()) != VK_SUCCESS)
		//	throw std::runtime_error("could not copy vertex or index buffer");

		// m_cbPool.waitForAllCommandBuffers();
#pragma endregion

#pragma region indexBuffer
		auto indexStagingBuffer = Buffer(
		    vk, sizeof(uint32_t) * m_indices.size(),
		    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
		    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

		m_indexBuffer = Buffer(vk, sizeof(uint32_t) * m_indices.size(),
		                       VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		                           VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		                       VMA_MEMORY_USAGE_GPU_ONLY,
		                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		uint32_t* indexData = indexStagingBuffer.map<uint32_t>();
		std::copy(m_indices.begin(), m_indices.end(), indexData);
		// for (size_t i = 0; i < m_indices.size(); i++)

		indexStagingBuffer.unmap();

		// copyCB.reset();
		// copyCB.begin();
		copyCB.copyBuffer(indexStagingBuffer.get(), m_indexBuffer.get(),
		                  sizeof(uint32_t) * m_indices.size());
		copyCB.end();

		if (frame.submit(vk.graphicsQueue()) != VK_SUCCESS)
			throw std::runtime_error("could not copy vertex or index buffer");

		m_cbPool.waitForAllCommandBuffers();
#pragma endregion
	}

	m_box.min = m_positions[0];
	m_box.max = m_positions[0];
	m_box.init = true;
	for (auto p : m_positions)
	{
		m_box.min.x = std::min(m_box.min.x, p.x);
		m_box.min.y = std::min(m_box.min.y, p.y);
		m_box.min.z = std::min(m_box.min.z, p.z);

		m_box.max.x = std::max(m_box.max.x, p.x);
		m_box.max.y = std::max(m_box.max.y, p.y);
		m_box.max.z = std::max(m_box.max.z, p.z);
	}

	//tree = std::make_unique<KDTree<int>>(m_box);
	//tree->nodes.reserve(m_indices.size() / 3 + 3024*80);

	aabb AABB;
	AABB.min = m_box.min;
	AABB.max = m_box.max;

	octree = std::make_unique<Octree<int>>(AABB);

	{
		auto* tris_begin = reinterpret_cast<TriangleI*>(m_indices.data());
		auto* tris_end = tris_begin + (m_indices.size() / 3);

		for (auto* tris_it = tris_begin; tris_it != tris_end; ++tris_it)
		// for (size_t i = 0; i < m_indices.size(); i += 3)
		{
			// TriangleI ti{ m_indices[i], m_indices[i + 1], m_indices[i + 2]
			// };
			TriangleI& ti = *tris_it;
			TriangleP t;
			t.a = m_positions[ti.a];
			t.b = m_positions[ti.b];
			t.c = m_positions[ti.c];

			//const Box b = t.computeBox();
			int triangleIndex = int(std::distance(tris_begin, tris_it));

			//tree->insert(b, triangleIndex);
			octree->insert(t.computeAABB(), triangleIndex);
			// i);
		}

		octree->spread_n(6);

		//size_t old_d = 0;
		//size_t d = octree->nodes.size();

		//for (size_t j = 0; j < 6; j++)
		//{
		//	for (size_t i = old_d; i < d; i++)
		//		octree->spread(i);
		//	old_d = d;
		//	d = octree->nodes.size();
		//}

		//for (size_t i = old_d; i < d; i++)
		//	octree->spread(i);


		//octree->spread(1);
		//octree->spread(8);

		// std::cout << "distance: " << int(std::distance(tris_begin,
		// tris_end)) << "\n";
		// std::cout << "indices:   " << m_indices.size() << "\n";
		//std::cout << "indices/3: " << m_indices.size() / 3 << "\n";
		// std::cout << "nodes:     " << tree->nodes.size() << "\n";

		//size_t total = 0;
		//size_t octotal = 0;

		// std::vector<int> data;

		//tree->traverse([&](const KDTree<int>::Node& n) {
		//	total = std::max(total, n.leafData.size());
		//	// for (const auto& pair : n.leafData)
		//	// data.push_back(pair.second);
		//});

		//std::cout << "tree max:  " << total << "\n";

		//octree->traverse([&](const Octree<int>::Node& n) {
		//	octotal = std::max(octotal, n.leafData.size());
		//	// for (const auto& pair : n.leafData)
		//	// data.push_back(pair.second);
		//});

		//std::cout << "octree max:" << octotal << "\n";

		// std::sort(data.begin(), data.end());

		// for (int i : data)
		//	std::cout << i << "\n";

		//tree->nodes.shrink_to_fit();
	}
#pragma endregion

	/*
#pragma region bunny mesh
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn;
	std::string err;
	bool bunnyFound = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn,
	                                   &err, "../resources/bunny.obj");

	std::cout << warn << std::endl;

	if (bunnyFound == false)
	{
	    std::cerr << err << std::endl;
	    throw std::runtime_error(err);
	}

	// Assimp::Importer importer;

	// const aiScene* bunnyScene = importer.ReadFile(
	//    "../resources/illumination_assets/bunny.obj",
	//    aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals |
	//        aiProcess_CalcTangentSpace);
	//
	// if (!bunnyScene || bunnyScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
	//    !bunnyScene->mRootNode)
	//{
	//	std::cerr << "assimp error: " << importer.GetErrorString()
	//	          << std::endl;
	//	throw std::runtime_error(std::string("assimp error: ") +
	//	                         importer.GetErrorString());
	//}
	//
	// assert(bunnyScene->mNumMeshes == 1);

	{
	    m_positions.clear();
	    m_vertices.clear();
	    m_indices.clear();

	    // auto& mesh = bunnyScene->mMeshes[0];
	    m_positions.reserve(attrib.vertices.size() / 3);
	    m_vertices.reserve(attrib.vertices.size() / 3);
	    for (size_t i = 0; i < attrib.vertices.size() / 3; i += 3)
	    {
	        Vertex vertex;
	        vertex.position.x = attrib.vertices[i + 0];
	        vertex.position.y = attrib.vertices[i + 1];
	        vertex.position.z = attrib.vertices[i + 2];

	        vertex.normal.x = attrib.normals[i + 0];
	        vertex.normal.y = attrib.normals[i + 1];
	        vertex.normal.z = attrib.normals[i + 2];

	        m_positions.push_back(vertex.position.xyz());
	        m_vertices.push_back(vertex);
	    }

	    m_indicesCount = uint32_t(shapes[0].mesh.indices.size());

	    m_indices.resize(m_indicesCount);
	    for (size_t i = 0; i < m_indices.size(); i++)
	    {
	        m_indices[i] = shapes[0].mesh.indices[i].vertex_index;
	    }
	    //m_indicesCount = uint32_t(m_indices.size());

	    auto& frame = m_cbPool.getAvailableCommandBuffer();
	    auto& copyCB = frame.commandBuffer;

#pragma region vertexBuffer
	    auto vertexStagingBuffer = Buffer(
	        vk, sizeof(Vertex) * m_vertices.size(),
	        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
	        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	    m_vertexBuffer = Buffer(vk, sizeof(Vertex) * m_vertices.size(),
	                            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
	                                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                            VMA_MEMORY_USAGE_GPU_ONLY,
	                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	    Vertex* vertexData = vertexStagingBuffer.map<Vertex>();
	    std::copy(m_vertices.begin(), m_vertices.end(), vertexData);
	    vertexStagingBuffer.unmap();

	    copyCB.reset();
	    copyCB.begin();
	    copyCB.copyBuffer(vertexStagingBuffer.get(), m_vertexBuffer.get(),
	                      sizeof(Vertex) * m_vertices.size());
	    //copyCB.end();

	    //if (frame.submit(vk.graphicsQueue()) != VK_SUCCESS)
	    //	throw std::runtime_error("could not copy vertex or index buffer");

	    //m_cbPool.waitForAllCommandBuffers();
#pragma endregion

#pragma region indexBuffer
	    auto indexStagingBuffer = Buffer(
	        vk, sizeof(uint32_t) * m_indices.size(),
	        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
	        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	    m_indexBuffer = Buffer(vk, sizeof(uint32_t) * m_indices.size(),
	                            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
	                                VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	                            VMA_MEMORY_USAGE_GPU_ONLY,
	                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	    uint32_t* indexData = indexStagingBuffer.map<uint32_t>();
	    std::copy(m_indices.begin(), m_indices.end(), indexData);
	    //for (size_t i = 0; i < m_indices.size(); i++)

	    indexStagingBuffer.unmap();

	    //copyCB.reset();
	    //copyCB.begin();
	    copyCB.copyBuffer(indexStagingBuffer.get(), m_indexBuffer.get(),
	                      sizeof(uint32_t) * m_indices.size());
	    copyCB.end();

	    if (frame.submit(vk.graphicsQueue()) != VK_SUCCESS)
	        throw std::runtime_error("could not copy vertex or index buffer");

	    m_cbPool.waitForAllCommandBuffers();
#pragma endregion
	}
#pragma endregion
	//*/

#pragma region UBO
	m_uniformBuffer = Buffer(
	    vk, sizeof(UniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	    VMA_MEMORY_USAGE_CPU_TO_GPU, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	m_camera.SetCenter((m_box.min + m_box.max) / 2.0f);
	m_camera.m_perspective.set_angle(120.0_deg);
	m_camera.m_perspective.set_ratio(float(renderWindow.width()) /
	                                 float(renderWindow.height()));

	UniformBuffer* ptr = m_uniformBuffer.map<UniformBuffer>();
	ptr->modelMatrix = matrix4::identity();
	// ptr->modelMatrix.m33 = 1;
	ptr->normalMatrix = matrix4::identity();
	ptr->mvp = m_camera.viewproj() * ptr->modelMatrix;
	ptr->mvp.transpose();
	ptr->modelMatrix.transpose();
	ptr->cameraPos = m_camera.m_transform.position;
	m_uniformBuffer.unmap();

	VkDescriptorBufferInfo setBufferInfo{};
	setBufferInfo.buffer = m_uniformBuffer.get();
	setBufferInfo.range = sizeof(UniformBuffer);
	setBufferInfo.offset = 0;

	vk::WriteDescriptorSet uboWrite;
	uboWrite.descriptorCount = 1;
	uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboWrite.dstArrayElement = 0;
	uboWrite.dstBinding = 0;
	uboWrite.dstSet = m_dSet.get();
	uboWrite.pBufferInfo = &setBufferInfo;

	vk.updateDescriptorSets(uboWrite);
#pragma endregion

	TextureFactory f(vk);

#pragma region outputImage
	f.setWidth(uint32_t(renderWindow.width()));
	f.setHeight(uint32_t(renderWindow.height()));
	f.setFormat(VK_FORMAT_R8G8B8A8_UNORM);
	f.setUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
	           VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	// f.setMipLevels(-1);

	m_outputImage = f.createTexture2D();

	vk.debugMarkerSetObjectName(m_outputImage.image(), "outputImage");

	// m_outputImage = Texture2D(
	//    rw, width, height, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
	//    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	//        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	//    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_outputImage.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
#pragma endregion

#pragma region depthTexture
	m_depthTexture = DepthTexture(renderWindow);

	vk.debugMarkerSetObjectName(m_depthTexture.image(), "depthTexture");

	m_depthTexture.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_UNDEFINED,
	    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL);
#pragma endregion

#pragma region framebuffer
	vk::FramebufferCreateInfo framebufferInfo;
	framebufferInfo.renderPass = m_renderPass.get();
	std::array attachments = { m_outputImage.view(), m_depthTexture.view() };
	framebufferInfo.attachmentCount = uint32_t(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = uint32_t(renderWindow.width());
	framebufferInfo.height = uint32_t(renderWindow.height());
	framebufferInfo.layers = 1;

	m_framebuffer = vk.create(framebufferInfo);
	if (!m_framebuffer)
	{
		std::cerr << "error: failed to create framebuffer" << std::endl;
		abort();
	}
#pragma endregion

#pragma region imguiFramebuffer
	framebufferInfo.renderPass = renderWindow.imguiRenderPass();
	std::array imguiAttachments = { m_outputImage.view() };
	framebufferInfo.attachmentCount = uint32_t(imguiAttachments.size());
	framebufferInfo.pAttachments = imguiAttachments.data();
	framebufferInfo.width = uint32_t(renderWindow.width());
	framebufferInfo.height = uint32_t(renderWindow.height());
	framebufferInfo.layers = 1;

	m_imguiFramebuffer = vk.create(framebufferInfo);
	if (!m_imguiFramebuffer)
	{
		std::cerr << "error: failed to create imgui Framebuffer" << std::endl;
		abort();
	}
#pragma endregion
}

SpatialPartitionning::~SpatialPartitionning() = default;

void SpatialPartitionning::render(CommandBuffer& cb)
{
	// extent = renderWindow.swapchainExtent();
	VkClearValue d;
	d.depthStencil.depth = 1.0f;
	std::array clearValues = { VkClearValue{}, d };

	vk::RenderPassBeginInfo rpInfo;
	rpInfo.framebuffer = m_framebuffer.get();
	rpInfo.renderPass = m_renderPass.get();
	rpInfo.renderArea.extent = renderWindow.swapchainExtent();
	// rpInfo.renderArea.extent.width = uint32_t(renderWindow.width());
	// rpInfo.renderArea.extent.height = uint32_t(renderWindow.height());

	if (rpInfo.renderArea.extent.width == 0 ||
	    rpInfo.renderArea.extent.height == 0)
		return;

	rpInfo.clearValueCount = 2;
	rpInfo.pClearValues = clearValues.data();

	vk::SubpassBeginInfo subpassBeginInfo;
	subpassBeginInfo.contents = VK_SUBPASS_CONTENTS_INLINE;

	vk::SubpassEndInfo subpassEndInfo;

	cb.beginRenderPass2(rpInfo, subpassBeginInfo);

	cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS,
	                     m_pipelineLayout.get(), 0, m_dSet.get());

	cb.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_meshPipeline.get());

	vk::Rect2D sc;
	sc.extent = renderWindow.swapchainExtent();
	// sc.extent.width = uint32_t(renderWindow.width());
	// sc.extent.height = uint32_t(renderWindow.height());

	cb.setScissor(sc);

	vk::Viewport vp;
	vp.width = sc.extent.width;
	vp.height = sc.extent.height;
	vp.minDepth = 0.0f;
	vp.maxDepth = 1.0f;

	cb.setViewport(vp);

	cb.bindIndexBuffer(m_indexBuffer.get());
	cb.bindVertexBuffer(m_vertexBuffer.get());
	// cb.draw(uint32_t(m_vertexBuffer.size()));
	// cb.drawIndexed((m_indicesCount / 3) / 10);
	cb.drawIndexed(m_indicesCount);

	static std::mt19937 gen;
	static std::uniform_real_distribution<float> dis(0.0f, 1.0f);
	gen.seed(0);
	dis.reset();

	matrix4 proj = m_camera.proj();
	proj.inverse();

	double xd, yd;
	getMousePos(xd, yd);

	float x = float(xd), y = float(yd);

	x = x / (float(renderWindow.width()) / 2.0f) - 1.0f;
	y = y / (float(renderWindow.height()) / 2.0f) - 1.0f;

	cdm::vector4 ray_origin{ x, y, 0.0f, 1.0f };
	cdm::vector4 ray_direction{ x, y, 1.0f, 1.0f };

	cdm::matrix4 m = m_camera.viewproj();
	m.inverse();

	ray_origin = m * ray_origin;
	ray_origin /= ray_origin.w;

	ray_direction = m * ray_direction;
	ray_direction /= ray_direction.w;

	ray_direction = ray_direction - ray_origin;

	cdm::vector3 dir = ray_direction.xyz();
	dir.normalize();

	cdm::ray3d r;
	r.origin = ray_origin.xyz();
	r.direction = dir;

	//*
	octree->query(
	    r,
	    //[&](const Octree<int>::Node& n) {
		   // // return true;
		   // //return collides(n.box, r);
		   // if (collides(n.box, r))
		   // {
			  //  m_debugBox->draw(cb, n.box.min, n.box.max,
			  //                   vector3(0, 0.0, 0.6), m_camera);
			  //  return true;
		   // }
		   // else
		   // {
			  //  //m_debugBox->draw(cb, n.box.min, n.box.max, vector3(1, 0, 0),
			  //  //                 m_camera);
			  //   //return true;
			  //  return false;
		   // }
	    //},
	    [&](const Octree<int>::DataPair& pair) {
		    //if (collides(pair.first, r))
			    m_debugBox->draw(cb, pair.first.min, pair.first.max,
			                     vector3(0, 0.9, 0), m_camera);
		    //else
			   // m_debugBox->draw(cb, pair.first.min, pair.first.max,
			   //                  vector3(0.5, 0.4, 0.1), m_camera);
	    });
	//*/

	/*aabb b;
	b.min = m_box.min;
	b.max = m_box.max;

	aabb b000, b100, b010, b001, b110, b101, b011, b111;

	split(b, b000, b100, b010, b001, b110, b101, b011, b111);

	for (const auto& box : { b000, b100, b010, b001, b110, b101, b011, b111 })
	{
		m_debugBox->draw(cb, box.min, box.max,
		                 vector3(dis(gen), dis(gen), dis(gen)), m_camera);
	}*/

	// tree->traverse([&](const KDTree<int>::Node& n) {
	//	// if (intersection(n.box, r))
	//	for (auto& p : n.leafData)
	//		// if (intersection(p.first, r))
	//		m_debugBox->draw(
	//		    cb, p.first.min, p.first.max,
	//		    // vector3(dis(gen), dis(gen), dis(gen)), m_camera);
	//		    vector3(0.6, 0, 0), m_camera);
	//});

	//{
	//	auto* tris_begin = reinterpret_cast<TriangleI*>(m_indices.data());
	//	auto* tris_end = tris_begin + (m_indices.size() / 3);
	//
	//	for (auto* tris_it = tris_begin; tris_it != tris_end; ++tris_it)
	//	{
	//		TriangleP t;
	//		t.a = m_positions[tris_it->a];
	//		t.b = m_positions[tris_it->b];
	//		t.c = m_positions[tris_it->c];
	//
	//		Box b = t.computeBox();
	//
	//		m_debugBox->draw(cb, b.min, b.max,
	//		                 vector3(0.1, 0.1, 0.6), m_camera);
	//	}
	//}

	cb.endRenderPass2(subpassEndInfo);

	/*
	{
	    unscaled_transform3d tr;
	    tr.position = { 2, 3, 4 };
	    tr.rotation = { 5, 6, 7, 8 };
	    tr.rotation.normalize();

	    auto print_mat = [](const matrix4& m)
	    {
	        std::cout
	            << m.m00 << "          \t" << m.m10 << "          \t" << m.m20
	<< "          \t" << m.m30 << "\n"
	            << m.m01 << "          \t" << m.m11 << "          \t" << m.m21
	<< "          \t" << m.m31 << "\n"
	            << m.m02 << "          \t" << m.m12 << "          \t" << m.m22
	<< "          \t" << m.m32 << "\n"
	            << m.m03 << "          \t" << m.m13 << "          \t" << m.m23
	<< "          \t" << m.m33 << "\n";
	    };

	    {
	        auto resmi = matrix4(tr).get_inversed();
	        auto resim = matrix4(tr.get_inversed());

	        print_mat(resmi);
	        std::cout << std::endl;
	        print_mat(resim);
	        std::cout << std::endl;
	    }
	}
	//*/
}

void SpatialPartitionning::imgui(CommandBuffer& cb)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	{
		// static float f = 0.0f;
		// static int counter = 0;

		ImGui::Begin("Controls");

		// bool changed = false;

		// changed |= ImGui::SliderFloat("CamFocalDistance",
		//                              &m_config.camFocalDistance,
		//                              0.1f, 30.0f);
		// changed |= ImGui::SliderFloat("CamFocalLength",
		//                              &m_config.camFocalLength, 0.0f, 20.0f);
		// changed |= ImGui::SliderFloat("CamAperture", &m_config.camAperture,
		//                              0.0f, 5.0f);

		// changed |= ImGui::DragFloat3("rotation", &m_config.camRot.x, 0.01f);
		// changed |= ImGui::DragFloat3("lightDir", &m_config.lightDir.x,
		// 0.01f);

		// changed |= ImGui::SliderFloat("scene radius", &m_config.sceneRadius,
		//                              0.0f, 10.0f);

		// ImGui::SliderFloat("BloomAscale1", &m_config.bloomAscale1,
		// 0.0f, 1.0f); ImGui::SliderFloat("BloomAscale2",
		// &m_config.bloomAscale2, 0.0f, 1.0f);
		// ImGui::SliderFloat("BloomBscale1", &m_config.bloomBscale1,
		// 0.0f, 1.0f); ImGui::SliderFloat("BloomBscale2",
		// &m_config.bloomBscale2, 0.0f, 1.0f);

		// changed |= ImGui::SliderFloat("Power", &m_config.power,
		// 0.0f, 50.0f); changed |=
		//    ImGui::DragInt("Iterations", &m_config.iterations, 0.1f, 1, 64);

		// if (changed)
		//	applyImguiParameters();

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
		            1000.0f / ImGui::GetIO().Framerate,
		            ImGui::GetIO().Framerate);
		ImGui::End();
	}

	ImGui::Render();

	// vk::ImageMemoryBarrier barrier;
	// barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	// barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	// barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	// barrier.image = outputImage();
	// barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	// barrier.subresourceRange.baseMipLevel = 0;
	// barrier.subresourceRange.levelCount = 1;
	// barrier.subresourceRange.baseArrayLayer = 0;
	// barrier.subresourceRange.layerCount = 1;
	// barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	// barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	// cb.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
	//                   VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);

	{
		std::array clearValues = { VkClearValue{} };

		vk::RenderPassBeginInfo rpInfo;
		rpInfo.framebuffer = m_imguiFramebuffer.get();
		rpInfo.renderPass = renderWindow.imguiRenderPass();

		rpInfo.renderArea.extent = renderWindow.swapchainExtent();
		// rpInfo.renderArea.extent.width = uint32_t(renderWindow.width());
		// rpInfo.renderArea.extent.height = uint32_t(renderWindow.height());

		if (rpInfo.renderArea.extent.width == 0 ||
		    rpInfo.renderArea.extent.height == 0)
			return;

		// rpInfo.renderArea.extent.width = uint32_t(renderWindow.width());
		// rpInfo.renderArea.extent.height = uint32_t(renderWindow.height());
		rpInfo.clearValueCount = 1;
		rpInfo.pClearValues = clearValues.data();

		vk::SubpassBeginInfo subpassBeginInfo;
		subpassBeginInfo.contents = VK_SUBPASS_CONTENTS_INLINE;

		cb.beginRenderPass2(rpInfo, subpassBeginInfo);
	}

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb.get());

	vk::SubpassEndInfo subpassEndInfo2;
	cb.endRenderPass2(subpassEndInfo2);

	// barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	// barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	// barrier.dstAccessMask =
	//    VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
	// cb.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
	//                   VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);
}

void SpatialPartitionning::onMouseWheel(double xoffset, double yoffset)
{
	vk.wait();

	m_camera.AddDistance(yoffset * 0.125);

	UniformBuffer* ptr = m_uniformBuffer.map<UniformBuffer>();
	ptr->modelMatrix = matrix4::identity();
	// ptr->modelMatrix.m33 = 1;
	ptr->normalMatrix = matrix4::identity();
	ptr->mvp = m_camera.viewproj() * ptr->modelMatrix;
	ptr->mvp.transpose();
	ptr->modelMatrix.transpose();
	ptr->cameraPos = m_camera.m_transform.position;
	m_uniformBuffer.unmap();
}

void SpatialPartitionning::resize(uint32_t width, uint32_t height)
{
	vk.wait();

	// if (renderWindow.resized())
	//{
	m_camera.m_perspective.set_angle(120.0_deg);
	m_camera.m_perspective.set_ratio(float(width) / float(height));

	UniformBuffer* ptr = m_uniformBuffer.map<UniformBuffer>();
	ptr->modelMatrix = matrix4::identity();
	// ptr->modelMatrix.m33 = 1;
	ptr->normalMatrix = matrix4::identity();
	ptr->mvp = m_camera.viewproj() * ptr->modelMatrix;
	ptr->mvp.transpose();
	ptr->modelMatrix.transpose();
	ptr->cameraPos = m_camera.m_transform.position;
	m_uniformBuffer.unmap();

	m_swapchainRecreated = false;
	TextureFactory f(vk);

#pragma region outputImage
	f.setWidth(width);
	f.setHeight(height);
	f.setFormat(VK_FORMAT_R8G8B8A8_UNORM);
	f.setUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
	           VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

	m_outputImage = f.createTexture2D();

	vk.debugMarkerSetObjectName(m_outputImage.image(), "outputImage");

	m_outputImage.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
#pragma endregion

#pragma region depthTexture
	m_depthTexture = DepthTexture(renderWindow);

	vk.debugMarkerSetObjectName(m_depthTexture.image(), "depthTexture");

	m_depthTexture.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_UNDEFINED,
	    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL);
#pragma endregion

#pragma region framebuffer
	vk::FramebufferCreateInfo framebufferInfo;
	framebufferInfo.renderPass = m_renderPass.get();
	std::array attachments = { m_outputImage.view(), m_depthTexture.view() };
	framebufferInfo.attachmentCount = uint32_t(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = width;
	framebufferInfo.height = height;
	framebufferInfo.layers = 1;

	m_framebuffer = vk.create(framebufferInfo);
	if (!m_framebuffer)
	{
		std::cerr << "error: failed to create framebuffer" << std::endl;
		abort();
	}
#pragma endregion

#pragma region imguiFramebuffer
	framebufferInfo.renderPass = renderWindow.imguiRenderPass();
	std::array imguiAttachments = { m_outputImage.view() };
	framebufferInfo.attachmentCount = uint32_t(imguiAttachments.size());
	framebufferInfo.pAttachments = imguiAttachments.data();
	framebufferInfo.width = width;
	framebufferInfo.height = height;
	framebufferInfo.layers = 1;

	m_imguiFramebuffer = vk.create(framebufferInfo);
	if (!m_imguiFramebuffer)
	{
		std::cerr << "error: failed to create imgui Framebuffer" << std::endl;
		abort();
	}
#pragma endregion
	//}
}

void SpatialPartitionning::update()
{
	if (getMouseButtonState(MouseButton::Left) == ButtonState::Pressing)
	{
		double dx, dy;
		getMouseDelta(dx, dy);

		vk.wait();

		m_camera.Rotate(degree(float(dy / 5.0)), degree(float(dx / 5.0)));

		UniformBuffer* ptr = m_uniformBuffer.map<UniformBuffer>();
		ptr->modelMatrix = matrix4::identity();
		// ptr->modelMatrix.m33 = 1;
		ptr->normalMatrix = matrix4::identity();
		ptr->mvp = m_camera.viewproj() * ptr->modelMatrix;
		ptr->mvp.transpose();
		ptr->modelMatrix.transpose();
		ptr->cameraPos = m_camera.m_transform.position;
		m_uniformBuffer.unmap();
	}
}

void SpatialPartitionning::draw()
{
	m_cbPool.waitForAllCommandBuffers();
	auto& frame = m_cbPool.getAvailableCommandBuffer();
	auto& cb = frame.commandBuffer;

	cb.reset();
	cb.begin();
	cb.debugMarkerBegin("render", 0.2f, 0.2f, 1.0f);
	render(cb);
	imgui(cb);
	cb.debugMarkerEnd();
	cb.end();

	if (frame.submit(vk.graphicsQueue()) != VK_SUCCESS)
	{
		std::cerr << "error: failed to submit imgui command buffer"
		          << std::endl;
		abort();
	}
	vk.wait(vk.graphicsQueue());

	auto swapImages = renderWindow.swapchainImages();

	renderWindow.present(m_outputImage,
	                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, nullptr,
	                     m_swapchainRecreated);

	/*if (m_swapchainRecreated && false)
	{
	    m_swapchainRecreated = false;
	    TextureFactory f(vk);

#pragma region outputImage
	    f.setWidth(uint32_t(renderWindow.width()));
	    f.setHeight(uint32_t(renderWindow.height()));
	    f.setFormat(VK_FORMAT_R8G8B8A8_UNORM);
	    f.setUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
	               VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

	    m_outputImage = f.createTexture2D();

	    vk.debugMarkerSetObjectName(m_outputImage.image(), "outputImage");

	    m_outputImage.transitionLayoutImmediate(
	        VK_IMAGE_LAYOUT_UNDEFINED,
	        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
#pragma endregion

#pragma region depthTexture
	    m_depthTexture = DepthTexture(renderWindow);

	    vk.debugMarkerSetObjectName(m_depthTexture.image(), "depthTexture");

	    m_depthTexture.transitionLayoutImmediate(
	        VK_IMAGE_LAYOUT_UNDEFINED,
	        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL);
#pragma endregion

#pragma region framebuffer
	    vk::FramebufferCreateInfo framebufferInfo;
	    framebufferInfo.renderPass = m_renderPass.get();
	    std::array attachments = { m_outputImage.view(),
	                               m_depthTexture.view() };
	    framebufferInfo.attachmentCount = uint32_t(attachments.size());
	    framebufferInfo.pAttachments = attachments.data();
	    framebufferInfo.width = uint32_t(renderWindow.width());
	    framebufferInfo.height = uint32_t(renderWindow.height());
	    framebufferInfo.layers = 1;

	    m_framebuffer = vk.create(framebufferInfo);
	    if (!m_framebuffer)
	    {
	        std::cerr << "error: failed to create framebuffer" << std::endl;
	        abort();
	    }
#pragma endregion

#pragma region imguiFramebuffer
	    framebufferInfo.renderPass = renderWindow.imguiRenderPass();
	    std::array imguiAttachments = { m_outputImage.view() };
	    framebufferInfo.attachmentCount = uint32_t(imguiAttachments.size());
	    framebufferInfo.pAttachments = imguiAttachments.data();
	    framebufferInfo.width = uint32_t(renderWindow.width());
	    framebufferInfo.height = uint32_t(renderWindow.height());
	    framebufferInfo.layers = 1;

	    m_imguiFramebuffer = vk.create(framebufferInfo);
	    if (!m_imguiFramebuffer)
	    {
	        std::cerr << "error: failed to create imgui Framebuffer"
	                  << std::endl;
	        abort();
	    }
#pragma endregion
	}*/
}
}  // namespace cdm

int main()
{
	using namespace cdm;

	RenderWindow rw(1280, 720, true);
	SpatialPartitionning SpatialPartitionning(rw);

	return SpatialPartitionning.run();
}
