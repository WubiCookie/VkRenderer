#include "ShaderBall.hpp"

#include "CompilerSpirV/compileSpirV.hpp"
#include "ShaderWriter/Intrinsics/Intrinsics.hpp"
#include "ShaderWriter/Source.hpp"

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <array>
#include <iostream>
#include <stdexcept>

constexpr size_t POINT_COUNT = 2000;

#define mandelbulbTexScale 1

constexpr uint32_t width = 1280 * mandelbulbTexScale;
constexpr uint32_t height = 720 * mandelbulbTexScale;
constexpr float widthf = 1280.0f * mandelbulbTexScale;
constexpr float heightf = 720.0f * mandelbulbTexScale;

namespace cdm
{
static constexpr float Pi{ 3.14159265359f };

void ShaderBall::Config::copyTo(void* ptr)
{
	std::memcpy(ptr, this, sizeof(*this));
}

template <typename T>
class ShaderBallShaderLib : public T
{
public:
	sdw::Ubo ubo;

	ShaderBallShaderLib(ShaderBall::Config const& config)
	    : T(),
	      ubo(sdw::Ubo(*this, "ubo", 0, 0))
	{
		using namespace sdw;

		(void)ubo.declMember<Mat4>("model");
		(void)ubo.declMember<Mat4>("view");
		(void)ubo.declMember<Mat4>("proj");
		ubo.end();
	}
};

void Mesh::init()
{
	auto& vk = rw.get()->device();

	CommandBuffer copyCB(vk, rw.get()->oneTimeCommandPool());

#pragma region vertexBuffer
	auto vertexStagingBuffer =
	    Buffer(*rw.get(), sizeof(Vertex) * vertices.size(),
	           VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
	           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	vertexBuffer = Buffer(
	    *rw.get(), sizeof(Vertex) * vertices.size(),
	    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	Vertex* vertexData = vertexStagingBuffer.map<Vertex>();
	std::copy(vertices.begin(), vertices.end(), vertexData);
	vertexStagingBuffer.unmap();

	copyCB.reset();
	copyCB.begin();
	copyCB.copyBuffer(vertexStagingBuffer.get(), vertexBuffer.get(),
	                  sizeof(Vertex) * vertices.size());
	copyCB.end();

	if (vk.queueSubmit(vk.graphicsQueue(), copyCB.get()) != VK_SUCCESS)
		throw std::runtime_error("could not copy vertex buffer");

#pragma endregion

#pragma region indexBuffer
	auto indexStagingBuffer =
	    Buffer(*rw.get(), sizeof(uint32_t) * indices.size(),
	           VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
	           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	indexBuffer = Buffer(
	    *rw.get(), sizeof(uint32_t) * indices.size(),
	    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	uint32_t* indexData = indexStagingBuffer.map<uint32_t>();
	std::copy(indices.begin(), indices.end(), indexData);
	indexStagingBuffer.unmap();

	vk.wait(vk.graphicsQueue());
	copyCB.reset();
	copyCB.begin();
	copyCB.copyBuffer(indexStagingBuffer.get(), indexBuffer.get(),
	                  sizeof(uint32_t) * indices.size());
	copyCB.end();

	if (vk.queueSubmit(vk.graphicsQueue(), copyCB.get()) != VK_SUCCESS)
		throw std::runtime_error("could not copy index buffer");

	vk.wait(vk.graphicsQueue());
#pragma endregion
}

void Mesh::draw(CommandBuffer& cb)
{
	cb.bindVertexBuffer(vertexBuffer.get());
	cb.bindIndexBuffer(indexBuffer.get(), 0, VK_INDEX_TYPE_UINT32);
	cb.drawIndexed(uint32_t(indices.size()));
}

static void processMesh(RenderWindow& rw, aiMesh* mesh, const aiScene* scene,
                        std::vector<Mesh>& meshes)
{
	Mesh res;
	res.rw = &rw;

	res.vertices.reserve(mesh->mNumVertices);
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;
		vertex.position.x = mesh->mVertices[i].x;
		vertex.position.y = mesh->mVertices[i].y;
		vertex.position.z = mesh->mVertices[i].z;
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
		if (mesh->mTextureCoords[0])
		{
			vertex.uv.x = mesh->mTextureCoords[0][i].x;
			vertex.uv.y = mesh->mTextureCoords[0][i].y;
		}
		else
		{
			vertex.uv = {};
		}

		res.vertices.push_back(vertex);
	}

	res.indices.reserve(mesh->mNumFaces * 3);
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		const aiFace& face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			res.indices.push_back(face.mIndices[j]);
	}

	// process material
	// if(mesh->mMaterialIndex >= 0)
	//{
	//    ...
	//}

	res.init();

	meshes.emplace_back(std::move(res));
}

static void processNode(RenderWindow& rw, aiNode* node, const aiScene* scene,
                        std::vector<Mesh>& meshes)
{
	// process all the node's meshes (if any)
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		processMesh(rw, mesh, scene, meshes);
	}
	// then do the same for each of its children
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(rw, node->mChildren[i], scene, meshes);
	}
}

ShaderBall::ShaderBall(RenderWindow& renderWindow)
    : rw(renderWindow),
      gen(rd()),
      dis(0.0f, 0.3f),
      computeCB(
          CommandBuffer(rw.get().device(), rw.get().oneTimeCommandPool())),
      imguiCB(CommandBuffer(rw.get().device(), rw.get().oneTimeCommandPool())),
      copyHDRCB(
          CommandBuffer(rw.get().device(), rw.get().oneTimeCommandPool())),
      cb(CommandBuffer(rw.get().device(), rw.get().oneTimeCommandPool()))
{
	auto& vk = rw.get().device();

	vk.setLogActive();

#pragma region assimp
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
	    "D:/Projects/git/VkRenderer-data/ShaderBall.fbx",
	    aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
	    !scene->mRootNode)
		throw std::runtime_error(std::string("assimp error: ") +
		                         importer.GetErrorString());

	// directory = path.substr(0, path.find_last_of('/'));

	processNode(rw, scene->mRootNode, scene, m_meshes);

	// for (auto& mesh : m_meshes)
	//	mesh.init();
#pragma endregion

#pragma region color attachment texture
	m_colorAttachmentTexture = Texture2D(
	    rw, rw.get().swapchainExtent().width,
	    rw.get().swapchainExtent().height, VK_FORMAT_B8G8R8A8_UNORM,
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
	        // VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_colorAttachmentTexture.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
#pragma endregion

#pragma region depth texture
	m_depthTexture = DepthTexture(rw);

	m_depthTexture.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_UNDEFINED,
	    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
#pragma endregion

#pragma region renderPass
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = rw.get().depthImageFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout =
	    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.finalLayout =
	    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::array colorAttachments{ colorAttachment, depthAttachment };

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout =
	    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::array colorAttachmentRefs{

		colorAttachmentRef  //, depthAttachmentRef
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
	                           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
	    //|
	    //                          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
	    //                          |
	    //                          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
	    ;

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

#pragma region framebuffer
	vk::FramebufferCreateInfo framebufferInfo;
	framebufferInfo.renderPass = m_renderPass.get();
	std::array attachments = { m_colorAttachmentTexture.view(),
		                       m_depthTexture.view() };
	framebufferInfo.attachmentCount = uint32_t(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = m_colorAttachmentTexture.width();
	framebufferInfo.height = m_colorAttachmentTexture.height();
	framebufferInfo.layers = 1;

	m_framebuffer = vk.create(framebufferInfo);
	if (!m_framebuffer)
	{
		std::cerr << "error: failed to create framebuffer" << std::endl;
		abort();
	}
#pragma endregion

#pragma region vertexShader
	std::cout << "vertexShader" << std::endl;
	{
		using namespace sdw;
		ShaderBallShaderLib<VertexWriter> writer{ m_config };

		auto inPosition = writer.declInput<Vec3>("inPosition", 0);
		auto inNormal = writer.declInput<Vec3>("inNormal", 1);
		auto inUV = writer.declInput<Vec2>("inUV", 2);

		auto fragPosition = writer.declOutput<Vec3>("fragPosition", 0);
		auto fragNormal = writer.declOutput<Vec3>("fragNormal", 1);
		auto fragUV = writer.declOutput<Vec2>("fragUV", 2);

		auto out = writer.getOut();

		writer.implementMain([&]() {
			auto model = writer.ubo.getMember<Mat4>("model");
			auto view = writer.ubo.getMember<Mat4>("view");
			auto proj = writer.ubo.getMember<Mat4>("proj");

			out.vtx.position = proj * view * model * vec4(inPosition, 1.0_f);
			fragPosition = (model * vec4(inPosition, 1.0_f)).xyz();

			Mat4 normalMatrix4 = writer.declLocale<Mat4>(
			    "normalMatrix4", transpose(inverse(model)));

			Mat3 normalMatrix = writer.declLocale<Mat3>(
			    "normalMatrix",
			    mat3(vec3(normalMatrix4[0][0], normalMatrix4[0][1],
			              normalMatrix4[0][2]),
			         vec3(normalMatrix4[1][0], normalMatrix4[1][1],
			              normalMatrix4[1][2]),
			         vec3(normalMatrix4[2][0], normalMatrix4[2][1],
			              normalMatrix4[2][2])));

			fragNormal = normalMatrix * inNormal;
			fragUV = inUV;
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
	std::cout << "fragmentShader" << std::endl;
	{
		using namespace sdw;
		ShaderBallShaderLib<FragmentWriter> writer{ m_config };

		auto in = writer.getIn();

		auto fragPosition = writer.declInput<Vec3>("fragPosition", 0);
		auto fragNormal = writer.declInput<Vec3>("fragNormal", 1);
		auto fragUV = writer.declInput<Vec2>("fragUV", 2);

		auto fragColor = writer.declOutput<Vec4>("fragColor", 0u);

		writer.implementMain([&]() { fragColor = vec4(fragNormal, 1.0_f); });

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
	}  // namespace cdm
#pragma endregion

#pragma region descriptor pool
	std::array poolSizes{
		// VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		// VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1
		// },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
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
	// VkDescriptorSetLayoutBinding layoutBindingKernelImage{};
	// layoutBindingKernelImage.binding = 0;
	// layoutBindingKernelImage.descriptorCount = 1;
	// layoutBindingKernelImage.descriptorType =
	// VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; layoutBindingKernelImage.stageFlags =
	//    VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding layoutBindingUbo{};
	layoutBindingUbo.binding = 0;
	layoutBindingUbo.descriptorCount = 1;
	layoutBindingUbo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBindingUbo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	// VkDescriptorSetLayoutBinding layoutBindingImage{};
	// layoutBindingImage.binding = 2;
	// layoutBindingImage.descriptorCount = 1;
	// layoutBindingImage.descriptorType =
	//    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	// layoutBindingImage.stageFlags =
	//    VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	// std::array layoutBindings{ layoutBindingKernelImage, layoutBindingUbo,
	//	                       layoutBindingImage };

	std::array layoutBindings{ layoutBindingUbo };

	vk::DescriptorSetLayoutCreateInfo setLayoutInfo;
	// setLayoutInfo.bindingCount = 3;
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
	vk::DescriptorSetAllocateInfo setAlloc;
	setAlloc.descriptorPool = m_descriptorPool.get();
	setAlloc.descriptorSetCount = 1;
	setAlloc.pSetLayouts = &m_descriptorSetLayout.get();

	vk.allocate(setAlloc, &m_descriptorSet.get());

	/// TODO
	// m_descriptorSet = vk.allocate(setAlloc);

	if (!m_descriptorSet)
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
	pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout.get();
	// pipelineLayoutInfo.setLayoutCount = 0;
	// pipelineLayoutInfo.pSetLayouts = nullptr;
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

#pragma region pipeline
	std::cout << "pipeline" << std::endl;
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

	VkVertexInputAttributeDescription positionAttribute = {};
	positionAttribute.binding = 0;
	positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	positionAttribute.location = 0;
	positionAttribute.offset = 0;

	VkVertexInputAttributeDescription normalAttribute = {};
	normalAttribute.binding = 0;
	normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	normalAttribute.location = 1;
	normalAttribute.offset = sizeof(vector3);

	VkVertexInputAttributeDescription uvAttribute = {};
	uvAttribute.binding = 0;
	uvAttribute.format = VK_FORMAT_R32G32_SFLOAT;
	uvAttribute.location = 2;
	uvAttribute.offset = sizeof(vector3) + sizeof(vector3);

	std::array attributes = { positionAttribute, normalAttribute,
		                      uvAttribute };

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
	viewport.width = float(m_colorAttachmentTexture.width());
	viewport.height = float(m_colorAttachmentTexture.height());
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent.width = m_colorAttachmentTexture.width();
	scissor.extent.height = m_colorAttachmentTexture.height();

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
	colorHDRBlendAttachment.blendEnable = false;
	colorHDRBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorHDRBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorHDRBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorHDRBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorHDRBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorHDRBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	// colorHDRBlendAttachment.blendEnable = true;
	// colorHDRBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	// colorHDRBlendAttachment.dstColorBlendFactor =
	//    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	// colorHDRBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	// colorHDRBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	// colorHDRBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	// colorHDRBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	std::array colorBlendAttachments{
		colorBlendAttachment,
		// colorHDRBlendAttachment
	};

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

	m_pipeline = vk.create(pipelineInfo);
	if (!m_pipeline)
	{
		std::cerr << "error: failed to create graphics pipeline" << std::endl;
		abort();
	}
#pragma endregion

#pragma region UBO
	m_ubo = Buffer(rw, sizeof(m_config), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	               VMA_MEMORY_USAGE_CPU_TO_GPU,
	               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	cameraTr.position.z = -10.0f;
	cameraTr.position.y = 10.0f;
	 cameraTr.rotation = quaternion(vector3{ 0, 1, 0 }, 180_deg);

	//modelTr =
	//    transform3d(vector3{ 0, 0, 0 },
	//                quaternion(vector3{ 0, 1, 0 }, 180_deg), { 1, 1, 1 });

	m_config.model = matrix4(modelTr).get_transposed();
	m_config.view = matrix4(cameraTr).get_inversed().get_transposed();
	m_config.proj =
	    matrix4::perspective(90_deg, 1280.0f / 720.0f, 0.01f, 100.0f);

	m_config.copyTo(m_ubo.map());
	m_ubo.unmap();

	VkDescriptorBufferInfo setBufferInfo{};
	setBufferInfo.buffer = m_ubo.get();
	setBufferInfo.range = sizeof(m_config);
	setBufferInfo.offset = 0;

	vk::WriteDescriptorSet uboWrite;
	uboWrite.descriptorCount = 1;
	uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboWrite.dstArrayElement = 0;
	uboWrite.dstBinding = 0;
	uboWrite.dstSet = m_descriptorSet.get();
	uboWrite.pBufferInfo = &setBufferInfo;

	vk.updateDescriptorSets(uboWrite);
#pragma endregion
}

ShaderBall::~ShaderBall() {}

void ShaderBall::renderOpaque(CommandBuffer& cb)
{
	VkClearValue clearColor{};
	clearColor.color.float32[0] = 0X27 / 255.0f;
	clearColor.color.float32[1] = 0X28 / 255.0f;
	clearColor.color.float32[2] = 0X22 / 255.0f;

	VkClearValue clearDepth{};
	clearDepth.depthStencil.depth = 1.0f;

	std::array clearValues = { clearColor, clearDepth };

	vk::RenderPassBeginInfo rpInfo;
	rpInfo.framebuffer = m_framebuffer.get();
	rpInfo.renderPass = m_renderPass.get();
	rpInfo.renderArea.extent.width = width;
	rpInfo.renderArea.extent.height = height;
	rpInfo.clearValueCount = 2;
	rpInfo.pClearValues = clearValues.data();

	vk::SubpassBeginInfo subpassBeginInfo;
	subpassBeginInfo.contents = VK_SUBPASS_CONTENTS_INLINE;

	vk::SubpassEndInfo subpassEndInfo;

	cb.beginRenderPass2(rpInfo, subpassBeginInfo);

	cb.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.get());
	cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS,
	                     m_pipelineLayout.get(), 0, m_descriptorSet.get());

	for (auto& mesh : m_meshes)
		mesh.draw(cb);

	cb.endRenderPass2(subpassEndInfo);
}

void ShaderBall::standaloneDraw()
{
	auto& vk = rw.get().device();

	vk.wait(vk.graphicsQueue());

	m_config.model = matrix4(modelTr).get_transposed();
	m_config.view = matrix4(cameraTr).get_inversed().get_transposed();
	m_config.proj =
	    matrix4::perspective(90_deg, 1280.0f / 720.0f, 0.01f, 100.0f);

	m_config.copyTo(m_ubo.map());
	m_ubo.unmap();

	imguiCB.reset();
	imguiCB.begin();
	imguiCB.debugMarkerBegin("render", 0.2f, 0.2f, 1.0f);
	// ShaderBall.compute(imguiCB);
	renderOpaque(imguiCB);
	// imgui(imguiCB);
	imguiCB.debugMarkerEnd();
	imguiCB.end();
	if (vk.queueSubmit(vk.graphicsQueue(), imguiCB.get()) != VK_SUCCESS)
	{
		std::cerr << "error: failed to submit imgui command buffer"
		          << std::endl;
		abort();
	}
	vk.wait(vk.graphicsQueue());

	// m_config.samples += 1.0f;

	auto swapImages = rw.get().swapchainImages();

	vk.debugMarkerSetObjectName(copyHDRCB.get(),
	                            VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
	                            "copyHDRCB");

	copyHDRCB.reset();
	copyHDRCB.begin();

	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_colorAttachmentTexture.get();
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	copyHDRCB.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
	                          VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);

	for (auto swapImage : swapImages)
	{
		vk::ImageMemoryBarrier swapBarrier = barrier;
		swapBarrier.image = swapImage;
		swapBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		swapBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		swapBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		swapBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		copyHDRCB.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
		                          VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
		                          swapBarrier);

		VkImageBlit blit{};
		blit.srcOffsets[1].x = 1280;
		blit.srcOffsets[1].y = 720;
		blit.srcOffsets[1].z = 1;
		blit.dstOffsets[1].x = 1280;
		blit.dstOffsets[1].y = 720;
		blit.dstOffsets[1].z = 1;
		blit.srcSubresource.aspectMask =
		    VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.srcSubresource.mipLevel = 0;
		blit.dstSubresource.aspectMask =
		    VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;
		blit.dstSubresource.mipLevel = 0;

		copyHDRCB.blitImage(m_colorAttachmentTexture.get(),
		                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapImage,
		                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, blit,
		                    VkFilter::VK_FILTER_LINEAR);

		swapBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		swapBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		swapBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		swapBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		copyHDRCB.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
		                          VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
		                          swapBarrier);
	}

	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	copyHDRCB.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
	                          VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);

	if (copyHDRCB.end() != VK_SUCCESS)
	{
		std::cerr << "error: failed to record command buffer" << std::endl;
		abort();
	}

	vk.wait(vk.graphicsQueue());

	if (vk.queueSubmit(vk.graphicsQueue(), copyHDRCB.get()) != VK_SUCCESS)
	{
		std::cerr << "error: failed to submit draw command buffer"
		          << std::endl;
		abort();
	}
	vk.wait(vk.graphicsQueue());
}

// void ShaderBall::render(CommandBuffer& cb)
//{
//	std::array clearValues = { VkClearValue{}, VkClearValue{} };
//
//	vk::RenderPassBeginInfo rpInfo;
//	rpInfo.framebuffer = m_framebuffer.get();
//	rpInfo.renderPass = m_renderPass.get();
//	rpInfo.renderArea.extent.width = width;
//	rpInfo.renderArea.extent.height = height;
//	rpInfo.clearValueCount = 1;
//	rpInfo.pClearValues = clearValues.data();
//
//	vk::SubpassBeginInfo subpassBeginInfo;
//	subpassBeginInfo.contents = VK_SUBPASS_CONTENTS_INLINE;
//
//	vk::SubpassEndInfo subpassEndInfo;
//
//	cb.beginRenderPass2(rpInfo, subpassBeginInfo);
//
//	cb.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.get());
//	cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS,
//	                     m_computePipelineLayout.get(), 0, m_computeSet.get());
//	cb.bindVertexBuffer(m_vertexBuffer.get());
//	// cb.draw(POINT_COUNT);
//	cb.draw(3);
//
//	cb.endRenderPass2(subpassEndInfo);
//}
//
// void ShaderBall::compute(CommandBuffer& cb)
//{
//	cb.bindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline.get());
//	cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE,
//	                     m_computePipelineLayout.get(), 0, m_computeSet.get());
//	cb.dispatch(width / 8, height / 8);
//}
//
// void ShaderBall::imgui(CommandBuffer& cb)
//{
//	ImGui_ImplVulkan_NewFrame();
//	ImGui_ImplGlfw_NewFrame();
//	ImGui::NewFrame();
//
//	{
//		static float f = 0.0f;
//		static int counter = 0;
//
//		ImGui::Begin("Controls");
//
//		bool changed = false;
//
//		changed |= ImGui::SliderFloat("CamFocalDistance",
//		                              &m_config.camFocalDistance, 0.1f, 30.0f);
//		changed |= ImGui::SliderFloat("CamFocalLength",
//		                              &m_config.camFocalLength, 0.0f, 20.0f);
//		changed |= ImGui::SliderFloat("CamAperture", &m_config.camAperture,
//		                              0.0f, 5.0f);
//
//		changed |= ImGui::DragFloat3("rotation", &m_config.camRot.x, 0.01f);
//		changed |= ImGui::DragFloat3("lightDir", &m_config.lightDir.x, 0.01f);
//
//		changed |= ImGui::SliderFloat("scene radius", &m_config.sceneRadius,
//		                              0.0f, 10.0f);
//
//		ImGui::SliderFloat("BloomAscale1", &m_config.bloomAscale1, 0.0f, 1.0f);
//		ImGui::SliderFloat("BloomAscale2", &m_config.bloomAscale2, 0.0f, 1.0f);
//		ImGui::SliderFloat("BloomBscale1", &m_config.bloomBscale1, 0.0f, 1.0f);
//		ImGui::SliderFloat("BloomBscale2", &m_config.bloomBscale2, 0.0f, 1.0f);
//
//		if (changed)
//			applyImguiParameters();
//
//		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
//		            1000.0f / ImGui::GetIO().Framerate,
//		            ImGui::GetIO().Framerate);
//		ImGui::End();
//	}
//
//	ImGui::Render();
//
//	{
//		std::array clearValues = { VkClearValue{}, VkClearValue{} };
//
//		vk::RenderPassBeginInfo rpInfo;
//		rpInfo.framebuffer = m_framebuffer.get();
//		rpInfo.renderPass = rw.get().imguiRenderPass();
//		rpInfo.renderArea.extent.width = width;
//		rpInfo.renderArea.extent.height = height;
//		rpInfo.clearValueCount = 1;
//		rpInfo.pClearValues = clearValues.data();
//
//		vk::SubpassBeginInfo subpassBeginInfo;
//		subpassBeginInfo.contents = VK_SUBPASS_CONTENTS_INLINE;
//
//		cb.beginRenderPass2(rpInfo, subpassBeginInfo);
//	}
//
//	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb.get());
//
//	vk::SubpassEndInfo subpassEndInfo2;
//	cb.endRenderPass2(subpassEndInfo2);
//}
//
// void ShaderBall::standaloneDraw()
//{
//	auto& vk = rw.get().device();
//
//	if (mustClear)
//	{
//		mustClear = false;
//		m_config.samples = -1.0f;
//
//		cb.reset();
//		cb.begin();
//
//		vk::ImageMemoryBarrier barrier;
//		barrier.image = outputImage();
//		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
//		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
//		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
//		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//		barrier.subresourceRange.baseMipLevel = 0;
//		barrier.subresourceRange.levelCount = outputTexture().mipLevels();
//		barrier.subresourceRange.baseArrayLayer = 0;
//		barrier.subresourceRange.layerCount = 1;
//		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//		cb.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
//		                   VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);
//
//		VkClearColorValue clearColor{};
//		VkImageSubresourceRange range{};
//		range.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
//		range.layerCount = 1;
//		range.levelCount = 1;
//		cb.clearColorImage(outputImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//		                   &clearColor, range);
//
//		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
//		cb.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
//		                   VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);
//
//		cb.end();
//
//		if (vk.queueSubmit(vk.graphicsQueue(), imguiCB.get()) != VK_SUCCESS)
//		{
//			std::cerr << "error: failed to submit clear command buffer"
//			          << std::endl;
//			abort();
//		}
//
//		vk.wait(vk.graphicsQueue());
//		m_config.samples += 1.0f;
//	}
//
//	setSampleAndRandomize(m_config.samples);
//
//	computeCB.reset();
//	computeCB.begin();
//	computeCB.debugMarkerBegin("compute", 1.0f, 0.2f, 0.2f);
//	compute(computeCB);
//	computeCB.debugMarkerEnd();
//	computeCB.end();
//	if (vk.queueSubmit(vk.graphicsQueue(), computeCB.get()) != VK_SUCCESS)
//	{
//		std::cerr << "error: failed to submit imgui command buffer"
//		          << std::endl;
//		abort();
//	}
//	vk.wait(vk.graphicsQueue());
//
//	outputTextureHDR().generateMipmapsImmediate(VK_IMAGE_LAYOUT_GENERAL);
//
//	imguiCB.reset();
//	imguiCB.begin();
//	imguiCB.debugMarkerBegin("render", 0.2f, 0.2f, 1.0f);
//	// ShaderBall.compute(imguiCB);
//	render(imguiCB);
//	imgui(imguiCB);
//	imguiCB.debugMarkerEnd();
//	imguiCB.end();
//	if (vk.queueSubmit(vk.graphicsQueue(), imguiCB.get()) != VK_SUCCESS)
//	{
//		std::cerr << "error: failed to submit imgui command buffer"
//		          << std::endl;
//		abort();
//	}
//	vk.wait(vk.graphicsQueue());
//
//	m_config.samples += 1.0f;
//
//	auto swapImages = rw.get().swapchainImages();
//
//	vk.debugMarkerSetObjectName(copyHDRCB.get(),
//	                            VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
//	                            "copyHDRCB");
//
//	copyHDRCB.reset();
//	copyHDRCB.begin();
//
//	vk::ImageMemoryBarrier barrier;
//	barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
//	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
//	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
//	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
//	barrier.image = outputImage();
//	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//	barrier.subresourceRange.baseMipLevel = 0;
//	barrier.subresourceRange.levelCount = 1;
//	barrier.subresourceRange.baseArrayLayer = 0;
//	barrier.subresourceRange.layerCount = 1;
//	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
//	copyHDRCB.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
//	                          VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);
//
//	for (auto swapImage : swapImages)
//	{
//		vk::ImageMemoryBarrier swapBarrier = barrier;
//		swapBarrier.image = swapImage;
//		swapBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
//		swapBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//		swapBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
//		swapBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//		copyHDRCB.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
//		                          VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
//		                          swapBarrier);
//
//		VkImageBlit blit{};
//		blit.srcOffsets[1].x = 1280;
//		blit.srcOffsets[1].y = 720;
//		blit.srcOffsets[1].z = 1;
//		blit.dstOffsets[1].x = 1280;
//		blit.dstOffsets[1].y = 720;
//		blit.dstOffsets[1].z = 1;
//		blit.srcSubresource.aspectMask =
//		    VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
//		blit.srcSubresource.baseArrayLayer = 0;
//		blit.srcSubresource.layerCount = 1;
//		blit.srcSubresource.mipLevel = 0;
//		blit.dstSubresource.aspectMask =
//		    VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
//		blit.dstSubresource.baseArrayLayer = 0;
//		blit.dstSubresource.layerCount = 1;
//		blit.dstSubresource.mipLevel = 0;
//
//		copyHDRCB.blitImage(outputImage(),
//		                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapImage,
//		                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, blit,
//		                    VkFilter::VK_FILTER_LINEAR);
//
//		swapBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//		swapBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
//		swapBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//		swapBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
//
//		copyHDRCB.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
//		                          VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
//		                          swapBarrier);
//	}
//
//	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
//	barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
//	barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
//	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//	copyHDRCB.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
//	                          VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);
//
//	if (copyHDRCB.end() != VK_SUCCESS)
//	{
//		std::cerr << "error: failed to record command buffer" << std::endl;
//		abort();
//	}
//
//	vk.wait(vk.graphicsQueue());
//
//	if (vk.queueSubmit(vk.graphicsQueue(), copyHDRCB.get()) != VK_SUCCESS)
//	{
//		std::cerr << "error: failed to submit draw command buffer"
//		          << std::endl;
//		abort();
//	}
//	vk.wait(vk.graphicsQueue());
//}
//
// void ShaderBall::applyImguiParameters()
//{
//	auto& vk = rw.get().device();
//
//	m_config.samples = -1.0f;
//	mustClear = true;
//}
//
// void ShaderBall::randomizePoints()
//{
//	auto& vk = rw.get().device();
//
//	using Vertex = std::array<float, 2>;
//	std::vector<Vertex> vertices(POINT_COUNT);
//
//	for (auto& vertex : vertices)
//	{
//		vertex[0] = dis(gen);
//		vertex[1] = dis(gen);
//	}
//
//	Vertex* data = m_vertexBuffer.map<Vertex>();
//	std::copy(vertices.begin(), vertices.end(), static_cast<Vertex*>(data));
//	m_vertexBuffer.unmap();
//
//	m_config.seed = udis(gen);
//	m_config.copyTo(m_computeUbo.map());
//	m_computeUbo.unmap();
//}
//
// void ShaderBall::setSampleAndRandomize(float s)
//{
//	m_config.samples = s;
//	m_config.seed = udis(gen);
//	m_config.copyTo(m_computeUbo.map());
//	m_computeUbo.unmap();
//}
}  // namespace cdm
