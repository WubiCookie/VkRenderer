#include "ShaderBall.hpp"

#include "EquirectangularToCubemap.hpp"
//#include "EquirectangularToIrradianceMap.hpp"
//#include "PrefilterCubemap.hpp"

#include "CompilerSpirV/compileSpirV.hpp"
#include "ShaderWriter/Intrinsics/Intrinsics.hpp"
#include "ShaderWriter/Source.hpp"

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "stb_image.h"

#include <array>
#include <iostream>
#include <stdexcept>

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
		(void)ubo.declMember<Vec3>("viewPos");
		(void)ubo.declMember<Vec3>("lightPos");
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
		if (mesh->HasTangentsAndBitangents())
		{
			vertex.tangent.x = mesh->mTangents[i].x;
			vertex.tangent.y = mesh->mTangents[i].y;
			vertex.tangent.z = mesh->mTangents[i].z;
		}
		else
		{
			vertex.tangent = { 0, 1, 0 };
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
      imguiCB(CommandBuffer(rw.get().device(), rw.get().oneTimeCommandPool())),
      copyHDRCB(
          CommandBuffer(rw.get().device(), rw.get().oneTimeCommandPool()))
{
	auto& vk = rw.get().device();

	vk.setLogActive();

#pragma region assimp
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
	    "D:/Projects/git/VkRenderer-data/ShaderBall.fbx",
	    aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals |
	        aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
	    !scene->mRootNode)
		throw std::runtime_error(std::string("assimp error: ") +
		                         importer.GetErrorString());

	// directory = path.substr(0, path.find_last_of('/'));

	processNode(rw, scene->mRootNode, scene, m_meshes);

	for (uint32_t i = 0; i < m_meshes.size(); i++)
	{
		auto& mesh = m_meshes[i];
		mesh.materialData.textureIndices = { i, i, i, i, i };
	}
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
	framebufferInfo.renderPass = m_renderPass;
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

#define Locale(name, value) auto name = writer.declLocale(#name, value);
#define FLOAT(name) auto name = writer.declLocale<Float>(#name);
#define VEC2(name) auto name = writer.declLocale<Vec2>(#name);
#define VEC3(name) auto name = writer.declLocale<Vec3>(#name);
#define Constant(name, value) auto name = writer.declConstant(#name, value);

#pragma region vertexShader
	std::cout << "vertexShader" << std::endl;
	{
		using namespace sdw;
		ShaderBallShaderLib<VertexWriter> writer{ m_config };

		auto inPosition = writer.declInput<Vec3>("inPosition", 0);
		auto inNormal = writer.declInput<Vec3>("inNormal", 1);
		auto inUV = writer.declInput<Vec2>("inUV", 2);
		auto inTangent = writer.declInput<Vec3>("inTangent", 3);

		auto fragPosition = writer.declOutput<Vec3>("fragPosition", 0);
		auto fragUV = writer.declOutput<Vec2>("fragUV", 1);
		auto fragTanLightPos = writer.declOutput<Vec3>("fragTanLightPos", 2);
		auto fragTanViewPos = writer.declOutput<Vec3>("fragTanViewPos", 3);
		auto fragTanFragPos = writer.declOutput<Vec3>("fragTanFragPos", 4);
		auto fragNormal = writer.declOutput<Vec3>("fragNormal", 5);
		auto fragTangent = writer.declOutput<Vec3>("fragTangent", 6);

		auto out = writer.getOut();

		writer.implementMain([&]() {
			auto model = writer.ubo.getMember<Mat4>("model");
			auto view = writer.ubo.getMember<Mat4>("view");
			auto proj = writer.ubo.getMember<Mat4>("proj");

			fragPosition = (model * vec4(inPosition, 1.0_f)).xyz();
			fragUV = inUV;

			Locale(model3, mat3(vec3(model[0][0], model[0][1], model[0][2]),
			                    vec3(model[1][0], model[1][1], model[1][2]),
			                    vec3(model[2][0], model[2][1], model[2][2])));

			Locale(normalMatrix, transpose(inverse(model3)));

			fragNormal = normalize(normalMatrix * inNormal);
			fragTangent = normalize(normalMatrix * inTangent);

			fragTangent = normalize(fragTangent -
			                        dot(fragTangent, fragNormal) * fragNormal);

			Locale(B, cross(fragNormal, fragTangent));

			Locale(TBN, transpose(mat3(fragTangent, B, fragNormal)));

			fragTanLightPos = TBN * writer.ubo.getMember<Vec3>("lightPos");
			fragTanViewPos = TBN * writer.ubo.getMember<Vec3>("viewPos");
			fragTanFragPos = TBN * fragPosition;

			out.vtx.position = proj * view * model * vec4(inPosition, 1.0_f);
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
		auto fragUV = writer.declInput<Vec2>("fragUV", 1);
		auto fragTanLightPos = writer.declInput<Vec3>("fragTanLightPos", 2);
		auto fragTanViewPos = writer.declInput<Vec3>("fragTanViewPos", 3);
		auto fragTanFragPos = writer.declInput<Vec3>("fragTanFragPos", 4);
		auto fragNormal = writer.declInput<Vec3>("fragNormal", 5);
		auto fragTangent = writer.declInput<Vec3>("fragTangent", 6);

		auto fragColor = writer.declOutput<Vec4>("fragColor", 0);

		auto albedos =
		    writer.declSampledImageArray<FImg2DRgba32>("albedos", 1, 0, 16);
		auto displacements = writer.declSampledImageArray<FImg2DRgba32>(
		    "displacements", 2, 0, 16);
		auto metalnesses = writer.declSampledImageArray<FImg2DRgba32>(
		    "metalnesses", 3, 0, 16);
		auto normals =
		    writer.declSampledImageArray<FImg2DRgba32>("normals", 4, 0, 16);
		auto roughnesses = writer.declSampledImageArray<FImg2DRgba32>(
		    "roughnesses", 5, 0, 16);
		auto irradianceMap =
		    writer.declSampledImage<FImgCubeRgba32>("irradianceMap", 6, 0);
		auto prefilteredMap =
		    writer.declSampledImage<FImgCubeRgba32>("prefilteredMap", 7, 0);
		auto brdfLut = writer.declSampledImage<FImg2DRg32>("brdfLut", 8, 0);

		auto pc = Pcb(writer, "pcTextureIndices");
		pc.declMember<Int>("albedoIndex");
		pc.declMember<Int>("displacementIndex");
		pc.declMember<Int>("metalnessIndex");
		pc.declMember<Int>("normalIndex");
		pc.declMember<Int>("roughnessIndex");
		pc.declMember<Float>("uShift");
		pc.declMember<Float>("uScale");
		pc.declMember<Float>("vShift");
		pc.declMember<Float>("vScale");
		pc.end();

		Constant(PI, 3.14159265359_f);

		auto DistributionGGX = writer.implementFunction<Float>(
		    "DistributionGGX",
		    [&](const Vec3& N, const Vec3& H, const Float& roughness) {
			    Locale(a, roughness * roughness);
			    Locale(a2, a * a);
			    Locale(NdotH, max(dot(N, H), 0.0_f));
			    Locale(NdotH2, NdotH * NdotH);

			    Locale(denom, NdotH2 * (a2 - 1.0_f) + 1.0_f);
			    denom = 3.14159265359_f * denom * denom;

			    writer.returnStmt(a2 / denom);
		    },
		    InVec3{ writer, "N" }, InVec3{ writer, "H" },
		    InFloat{ writer, "roughness" });

		auto GeometrySchlickGGX = writer.implementFunction<Float>(
		    "GeometrySchlickGGX",
		    [&](const Float& NdotV, const Float& roughness) {
			    Locale(r, roughness + 1.0_f);
			    Locale(k, (r * r) / 8.0_f);

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

		auto fresnelSchlick = writer.implementFunction<Vec4>(
		    "fresnelSchlick",
		    [&](const Float& cosTheta, const Vec4& F0) {
			    writer.returnStmt(F0 + (vec4(1.0_f) - F0) *
			                               vec4(pow(1.0_f - cosTheta, 5.0_f)));
		    },
		    InFloat{ writer, "cosTheta" }, InVec4{ writer, "F0" });

		auto fresnelSchlickRoughness = writer.implementFunction<Vec4>(
		    "fresnelSchlickRoughness",
		    [&](const Float& cosTheta, const Vec4& F0,
		        const Float& roughness) {
			    writer.returnStmt(F0 +
			                      (max(vec4(1.0_f - roughness), F0) - F0) *
			                          vec4(pow(1.0_f - cosTheta, 5.0_f)));
		    },
		    InFloat{ writer, "cosTheta" }, InVec4{ writer, "F0" },
		    InFloat{ writer, "roughness" });

		writer.implementMain([&]() {
			Locale(scaledUV, (fragUV * vec2(pc.getMember<Float>("uScale"),
			                                pc.getMember<Float>("vScale")) +
			                  vec2(pc.getMember<Float>("uShift"),
			                       pc.getMember<Float>("vShift"))));

			Locale(albedo,
			       pow(texture(albedos[pc.getMember<Int>("albedoIndex")],
			                   scaledUV),
			           vec4(2.2_f)));
			Locale(metalness,
			       texture(metalnesses[pc.getMember<Int>("metalnessIndex")],
			               scaledUV)
			           .r());
			Locale(roughness,
			       texture(roughnesses[pc.getMember<Int>("roughnessIndex")],
			               scaledUV)
			           .r());

			Locale(N,
			       texture(normals[pc.getMember<Int>("normalIndex")], scaledUV)
			           .xyz());
			N = normalize((N * vec3(2.0_f) - vec3(1.0_f)));
			Locale(V, normalize(fragTanViewPos - fragTanFragPos));

			Locale(F0, mix(vec4(0.04_f), albedo, vec4(metalness)));

			Locale(Lo, vec4(0.0_f));

			Locale(F, vec4(0.0_f));
			Locale(kS, vec4(0.0_f));
			Locale(kD, vec4(0.0_f));
			Locale(specular, vec4(0.0_f));

			// FOR
			// Locale(L, normalize(fragTanLightPos - fragTanFragPos));
			// Locale(H, normalize(V + L));
			// Locale(distance, length(fragTanLightPos - fragTanFragPos));
			// Locale(attenuation, 1.0_f / (distance * distance));
			// Locale(radiance, vec4(attenuation));
			//
			// Locale(NDF, DistributionGGX(N, H, roughness));
			// Locale(G, GeometrySmith(N, V, L, roughness));
			// F = fresnelSchlick(max(dot(H, V), 0.0_f), F0);
			//
			// Locale(nominator, NDF * G * F);
			// Locale(denominator,
			//       4.0_f * max(dot(N, V), 0.0_f) * max(dot(N, L), 0.0_f) +
			//           0.001_f);
			// Locale(specular, nominator / vec4(denominator));
			//
			// kS = F;
			// kD = vec4(1.0_f) - kS;
			// kD = kD * vec4(1.0_f - metalness);
			//
			// Locale(NdotL, max(dot(N, L), 0.0_f));
			//
			// Lo += (kD * albedo / vec4(PI) + specular) * radiance *
			// vec4(NdotL);
			// ROF

			F = fresnelSchlickRoughness(max(dot(N, V), 0.0_f), F0, roughness);

			kS = F;
			kD = vec4(1.0_f) - kS;
			kD = kD * vec4(1.0_f - metalness);

			Locale(B, cross(fragNormal, fragTangent));
			// Locale(TBN, inverse(transpose(mat3(fragTangent, B,
			// fragNormal))));
			Locale(TBN, inverse(transpose(mat3(fragTangent, B, fragNormal))));

			Locale(R, reflect(TBN * -V, TBN * N));

			Locale(irradiance, texture(irradianceMap, TBN * N));
			Locale(diffuse, irradiance * albedo);

			Locale(MAX_REFLECTION_LOD, 4.0_f);
			Locale(
			    prefilteredColor,
			    textureLod(prefilteredMap, R, roughness * MAX_REFLECTION_LOD));
			Locale(
			    brdf,
			    texture(brdfLut, vec2(max(dot(N, V), 0.0_f), roughness)).rg());
			specular = prefilteredColor * (F * brdf.x() + brdf.y());

			Locale(ambient, kD * diffuse + specular);

			Locale(color, ambient + Lo);

			color = color / (color + vec4(1.0_f));
			color = pow(color, vec4(1.0_f / 2.2_f));

			fragColor = color;
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
		// VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 83 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
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
	VkDescriptorSetLayoutBinding layoutBindingUbo{};
	layoutBindingUbo.binding = 0;
	layoutBindingUbo.descriptorCount = 1;
	layoutBindingUbo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBindingUbo.stageFlags =
	    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding layoutBindingAlbedoImages{};
	layoutBindingAlbedoImages.binding = 1;
	layoutBindingAlbedoImages.descriptorCount = 16;
	layoutBindingAlbedoImages.descriptorType =
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindingAlbedoImages.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding layoutBindingDisplacementImages{};
	layoutBindingDisplacementImages.binding = 2;
	layoutBindingDisplacementImages.descriptorCount = 16;
	layoutBindingDisplacementImages.descriptorType =
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindingDisplacementImages.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding layoutBindingMetalnessImages{};
	layoutBindingMetalnessImages.binding = 3;
	layoutBindingMetalnessImages.descriptorCount = 16;
	layoutBindingMetalnessImages.descriptorType =
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindingMetalnessImages.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding layoutBindingNormalImages{};
	layoutBindingNormalImages.binding = 4;
	layoutBindingNormalImages.descriptorCount = 16;
	layoutBindingNormalImages.descriptorType =
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindingNormalImages.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding layoutBindingRoughnessImages{};
	layoutBindingRoughnessImages.binding = 5;
	layoutBindingRoughnessImages.descriptorCount = 16;
	layoutBindingRoughnessImages.descriptorType =
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindingRoughnessImages.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding layoutBindingIrradianceMapImages{};
	layoutBindingIrradianceMapImages.binding = 6;
	layoutBindingIrradianceMapImages.descriptorCount = 1;
	layoutBindingIrradianceMapImages.descriptorType =
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindingIrradianceMapImages.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding layoutBindingPrefilteredMapImages{};
	layoutBindingPrefilteredMapImages.binding = 7;
	layoutBindingPrefilteredMapImages.descriptorCount = 1;
	layoutBindingPrefilteredMapImages.descriptorType =
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindingPrefilteredMapImages.stageFlags =
	    VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding layoutBindingBrdfLutImages{};
	layoutBindingBrdfLutImages.binding = 8;
	layoutBindingBrdfLutImages.descriptorCount = 1;
	layoutBindingBrdfLutImages.descriptorType =
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindingBrdfLutImages.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array layoutBindings{ layoutBindingUbo,
		                       layoutBindingAlbedoImages,
		                       layoutBindingDisplacementImages,
		                       layoutBindingMetalnessImages,
		                       layoutBindingNormalImages,
		                       layoutBindingRoughnessImages,
		                       layoutBindingIrradianceMapImages,
		                       layoutBindingPrefilteredMapImages,
		                       layoutBindingBrdfLutImages };

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
	pcRange.size = sizeof(Mesh::MaterialData);
	pcRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

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
	std::cout << "pipeline" << std::endl;
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

	VkVertexInputAttributeDescription tangentAttribute = {};
	tangentAttribute.binding = 0;
	tangentAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	tangentAttribute.location = 3;
	tangentAttribute.offset =
	    sizeof(vector3) + sizeof(vector3) + sizeof(vector2);

	std::array attributes = { positionAttribute, normalAttribute, uvAttribute,
		                      tangentAttribute };

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
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

#pragma region UBO
	m_matricesUBO = Buffer(
	    rw, sizeof(m_config), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	    VMA_MEMORY_USAGE_CPU_TO_GPU, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	// cameraTr.position.x = 15.0f;
	cameraTr.position.y = 10.0f;
	cameraTr.position.z = 15.0f;
	// cameraTr.rotation = quaternion(vector3{ 0, 1, 0 }, 180_deg);

	modelTr =
	    transform3d(vector3{ 0, 0, 0 },
	                quaternion(vector3{ 0, 1, 0 }, 180_deg), { 1, 1, 1 });

	m_config.model = matrix4(modelTr).get_transposed();
	m_config.view = matrix4(cameraTr).get_transposed().get_inversed();
	m_config.proj =
	    matrix4::perspective(90_deg,
	                         float(rw.get().swapchainExtent().width) /
	                             float(rw.get().swapchainExtent().height),
	                         0.01f, 100.0f);

	m_config.viewPos = cameraTr.position;

	m_config.copyTo(m_matricesUBO.map());
	m_matricesUBO.unmap();

	// m_materialUBO = Buffer(rw, sizeof(m_config),
	// VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	//               VMA_MEMORY_USAGE_CPU_TO_GPU,
	//               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	VkDescriptorBufferInfo setBufferInfo{};
	setBufferInfo.buffer = m_matricesUBO;
	setBufferInfo.range = sizeof(m_config);
	setBufferInfo.offset = 0;

	vk::WriteDescriptorSet uboWrite;
	uboWrite.descriptorCount = 1;
	uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboWrite.dstArrayElement = 0;
	uboWrite.dstBinding = 0;
	uboWrite.dstSet = m_descriptorSet;
	uboWrite.pBufferInfo = &setBufferInfo;

	vk.updateDescriptorSets(uboWrite);
#pragma endregion

#pragma region equirectangularHDR
	int w, h, c;
	float* imageData = stbi_loadf(
	    "D:/Projects/git/VkRenderer-data/PaperMill_Ruins_E/"
	    "PaperMill_E_3k.hdr",
	    //"D:/Projects/git/VkRenderer-data/Milkyway/"
	    //"Milkyway_small.hdr",
	    //"D:/Projects/git/VkRenderer-data/UV-Testgrid/"
	    //"testgrid.jpg",
	    &w, &h, &c, 4);

	if (!imageData)
		throw std::runtime_error("could not load equirectangular map");

	m_equirectangularTexture = Texture2D(
	    rw, w, h, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
	    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkBufferImageCopy copy{};
	copy.bufferRowLength = w;
	copy.bufferImageHeight = h;
	copy.imageExtent.width = w;
	copy.imageExtent.height = h;
	copy.imageExtent.depth = 1;
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.imageSubresource.baseArrayLayer = 0;
	copy.imageSubresource.layerCount = 1;
	copy.imageSubresource.mipLevel = 0;

	m_equirectangularTexture.uploadDataImmediate(
	    imageData, w * h * 4 * sizeof(float), copy, VK_IMAGE_LAYOUT_UNDEFINED,
	    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	stbi_image_free(imageData);

	// VkDescriptorImageInfo imageInfo{};
	// imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	// imageInfo.imageView = m_equirectangularTexture.view();
	// imageInfo.sampler = m_equirectangularTexture.sampler();

	// vk::WriteDescriptorSet textureWrite;
	// textureWrite.descriptorCount = 1;
	// textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	// textureWrite.dstArrayElement = 0;
	// textureWrite.dstBinding = 0;
	// textureWrite.dstSet = m_descriptorSet;
	// textureWrite.pImageInfo = &imageInfo;

	// vk.updateDescriptorSets(textureWrite);
#pragma endregion

#pragma region cubemaps
	{
		EquirectangularToCubemap e2c(rw, 1024);
		m_environmentMap = e2c.computeCubemap(m_equirectangularTexture);

		if (m_environmentMap.get() == nullptr)
			throw std::runtime_error("could not create environmentMap");

		m_irradianceMap = IrradianceMap(rw, 512, m_equirectangularTexture,
		                                "PaperMill_E_irradiance.hdr");

		if (m_irradianceMap.get() == nullptr)
			throw std::runtime_error("could not create irradianceMap");

		VkDescriptorImageInfo irradianceMapImageInfo{};
		irradianceMapImageInfo.imageLayout =
		    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		irradianceMapImageInfo.imageView = m_irradianceMap.get().view();
		irradianceMapImageInfo.sampler = m_irradianceMap.get().sampler();

		vk::WriteDescriptorSet irradianceMapTextureWrite;
		irradianceMapTextureWrite.descriptorCount = 1;
		irradianceMapTextureWrite.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		irradianceMapTextureWrite.dstArrayElement = 0;
		irradianceMapTextureWrite.dstBinding = 6;
		irradianceMapTextureWrite.dstSet = m_descriptorSet;
		irradianceMapTextureWrite.pImageInfo = &irradianceMapImageInfo;

		m_prefilteredMap = PrefilteredCubemap(rw, 1024, -1, m_environmentMap,
		                                      "PaperMill_E_prefiltered.hdr");

		if (m_prefilteredMap.get().get() == nullptr)
			throw std::runtime_error("could not create prefilteredMap");

		VkDescriptorImageInfo prefilteredMapImageInfo{};
		prefilteredMapImageInfo.imageLayout =
		    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		prefilteredMapImageInfo.imageView = m_prefilteredMap.get().view();
		prefilteredMapImageInfo.sampler = m_prefilteredMap.get().sampler();

		vk::WriteDescriptorSet prefilteredMapTextureWrite;
		prefilteredMapTextureWrite.descriptorCount = 1;
		prefilteredMapTextureWrite.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		prefilteredMapTextureWrite.dstArrayElement = 0;
		prefilteredMapTextureWrite.dstBinding = 7;
		prefilteredMapTextureWrite.dstSet = m_descriptorSet;
		prefilteredMapTextureWrite.pImageInfo = &prefilteredMapImageInfo;

		m_brdfLut = BrdfLut(rw, 128);

		if (m_brdfLut.get() == nullptr)
			throw std::runtime_error("could not create brdfLut");

		VkDescriptorImageInfo brdfLutImageInfo{};
		brdfLutImageInfo.imageLayout =
		    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		brdfLutImageInfo.imageView = m_brdfLut.get().view();
		brdfLutImageInfo.sampler = m_brdfLut.get().sampler();

		vk::WriteDescriptorSet brdfLutTextureWrite;
		brdfLutTextureWrite.descriptorCount = 1;
		brdfLutTextureWrite.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		brdfLutTextureWrite.dstArrayElement = 0;
		brdfLutTextureWrite.dstBinding = 8;
		brdfLutTextureWrite.dstSet = m_descriptorSet;
		brdfLutTextureWrite.pImageInfo = &brdfLutImageInfo;

		vk.updateDescriptorSets({ irradianceMapTextureWrite,
		                          prefilteredMapTextureWrite,
		                          brdfLutTextureWrite });
	}
#pragma endregion

#pragma region textures
	m_defaultTexture = Texture2D(
	    rw, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
	    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
	    VMA_MEMORY_USAGE_CPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	union U
	{
		uint32_t defaultTextureTexel;
		uint8_t b[4];
	};

	U u;
	u.b[0] = 255 / 2;
	u.b[1] = 255 / 2;
	u.b[2] = 255;
	u.b[3] = 255;

	VkBufferImageCopy defaultTextureCopy{};
	defaultTextureCopy.bufferImageHeight = 1;
	defaultTextureCopy.bufferRowLength = 1;
	defaultTextureCopy.imageExtent.width = 1;
	defaultTextureCopy.imageExtent.height = 1;
	defaultTextureCopy.imageExtent.depth = 1;
	defaultTextureCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	defaultTextureCopy.imageSubresource.baseArrayLayer = 0;
	defaultTextureCopy.imageSubresource.layerCount = 1;
	defaultTextureCopy.imageSubresource.mipLevel = 0;

	m_defaultTexture.uploadDataImmediate(
	    &u.defaultTextureTexel, sizeof(uint32_t), defaultTextureCopy,
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	std::vector<vk::WriteDescriptorSet> textureWrites;
	textureWrites.reserve(5);

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = m_defaultTexture.view();
	imageInfo.sampler = m_defaultTexture.sampler();

	for (uint32_t i = 0; i < 5; i++)
	{
		vk::WriteDescriptorSet textureWrite;
		textureWrite.descriptorCount = 1;
		textureWrite.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textureWrite.dstArrayElement = 0;
		textureWrite.dstBinding = i + 1;
		textureWrite.dstSet = m_descriptorSet;
		textureWrite.pImageInfo = &imageInfo;

		for (uint32_t j = 0; j < 16; j++)
		{
			textureWrite.dstArrayElement = j;
			textureWrites.push_back(textureWrite);
		}
	}

	vk.updateDescriptorSets(uint32_t(textureWrites.size()),
	                        textureWrites.data());

	auto updateTextureDescriptor = [&](std::string_view filename,
	                                   std::array<Texture2D, 16>& arr,
	                                   uint32_t element) {
		uint32_t binding;

		if (&arr == &m_albedos)
			binding = 1;
		else if (&arr == &m_displacements)
			binding = 2;
		else if (&arr == &m_metalnesses)
			binding = 3;
		else if (&arr == &m_normals)
			binding = 4;
		else if (&arr == &m_roughnesses)
			binding = 5;
		else
			return;

		int w, h, c;
		uint8_t* imageData = stbi_load(filename.data(), &w, &h, &c, 4);

		if (imageData)
		{
			arr[element] = Texture2D(
			    rw, w, h, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
			    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			    VMA_MEMORY_USAGE_GPU_ONLY,
			    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			VkBufferImageCopy copy{};
			copy.bufferImageHeight = h;
			copy.bufferRowLength = w;  // *4 * sizeof(float);
			copy.imageExtent.width = w;
			copy.imageExtent.height = h;
			copy.imageExtent.depth = 1;
			copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copy.imageSubresource.baseArrayLayer = 0;
			copy.imageSubresource.layerCount = 1;
			copy.imageSubresource.mipLevel = 0;

			arr[element].uploadDataImmediate(
			    imageData, w * h * 4 * sizeof(uint8_t), copy,
			    VK_IMAGE_LAYOUT_UNDEFINED,
			    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			stbi_image_free(imageData);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = arr[element].view();
			imageInfo.sampler = arr[element].sampler();

			vk::WriteDescriptorSet textureWrite;
			textureWrite.descriptorCount = 1;
			textureWrite.descriptorType =
			    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			textureWrite.dstArrayElement = element;
			textureWrite.dstBinding = binding;
			textureWrite.dstSet = m_descriptorSet;
			textureWrite.pImageInfo = &imageInfo;

			vk.updateDescriptorSets(textureWrite);
		}
	};

	std::string resourcePath = "D:/Projects/git/VkRenderer-data/";
	resourcePath += "Metal007_4K-JPG/";
	// resourcePath += "Marble009_8K-JPG/";
	// resourcePath += "Leather011_8K-JPG/";
	// resourcePath += "ChristmasTreeOrnament006_8K-JPG/";
	// resourcePath += "Chip001_4K-JPG";

	// updateTextureDescriptor(
	//    "D:/Projects/git/VkRenderer-data/Marble009_2K-JPG/"
	//    "Marble009_2K_Color.jpg",
	//    m_albedos, 1);
	updateTextureDescriptor(resourcePath + "/Color.jpg", m_albedos, 1);
	updateTextureDescriptor(resourcePath + "/Displacement.jpg",
	                        m_displacements, 1);
	updateTextureDescriptor(resourcePath + "/Metalness.jpg", m_metalnesses, 1);
	updateTextureDescriptor(resourcePath + "/Normal.jpg", m_normals, 1);
	updateTextureDescriptor(resourcePath + "/Roughness.jpg", m_roughnesses, 1);

	updateTextureDescriptor(
	    "D:/Projects/git/VkRenderer-data/MathieuMaurel ShaderBall "
	    "2017/Textures/ShaderBall_A_INBALL.png",
	    m_albedos, 2);
	updateTextureDescriptor(
	    "D:/Projects/git/VkRenderer-data/MathieuMaurel ShaderBall "
	    "2017/Textures/ShaderBall_A_REPERE.png",
	    m_albedos, 8);
	updateTextureDescriptor(
	    "D:/Projects/git/VkRenderer-data/MathieuMaurel ShaderBall "
	    "2017/Textures/ShaderBall_A_CYCLO.png",
	    m_albedos, 9);
	m_meshes[9].materialData.uScale = 20.0f;
	m_meshes[9].materialData.vScale = 20.0f;
#pragma endregion

#pragma region skybox
	m_skybox = std::make_unique<Skybox>(rw, m_renderPass.get(), viewport,
	                                    m_environmentMap);
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
	rpInfo.framebuffer = m_framebuffer;
	rpInfo.renderPass = m_renderPass;
	rpInfo.renderArea.extent.width = rw.get().swapchainExtent().width;
	rpInfo.renderArea.extent.height = rw.get().swapchainExtent().height;
	rpInfo.clearValueCount = 2;
	rpInfo.pClearValues = clearValues.data();

	vk::SubpassBeginInfo subpassBeginInfo;
	subpassBeginInfo.contents = VK_SUBPASS_CONTENTS_INLINE;

	vk::SubpassEndInfo subpassEndInfo;

	cb.beginRenderPass2(rpInfo, subpassBeginInfo);

	m_skybox->render(cb);

	cb.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
	cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0,
	                     m_descriptorSet);

	for (uint32_t i = 0; i < m_meshes.size(); i++)
	{
		auto& mesh = m_meshes[i];
		cb.pushConstants(m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
		                 &mesh.materialData);
		mesh.draw(cb);
	}

	cb.endRenderPass2(subpassEndInfo);
}

void ShaderBall::standaloneDraw()
{
	auto& vk = rw.get().device();

	vk.wait(vk.graphicsQueue());

	m_config.model = matrix4(modelTr).get_transposed();
	m_config.view = matrix4(cameraTr).get_transposed().get_inversed();
	m_config.proj =
	    matrix4::perspective(90_deg,
	                         float(rw.get().swapchainExtent().width) /
	                             float(rw.get().swapchainExtent().height),
	                         0.01f, 100.0f)
	        .get_transposed();
	m_config.viewPos = cameraTr.position;

	m_config.copyTo(m_matricesUBO.map());
	m_matricesUBO.unmap();

	m_skybox->setMatrices(m_config.proj, m_config.view);

	imguiCB.reset();
	imguiCB.begin();
	imguiCB.debugMarkerBegin("render", 0.2f, 0.2f, 1.0f);
	renderOpaque(imguiCB);
	imguiCB.debugMarkerEnd();
	imguiCB.end();
	if (vk.queueSubmit(vk.graphicsQueue(), imguiCB) != VK_SUCCESS)
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
	barrier.image = m_colorAttachmentTexture;
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
		blit.srcOffsets[1].x = rw.get().swapchainExtent().width;
		blit.srcOffsets[1].y = rw.get().swapchainExtent().height;
		blit.srcOffsets[1].z = 1;
		blit.dstOffsets[1].x = rw.get().swapchainExtent().width;
		blit.dstOffsets[1].y = rw.get().swapchainExtent().height;
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

		copyHDRCB.blitImage(m_colorAttachmentTexture,
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

	if (vk.queueSubmit(vk.graphicsQueue(), copyHDRCB) != VK_SUCCESS)
	{
		std::cerr << "error: failed to submit draw command buffer"
		          << std::endl;
		abort();
	}
	vk.wait(vk.graphicsQueue());
}
}  // namespace cdm
