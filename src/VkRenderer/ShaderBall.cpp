#include "ShaderBall.hpp"

#include "EquirectangularToCubemap.hpp"
//#include "EquirectangularToIrradianceMap.hpp"
//#include "PrefilterCubemap.hpp"

#include <CompilerSpirV/compileSpirV.hpp>
#include <ShaderWriter/Intrinsics/Intrinsics.hpp>
#include <ShaderWriter/Source.hpp>

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "my_imgui_impl_vulkan.h"

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

class PBRForwardFragmentShaderBallShaderLib
    : public ShaderBallShaderLib<sdw::FragmentWriter>
{
public:
	sdw::Pcb pc;

	sdw::Vec3 fragPosition;
	sdw::Vec2 fragUV;
	sdw::Vec3 fragTanLightPos;
	sdw::Vec3 fragTanViewPos;
	sdw::Vec3 fragTanFragPos;
	sdw::Vec3 fragNormal;
	sdw::Vec3 fragTangent;

	sdw::Function<sdw::Float, sdw::InVec3, sdw::InVec3, sdw::InFloat>
	    DistributionGGX;
	sdw::Function<sdw::Float, sdw::InFloat, sdw::InFloat> GeometrySchlickGGX;
	sdw::Function<sdw::Float, sdw::InVec3, sdw::InVec3, sdw::InVec3,
	              sdw::InFloat>
	    GeometrySmith;
	sdw::Function<sdw::Vec4, sdw::InFloat, sdw::InVec4> fresnelSchlick;
	sdw::Function<sdw::Vec4, sdw::InFloat, sdw::InVec4, sdw::InFloat>
	    fresnelSchlickRoughness;

	sdw::Function<sdw::Vec4, sdw::InVec4, sdw::InFloat, sdw::InFloat,
	              sdw::InVec3>
	    PBRColor;

	PBRForwardFragmentShaderBallShaderLib(ShaderBall::Config const& config)
	    : ShaderBallShaderLib<sdw::FragmentWriter>(config),
	      pc(sdw::Pcb(*this, "pcTextureIndices")),
	      fragPosition(declInput<sdw::Vec3>("fragPosition", 0)),
	      fragUV(declInput<sdw::Vec2>("fragUV", 1)),
	      fragTanLightPos(declInput<sdw::Vec3>("fragTanLightPos", 2)),
	      fragTanViewPos(declInput<sdw::Vec3>("fragTanViewPos", 3)),
	      fragTanFragPos(declInput<sdw::Vec3>("fragTanFragPos", 4)),
	      fragNormal(declInput<sdw::Vec3>("fragNormal", 5)),
	      fragTangent(declInput<sdw::Vec3>("fragTangent", 6))
	{
		using namespace sdw;

		// fragTanLightPos (declInput<Vec3>("fragTanLightPos", 2)),
		// fragTanViewPos  (declInput<Vec3>("fragTanViewPos", 3) ),
		// fragTanFragPos  (declInput<Vec3>("fragTanFragPos", 4) ),
		// fragTangent     (declInput<Vec3>("fragTangent", 6)	  ),

		pc.declMember<UInt>("objectID");
		pc.declMember<UInt>("albedoIndex");
		pc.declMember<UInt>("displacementIndex");
		pc.declMember<UInt>("metalnessIndex");
		pc.declMember<UInt>("normalIndex");
		pc.declMember<UInt>("roughnessIndex");
		pc.declMember<Float>("uShift");
		pc.declMember<Float>("uScale");
		pc.declMember<Float>("vShift");
		pc.declMember<Float>("vScale");
		pc.declMember<Float>("metalnessShift");
		pc.declMember<Float>("metalnessScale");
		pc.declMember<Float>("roughnessShift");
		pc.declMember<Float>("roughnessScale");
		pc.end();

		auto irradianceMap =
		    declSampledImage<FImgCubeRgba32>("irradianceMap", 6, 0);
		auto prefilteredMap =
		    declSampledImage<FImgCubeRgba32>("prefilteredMap", 7, 0);
		auto brdfLut = declSampledImage<FImg2DRg32>("brdfLut", 8, 0);

#define Locale(name, value) auto name = declLocale(#name, value);
#define FLOAT(name) auto name = declLocale<Float>(#name);
#define VEC2(name) auto name = declLocale<Vec2>(#name);
#define VEC3(name) auto name = declLocale<Vec3>(#name);
#define Constant(name, value) auto name = declConstant(#name, value);

		Constant(PI, 3.14159265359_f);

		DistributionGGX = implementFunction<Float>(
		    "DistributionGGX",
		    [&](const Vec3& N, const Vec3& H, const Float& roughness) {
			    Locale(a, roughness * roughness);
			    Locale(a2, a * a);
			    Locale(NdotH, max(dot(N, H), 0.0_f));
			    Locale(NdotH2, NdotH * NdotH);

			    Locale(denom, NdotH2 * (a2 - 1.0_f) + 1.0_f);
			    denom = 3.14159265359_f * denom * denom;

			    returnStmt(a2 / denom);
		    },
		    InVec3{ *this, "N" }, InVec3{ *this, "H" },
		    InFloat{ *this, "roughness" });

		GeometrySchlickGGX = implementFunction<Float>(
		    "GeometrySchlickGGX",
		    [&](const Float& NdotV, const Float& roughness) {
			    Locale(r, roughness + 1.0_f);
			    Locale(k, (r * r) / 8.0_f);

			    Locale(denom, NdotV * (1.0_f - k) + k);

			    returnStmt(NdotV / denom);
		    },
		    InFloat{ *this, "NdotV" }, InFloat{ *this, "roughness" });

		GeometrySmith = implementFunction<Float>(
		    "GeometrySmith",
		    [&](const Vec3& N, const Vec3& V, const Vec3& L,
		        const Float& roughness) {
			    Locale(NdotV, max(dot(N, V), 0.0_f));
			    Locale(NdotL, max(dot(N, L), 0.0_f));
			    Locale(ggx1, GeometrySchlickGGX(NdotV, roughness));
			    Locale(ggx2, GeometrySchlickGGX(NdotL, roughness));

			    returnStmt(ggx1 * ggx2);
		    },
		    InVec3{ *this, "N" }, InVec3{ *this, "V" }, InVec3{ *this, "L" },
		    InFloat{ *this, "roughness" });

		fresnelSchlick = implementFunction<Vec4>(
		    "fresnelSchlick",
		    [&](const Float& cosTheta, const Vec4& F0) {
			    returnStmt(F0 + (vec4(1.0_f) - F0) *
			                        vec4(pow(1.0_f - cosTheta, 5.0_f)));
		    },
		    InFloat{ *this, "cosTheta" }, InVec4{ *this, "F0" });

		fresnelSchlickRoughness = implementFunction<Vec4>(
		    "fresnelSchlickRoughness",
		    [&](const Float& cosTheta, const Vec4& F0,
		        const Float& roughness) {
			    returnStmt(F0 + (max(vec4(1.0_f - roughness), F0) - F0) *
			                        vec4(pow(1.0_f - cosTheta, 5.0_f)));
		    },
		    InFloat{ *this, "cosTheta" }, InVec4{ *this, "F0" },
		    InFloat{ *this, "roughness" });

		PBRColor = implementFunction<Vec4>(
		    "PBRColor",
		    [&](const Vec4& albedo, const Float& metalness,
		        const Float& roughness, const Vec3& N) {
			    Locale(B, cross(fragNormal, fragTangent));
			    Locale(TBN,
			           inverse(transpose(mat3(fragTangent, B, fragNormal))));

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
			    //       4.0_f * max(dot(N, V), 0.0_f) * max(dot(N, L), 0.0_f)
			    //       +
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

			    F = fresnelSchlickRoughness(max(dot(N, V), 0.0_f), F0,
			                                roughness);

			    kS = F;
			    kD = vec4(1.0_f) - kS;
			    kD = kD * vec4(1.0_f - metalness);

			    Locale(R, reflect(TBN * -V, TBN * N));

			    Locale(irradiance, texture(irradianceMap, TBN * N));
			    Locale(diffuse, irradiance * albedo);

			    Locale(MAX_REFLECTION_LOD,
			           cast<Float>(textureQueryLevels(prefilteredMap)));
			    Locale(prefilteredColor,
			           textureLod(prefilteredMap, R,
			                      roughness * MAX_REFLECTION_LOD));
			    Locale(brdf,
			           texture(brdfLut, vec2(max(dot(N, V), 0.0_f), roughness))
			               .rg());
			    specular = prefilteredColor * (F * brdf.x() + brdf.y());

			    Locale(ambient, kD * diffuse + specular);

			    Locale(color, ambient + Lo);

			    returnStmt(color);
		    },
		    InVec4{ *this, "albedo" }, InFloat{ *this, "metalness" },
		    InFloat{ *this, "roughness" }, InVec3{ *this, "N" });
	}
};

void ShaderBallMesh::init()
{
	auto& vk = rw.get()->device();

	CommandBuffer copyCB(vk, rw.get()->oneTimeCommandPool());

#pragma region vertexBuffer
	auto vertexStagingBuffer =
	    Buffer(*rw.get(), sizeof(ShaderBallVertex) * vertices.size(),
	           VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
	           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	vertexBuffer = Buffer(
	    *rw.get(), sizeof(ShaderBallVertex) * vertices.size(),
	    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	ShaderBallVertex* vertexData = vertexStagingBuffer.map<ShaderBallVertex>();
	std::copy(vertices.begin(), vertices.end(), vertexData);
	vertexStagingBuffer.unmap();

	copyCB.reset();
	copyCB.begin();
	copyCB.copyBuffer(vertexStagingBuffer.get(), vertexBuffer.get(),
	                  sizeof(ShaderBallVertex) * vertices.size());
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

void ShaderBallMesh::draw(CommandBuffer& cb)
{
	cb.bindVertexBuffer(vertexBuffer.get());
	cb.bindIndexBuffer(indexBuffer.get(), 0, VK_INDEX_TYPE_UINT32);
	cb.drawIndexed(uint32_t(indices.size()));
}

static void processMesh(RenderWindow& rw, aiMesh* mesh, const aiScene* scene,
                        std::vector<ShaderBallMesh>& meshes)
{
	ShaderBallMesh res;
	res.rw = &rw;

	res.vertices.reserve(mesh->mNumVertices);
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		ShaderBallVertex vertex;
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
                        std::vector<ShaderBallMesh>& meshes)
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
      m_shadingModel(rw),
      m_defaultMaterial(rw, m_shadingModel, 2),
      m_scene(renderWindow),
      imguiCB(CommandBuffer(rw.get().device(), rw.get().oneTimeCommandPool())),
      copyHDRCB(
          CommandBuffer(rw.get().device(), rw.get().oneTimeCommandPool()))
{
	auto& vk = rw.get().device();

	vk.setLogActive();

	Assimp::Importer importer;

#pragma region bunny mesh
	const aiScene* bunnyScene = importer.ReadFile(
	    "D:/Projects/git/VkRenderer-data/bunny.obj",
	    aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals |
	        aiProcess_CalcTangentSpace);

	if (!bunnyScene || bunnyScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
	    !bunnyScene->mRootNode)
		throw std::runtime_error(std::string("assimp error: ") +
		                         importer.GetErrorString());

	assert(bunnyScene->mNumMeshes == 1);

	{
		auto& mesh = bunnyScene->mMeshes[0];
		std::vector<StandardMesh::Vertex> vertices;
		vertices.reserve(mesh->mNumVertices);
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			StandardMesh::Vertex vertex;
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

			vertices.push_back(vertex);
		}

		std::vector<uint32_t> indices;
		indices.reserve(mesh->mNumFaces * 3);
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			const aiFace& face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

		m_bunnyMesh =
		    StandardMesh(rw, std::move(vertices), std::move(indices));
	}
#pragma endregion

#pragma region assimp
	// const aiScene* scene = importer.ReadFile(
	//    "D:/Projects/git/VkRenderer-data/ShaderBall.fbx",
	//    aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals |
	//        aiProcess_CalcTangentSpace);

	// if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
	//    !scene->mRootNode)
	//	throw std::runtime_error(std::string("assimp error: ") +
	//	                         importer.GetErrorString());

	// processNode(rw, scene->mRootNode, scene, m_meshes);

	// for (uint32_t i = 0; i < m_meshes.size(); i++)
	//{
	//	auto& mesh = m_meshes[i];
	//	mesh.materialData.objectID = i;
	//	mesh.materialData.textureIndices = { i, i, i, i, i };
	//}
#pragma endregion

#pragma region renderPass
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
	colorAttachment.samples = VK_SAMPLE_COUNT_4_BIT;
	//colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription objectIDAttachment = {};
	objectIDAttachment.format = VK_FORMAT_R32_UINT;
	objectIDAttachment.samples = VK_SAMPLE_COUNT_4_BIT;
	objectIDAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	objectIDAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	objectIDAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	objectIDAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	objectIDAttachment.initialLayout =
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	objectIDAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = rw.get().depthImageFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_4_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout =
	    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.finalLayout =
	    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorResolveAttachment = {};
	colorResolveAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
	colorResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	//colorResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorResolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorResolveAttachment.initialLayout =
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorResolveAttachment.finalLayout =
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription objectIDResolveAttachment = {};
	objectIDResolveAttachment.format = VK_FORMAT_R32_UINT;
	objectIDResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	objectIDResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	objectIDResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	objectIDResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	objectIDResolveAttachment.stencilStoreOp =
	    VK_ATTACHMENT_STORE_OP_DONT_CARE;
	objectIDResolveAttachment.initialLayout =
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	objectIDResolveAttachment.finalLayout =
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::array colorAttachments{ colorAttachment, objectIDAttachment,
		                         depthAttachment, colorResolveAttachment,
		                         objectIDResolveAttachment };

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference objectIDAttachmentRef = {};
	objectIDAttachmentRef.attachment = 1;
	objectIDAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 2;
	depthAttachmentRef.layout =
	    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorResolveAttachmentRef = {};
	colorResolveAttachmentRef.attachment = 3;
	colorResolveAttachmentRef.layout =
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference objectIDResolveAttachmentRef = {};
	objectIDResolveAttachmentRef.attachment = 4;
	objectIDResolveAttachmentRef.layout =
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::array colorAttachmentRefs{ colorAttachmentRef  //, depthAttachmentRef
		                            ,
		                            objectIDAttachmentRef };

	std::array resolveAttachmentRefs{ colorResolveAttachmentRef,
		                              objectIDResolveAttachmentRef };

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = uint32_t(colorAttachmentRefs.size());
	subpass.pColorAttachments = colorAttachmentRefs.data();
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.pResolveAttachments = resolveAttachmentRefs.data();
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

#pragma region hightlight render
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout =
		    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		std::array colorAttachments{ colorAttachment };

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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
		subpass.pDepthStencilAttachment = nullptr;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = nullptr;

		VkSubpassDependency dependency = {};
		dependency.dependencyFlags = 0;
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask =
		    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask =
		    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
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

		m_highlightRenderPass = vk.create(renderPassInfo);
		if (!m_highlightRenderPass)
		{
			std::cerr << "error: failed to create hightlight render pass"
			          << std::endl;
			abort();
		}
	}
#pragma endregion

#pragma region descriptor pool
	std::array poolSizes{
		// VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 128 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
	};

	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.maxSets = 2;
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
	{
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
		layoutBindingDisplacementImages.stageFlags =
		    VK_SHADER_STAGE_FRAGMENT_BIT;

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
		layoutBindingIrradianceMapImages.stageFlags =
		    VK_SHADER_STAGE_FRAGMENT_BIT;

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
	pcRange.size = sizeof(ShaderBallMesh::MaterialData);
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

#pragma region highlight descriptor set layout
	{
		VkDescriptorSetLayoutBinding layoutBindingHighlightInputColor{};
		layoutBindingHighlightInputColor.binding = 0;
		layoutBindingHighlightInputColor.descriptorCount = 1;
		layoutBindingHighlightInputColor.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindingHighlightInputColor.stageFlags =
		    VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding layoutBindingHighlightInputID{};
		layoutBindingHighlightInputID.binding = 1;
		layoutBindingHighlightInputID.descriptorCount = 1;
		layoutBindingHighlightInputID.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindingHighlightInputID.stageFlags =
		    VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array highlightLayoutBindings{ layoutBindingHighlightInputColor,
			                                layoutBindingHighlightInputID };

		vk::DescriptorSetLayoutCreateInfo highlightSetLayoutInfo;
		highlightSetLayoutInfo.bindingCount =
		    uint32_t(highlightLayoutBindings.size());
		highlightSetLayoutInfo.pBindings = highlightLayoutBindings.data();

		m_highlightDescriptorSetLayout = vk.create(highlightSetLayoutInfo);
		if (!m_highlightDescriptorSetLayout)
		{
			std::cerr
			    << "error: failed to create highlight descriptor set layout"
			    << std::endl;
			abort();
		}
	}
#pragma endregion

#pragma region highlight descriptor set
	m_highlightDescriptorSet =
	    vk.allocate(m_descriptorPool, m_highlightDescriptorSetLayout);

	if (!m_highlightDescriptorSet)
	{
		std::cerr << "error: failed to allocate highlight descriptor set"
		          << std::endl;
		abort();
	}
#pragma endregion

#pragma region highlight pipeline layout
	VkPushConstantRange highlightPcRange{};
	highlightPcRange.size = sizeof(uint32_t);
	highlightPcRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	// vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_highlightDescriptorSetLayout.get();
	// pipelineLayoutInfo.setLayoutCount = 0;
	// pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &highlightPcRange;
	// pipelineLayoutInfo.pushConstantRangeCount = 0;
	// pipelineLayoutInfo.pPushConstantRanges = nullptr;

	m_highlightPipelineLayout = vk.create(pipelineLayoutInfo);
	if (!m_highlightPipelineLayout)
	{
		std::cerr << "error: failed to create highlight pipeline layout"
		          << std::endl;
		abort();
	}
#pragma endregion

#pragma region UBO
	/*
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
	//*/
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
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

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
			    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			        VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			    -1);

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

			arr[element].generateMipmapsImmediate(
			    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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
	// resourcePath += "Metal007_4K-JPG/";
	// resourcePath += "Marble009_8K-JPG/";
	resourcePath += "Leather011_8K-JPG/";
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
	// m_meshes[9].materialData.uScale = 20.0f;
	// m_meshes[9].materialData.vScale = 20.0f;
	// m_meshes[2].materialData.uScale = 2.0f;
	// m_meshes[2].materialData.vScale = 2.0f;
	// m_meshes[2].materialData.uShift = 0.5f;
	// m_meshes[2].materialData.vShift = 0.5f;
#pragma endregion

#pragma region bunny
	m_bunnySceneObject = &m_scene.instantiateSceneObject();
	m_bunnySceneObject->setMesh(m_bunnyMesh);
	m_bunnySceneObject->setMaterial(m_defaultMaterial);

	m_materialInstance1 = m_defaultMaterial.instanciate();
	m_materialInstance1->setFloatParameter("roughness", 0.5f);
	m_materialInstance1->setFloatParameter("metalness", 0.0f);
	m_materialInstance1->setVec4Parameter("color", vector4(1, 0, 0, 1));

	m_materialInstance2 = m_defaultMaterial.instanciate();
	m_materialInstance2->setFloatParameter("roughness", 0.001f);
	m_materialInstance2->setFloatParameter("metalness", 1.0f);
	m_materialInstance2->setVec4Parameter("color", vector4(0, 0, 1, 1));

	m_bunnySceneObject2 = &m_scene.instantiateSceneObject();
	m_bunnySceneObject2->setMesh(m_bunnyMesh);
	m_bunnySceneObject2->setMaterial(*m_materialInstance1);

	m_bunnySceneObject3 = &m_scene.instantiateSceneObject();
	m_bunnySceneObject3->setMesh(m_bunnyMesh);
	m_bunnySceneObject3->setMaterial(*m_materialInstance2);

	{
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
		irradianceMapTextureWrite.dstBinding = 0;
		irradianceMapTextureWrite.dstSet = m_shadingModel.m_descriptorSet;
		irradianceMapTextureWrite.pImageInfo = &irradianceMapImageInfo;

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
		prefilteredMapTextureWrite.dstBinding = 1;
		prefilteredMapTextureWrite.dstSet = m_shadingModel.m_descriptorSet;
		prefilteredMapTextureWrite.pImageInfo = &prefilteredMapImageInfo;

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
		brdfLutTextureWrite.dstBinding = 2;
		brdfLutTextureWrite.dstSet = m_shadingModel.m_descriptorSet;
		brdfLutTextureWrite.pImageInfo = &brdfLutImageInfo;

		vk.updateDescriptorSets({ irradianceMapTextureWrite,
		                          prefilteredMapTextureWrite,
		                          brdfLutTextureWrite });
	}
#pragma endregion
}

ShaderBall::~ShaderBall() {}

void ShaderBall::renderOpaque(CommandBuffer& cb)
{
	{
		VkClearValue clearColor{};
		clearColor.color.float32[0] = 0X27 / 255.0f;
		clearColor.color.float32[1] = 0X28 / 255.0f;
		clearColor.color.float32[2] = 0X22 / 255.0f;

		VkClearValue clearID{};
		clearID.color.uint32[0] = -1u;
		clearID.color.uint32[1] = -1u;
		clearID.color.uint32[2] = -1u;
		clearID.color.uint32[3] = -1u;

		VkClearValue clearDepth{};
		clearDepth.depthStencil.depth = 1.0f;

		std::array clearValues = { clearColor, clearID, clearDepth, clearColor, clearID };

		vk::RenderPassBeginInfo rpInfo;
		rpInfo.framebuffer = m_framebuffer;
		rpInfo.renderPass = m_renderPass;
		rpInfo.renderArea.extent.width = rw.get().swapchainExtent().width;
		rpInfo.renderArea.extent.height = rw.get().swapchainExtent().height;
		rpInfo.clearValueCount = uint32_t(clearValues.size());
		rpInfo.pClearValues = clearValues.data();

		vk::SubpassBeginInfo subpassBeginInfo;
		subpassBeginInfo.contents = VK_SUBPASS_CONTENTS_INLINE;

		vk::SubpassEndInfo subpassEndInfo;

		cb.beginRenderPass2(rpInfo, subpassBeginInfo);

		m_skybox->render(cb);

		cb.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
		cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
		                     0, m_descriptorSet);

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = float(m_colorAttachmentTexture.width());
		viewport.height = float(m_colorAttachmentTexture.height());
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		// cb.setViewport(viewport);

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent.width = m_colorAttachmentTexture.width();
		scissor.extent.height = m_colorAttachmentTexture.height();
		// cb.setScissor(scissor);

		// m_bunnyPipeline.bindDescriptorSet(cb);
		// m_bunnyPipeline.draw(cb);

		m_bunnySceneObject->draw(cb, m_renderPass, viewport, scissor);
		m_bunnySceneObject2->draw(cb, m_renderPass, viewport, scissor);
		m_bunnySceneObject3->draw(cb, m_renderPass, viewport, scissor);

		cb.endRenderPass2(subpassEndInfo);
	}
	{
		vk::ImageMemoryBarrier barrier;
		barrier.image = m_colorResolveTexture;
		barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		cb.pipelineBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		                   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, barrier);

		barrier.image = m_objectIDResolveTexture;
		cb.pipelineBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		                   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, barrier);
		// VkClearValue clearColor{};
		// clearColor.color.float32[0] = 0X27 / 255.0f;
		// clearColor.color.float32[1] = 0X28 / 255.0f;
		// clearColor.color.float32[2] = 0X22 / 255.0f;

		// VkClearValue clearID{};
		// clearID.color.uint32[0] = -1u;
		// clearID.color.uint32[1] = -1u;
		// clearID.color.uint32[2] = -1u;
		// clearID.color.uint32[3] = -1u;

		// VkClearValue clearDepth{};
		// clearDepth.depthStencil.depth = 1.0f;

		// std::array clearValues = { clearColor, clearID, clearDepth };

		vk::RenderPassBeginInfo rpInfo;
		rpInfo.framebuffer = m_highlightFramebuffer;
		rpInfo.renderPass = m_highlightRenderPass;
		rpInfo.renderArea.extent.width = rw.get().swapchainExtent().width;
		rpInfo.renderArea.extent.height = rw.get().swapchainExtent().height;
		rpInfo.clearValueCount = 0;  // uint32_t(clearValues.size());
		// rpInfo.pClearValues = clearValues.data();

		vk::SubpassBeginInfo subpassBeginInfo;
		subpassBeginInfo.contents = VK_SUBPASS_CONTENTS_INLINE;

		vk::SubpassEndInfo subpassEndInfo;

		cb.beginRenderPass2(rpInfo, subpassBeginInfo);

		cb.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_highlightPipeline);
		cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS,
		                     m_highlightPipelineLayout, 0,
		                     m_highlightDescriptorSet);

		cb.pushConstants(m_highlightPipelineLayout,
		                 VK_SHADER_STAGE_FRAGMENT_BIT, 0, &m_highlightID);

		cb.draw(3);

		cb.endRenderPass2(subpassEndInfo);

		barrier.image = m_colorResolveTexture;
		barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = 0;
		cb.pipelineBarrier(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, barrier);

		barrier.image = m_objectIDResolveTexture;
		cb.pipelineBarrier(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, barrier);
	}
}

void ShaderBall::imgui(CommandBuffer& cb)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	{
		static float f = 0.0f;
		static int counter = 0;

		/*
		if (m_lastSelectedHighlightID < m_meshes.size() &&
		    m_showMaterialWindow)
		{
			ImGui::Begin("Material", &m_showMaterialWindow);

			auto& m = m_meshes[m_lastSelectedHighlightID].materialData;

			bool changed = false;

			changed |= ImGui::DragFloat("u scale", &m.uScale, 0.01f);
			changed |= ImGui::DragFloat("v scale", &m.vScale, 0.01f);

			changed |= ImGui::DragFloat("u shift", &m.uShift, 0.01f);
			changed |= ImGui::DragFloat("v shift", &m.vShift, 0.01f);

			changed |=
			    ImGui::DragFloat("metalness scale", &m.metalnessScale, 0.01f);
			changed |= ImGui::DragFloat("metalness shift", &m.metalnessShift,
			                            0.001f, 0.0f, 1.0f);

			changed |=
			    ImGui::DragFloat("roughness scale", &m.roughnessScale, 0.01f);
			changed |= ImGui::DragFloat("roughness shift", &m.roughnessShift,
			                            0.001f, 0.001f, 0.999f);

			static uint32_t pmin = 0;
			static uint32_t pmax = 15;

			changed |=
			    ImGui::DragScalar("albedo index", ImGuiDataType_U32,
			                      &m.textureIndices[0], 0.1f, &pmin, &pmax);
			changed |=
			    ImGui::DragScalar("displacement index", ImGuiDataType_U32,
			                      &m.textureIndices[1], 0.1f, &pmin, &pmax);
			changed |=
			    ImGui::DragScalar("metal index", ImGuiDataType_U32,
			                      &m.textureIndices[2], 0.1f, &pmin, &pmax);
			changed |=
			    ImGui::DragScalar("normal index", ImGuiDataType_U32,
			                      &m.textureIndices[3], 0.1f, &pmin, &pmax);
			changed |=
			    ImGui::DragScalar("roughness index", ImGuiDataType_U32,
			                      &m.textureIndices[4], 0.1f, &pmin, &pmax);

			// changed |= ImGui::SliderFloat("CamFocalDistance",
			//	&m_config.camFocalDistance, 0.1f, 30.0f);
			// changed |= ImGui::SliderFloat("CamFocalLength",
			//	&m_config.camFocalLength, 0.0f, 20.0f);
			// changed |= ImGui::SliderFloat("CamAperture",
			// &m_config.camAperture, 	0.0f, 5.0f);

			// changed |= ImGui::DragFloat3("rotation", &m_config.camRot.x,
			// 0.01f); changed |= ImGui::DragFloat3("lightDir",
			// &m_config.lightDir.x, 0.01f);

			// changed |= ImGui::SliderFloat("scene radius",
			// &m_config.sceneRadius, 	0.0f, 10.0f);

			// ImGui::SliderFloat("BloomAscale1", &m_config.bloomAscale1,
			// 0.0f, 1.0f); ImGui::SliderFloat("BloomAscale2",
			// &m_config.bloomAscale2, 0.0f, 1.0f);
			// ImGui::SliderFloat("BloomBscale1", &m_config.bloomBscale1,
			// 0.0f, 1.0f); ImGui::SliderFloat("BloomBscale2",
			// &m_config.bloomBscale2, 0.0f, 1.0f);

			// if (changed)
			//	applyImguiParameters();

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
			            1000.0f / ImGui::GetIO().Framerate,
			            ImGui::GetIO().Framerate);
			ImGui::End();
		}
		//*/
	}

	ImGui::Render();

	 vk::ImageMemoryBarrier barrier;
	 barrier.image = m_highlightColorAttachmentTexture;
	 barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	 barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	 barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	 barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	 barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	 barrier.subresourceRange.baseMipLevel = 0;
	 barrier.subresourceRange.levelCount = 1;
	 barrier.subresourceRange.baseArrayLayer = 0;
	 barrier.subresourceRange.layerCount = 1;
	 barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	 barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	 cb.pipelineBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
	                   barrier);

	{
		std::array clearValues = { VkClearValue{}, VkClearValue{} };

		vk::RenderPassBeginInfo rpInfo;
		rpInfo.framebuffer = m_highlightFramebuffer.get();
		rpInfo.renderPass = rw.get().imguiRenderPass();
		rpInfo.renderArea.extent.width = rw.get().swapchainExtent().width;
		rpInfo.renderArea.extent.height = rw.get().swapchainExtent().height;
		rpInfo.clearValueCount = 1;
		rpInfo.pClearValues = clearValues.data();

		vk::SubpassBeginInfo subpassBeginInfo;
		subpassBeginInfo.contents = VK_SUBPASS_CONTENTS_INLINE;

		cb.beginRenderPass2(rpInfo, subpassBeginInfo);
	}

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb.get());

	vk::SubpassEndInfo subpassEndInfo2;
	cb.endRenderPass2(subpassEndInfo2);

	 barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	 barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	 cb.pipelineBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
	                   barrier);
}

void ShaderBall::standaloneDraw()
{
	auto& vk = rw.get().device();

	// vk.wait(vk.graphicsQueue());

	if (rw.get().swapchainExtent().width == 0 ||
	    rw.get().swapchainExtent().height == 0)
		return;

	if (!ImGui::IsAnyWindowHovered() &&
	    rw.get().mouseState(MouseButton::Left, ButtonState::Pressed))
	{
		auto ids = m_objectIDResolveTexture.downloadDataImmediate<uint32_t>(
		    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		auto [x, y] = rw.get().mousePos();

		size_t mouseX = size_t(x);
		size_t mouseY = size_t(y);

		size_t i = mouseX + mouseY * m_objectIDResolveTexture.width();

		if (i < ids.size())
			m_highlightID = ids[i];

		if (m_highlightID != 4294967294)
		{
			m_lastSelectedHighlightID = m_highlightID;
			m_showMaterialWindow = true;
		}
	}

	if (mustRebuild())
		rebuild();

	m_config.model = matrix4(modelTr).get_transposed();
	m_config.view = matrix4(cameraTr).get_transposed().get_inversed();
	m_config.proj =
	    matrix4::perspective(90_deg,
	                         float(rw.get().swapchainExtent().width) /
	                             float(rw.get().swapchainExtent().height),
	                         0.01f, 100.0f)
	        .get_transposed();
	m_config.viewPos = cameraTr.position;

	//m_config.copyTo(m_matricesUBO.map());
	//m_matricesUBO.unmap();

	m_bunnySceneObject->transform.position = { 0, 0, -20 };
	m_bunnySceneObject->transform.scale = { 6, 6, 6 };
	m_bunnySceneObject2->transform.position = { 20, 0, -20 };
	m_bunnySceneObject2->transform.scale = { 6, 6, 6 };
	m_bunnySceneObject3->transform.position = { -20, 0, -20 };
	m_bunnySceneObject3->transform.scale = { 6, 6, 6 };

	struct alignas(16) SceneUboStruct
	{
		matrix4 view;
		matrix4 proj;
		vector3 viewPos;
		vector3 lightPos;
	};

	struct alignas(16) ModelUboStruct
	{
		std::array<matrix4, Scene::MaxSceneObjectCountPerPool> model;
	};

	SceneUboStruct* sceneUBOPtr =
	    m_scene.sceneUniformBuffer().map<SceneUboStruct>();
	sceneUBOPtr->lightPos = { 0, 0, 0 };
	sceneUBOPtr->view = matrix4(cameraTr).get_transposed().get_inversed();
	sceneUBOPtr->proj = m_config.proj;
	sceneUBOPtr->viewPos = cameraTr.position;
	m_scene.sceneUniformBuffer().unmap();

	ModelUboStruct* modelUBOPtr =
	    m_scene.modelUniformBuffer().map<ModelUboStruct>();
	modelUBOPtr->model[0] =
	    matrix4(m_bunnySceneObject->transform).get_transposed();
	modelUBOPtr->model[1] =
	    matrix4(m_bunnySceneObject2->transform).get_transposed();
	modelUBOPtr->model[2] =
	    matrix4(m_bunnySceneObject3->transform).get_transposed();
	m_scene.modelUniformBuffer().unmap();

	m_skybox->setMatrices(m_config.proj, m_config.view);

	auto& frame = rw.get().getAvailableCommandBuffer();
	frame.reset();
	// vk.resetFence(frame.fence);
	auto& cb = frame.commandBuffer;

	// cb.reset();
	cb.begin();
	cb.debugMarkerBegin("render", 0.2f, 0.2f, 1.0f);
	renderOpaque(cb);
	imgui(cb);
	cb.debugMarkerEnd();
	cb.end();

	// vk::SubmitInfo drawSubmit;
	// drawSubmit.commandBufferCount = 1;
	// drawSubmit.pCommandBuffers = &imguiCB.get();
	// drawSubmit.signalSemaphoreCount = 1;
	// drawSubmit.pSignalSemaphores =
	// &rw.get().currentImageAvailableSemaphore();

	////if (vk.queueSubmit(vk.graphicsQueue(), imguiCB) != VK_SUCCESS)
	// if (vk.queueSubmit(vk.graphicsQueue(), drawSubmit) != VK_SUCCESS)

	if (vk.queueSubmit(vk.graphicsQueue(), cb, frame.semaphore, frame.fence) !=
	    VK_SUCCESS)
	{
		std::cerr << "error: failed to submit ShaderBall command buffer"
		          << std::endl;
		abort();
	}
	frame.submitted = true;

	// vk.wait(vk.graphicsQueue());

	// vk.wait();

	rw.get().present(m_highlightColorAttachmentTexture,
	                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	                 frame.semaphore);
}

bool ShaderBall::mustRebuild() const
{
	return rw.get().swapchainCreationTime() > m_creationTime;
}

void ShaderBall::rebuild()
{
	auto& vk = rw.get().device();

#pragma region color attachment texture
	m_colorAttachmentTexture = Texture2D(
	    rw, rw.get().swapchainExtent().width,
	    rw.get().swapchainExtent().height, VK_FORMAT_B8G8R8A8_UNORM,
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
	        // VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1,
	    VK_FILTER_LINEAR, VK_SAMPLE_COUNT_4_BIT);

	vk.debugMarkerSetObjectName(m_colorAttachmentTexture.image(),
	                            "colorAttachmentTexture");

	m_colorAttachmentTexture.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	{
		// VkDescriptorImageInfo imageInfo{};
		// imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		// imageInfo.imageView = m_colorAttachmentTexture.view();
		// imageInfo.sampler = m_colorAttachmentTexture.sampler();
		//
		// vk::WriteDescriptorSet textureWrite;
		// textureWrite.descriptorCount = 1;
		// textureWrite.descriptorType =
		//    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		// textureWrite.dstArrayElement = 0;
		// textureWrite.dstBinding = 0;
		// textureWrite.dstSet = m_highlightDescriptorSet;
		// textureWrite.pImageInfo = &imageInfo;
		//
		// vk.updateDescriptorSets(textureWrite);
	}
#pragma endregion

#pragma region highlight color attachment texture
	m_highlightColorAttachmentTexture = Texture2D(
	    rw, rw.get().swapchainExtent().width,
	    rw.get().swapchainExtent().height, VK_FORMAT_B8G8R8A8_UNORM,
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
	        // VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vk.debugMarkerSetObjectName(m_highlightColorAttachmentTexture.image(),
	                            "highlightColorAttachmentTexture");

	m_highlightColorAttachmentTexture.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
#pragma endregion

#pragma region object ID attachment texture
	m_objectIDAttachmentTexture = Texture2D(
	    rw, rw.get().swapchainExtent().width,
	    rw.get().swapchainExtent().height, VK_FORMAT_R32_UINT,
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
	        // VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1,
	    VK_FILTER_NEAREST, VK_SAMPLE_COUNT_4_BIT);

	vk.debugMarkerSetObjectName(m_objectIDAttachmentTexture.image(),
	                            "objectIDAttachmentTexture");

	m_objectIDAttachmentTexture.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	//{
	//	VkDescriptorImageInfo imageInfo{};
	//	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	//	imageInfo.imageView = m_objectIDAttachmentTexture.view();
	//	imageInfo.sampler = m_objectIDAttachmentTexture.sampler();

	//	vk::WriteDescriptorSet textureWrite;
	//	textureWrite.descriptorCount = 1;
	//	textureWrite.descriptorType =
	//	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	//	textureWrite.dstArrayElement = 0;
	//	textureWrite.dstBinding = 1;
	//	textureWrite.dstSet = m_highlightDescriptorSet;
	//	textureWrite.pImageInfo = &imageInfo;

	//	vk.updateDescriptorSets(textureWrite);
	//}
#pragma endregion

#pragma region object ID resolve texture
	m_objectIDResolveTexture = Texture2D(
	    rw, rw.get().swapchainExtent().width,
	    rw.get().swapchainExtent().height, VK_FORMAT_R32_UINT,
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
	        // VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1,
	    VK_FILTER_NEAREST, VK_SAMPLE_COUNT_1_BIT);

	vk.debugMarkerSetObjectName(m_objectIDResolveTexture.image(),
	                            "objectIDResolveTexture");

	m_objectIDResolveTexture.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_objectIDResolveTexture.view();
		imageInfo.sampler = m_objectIDResolveTexture.sampler();

		vk::WriteDescriptorSet textureWrite;
		textureWrite.descriptorCount = 1;
		textureWrite.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textureWrite.dstArrayElement = 0;
		textureWrite.dstBinding = 1;
		textureWrite.dstSet = m_highlightDescriptorSet;
		textureWrite.pImageInfo = &imageInfo;

		vk.updateDescriptorSets(textureWrite);
	}
#pragma endregion

#pragma region depth texture
	m_depthTexture = DepthTexture(
	    rw, rw.get().swapchainExtent().width,
	    rw.get().swapchainExtent().height, rw.get().depthImageFormat(),
	    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1,
	    VK_SAMPLE_COUNT_4_BIT);

	vk.debugMarkerSetObjectName(m_depthTexture.image(), "depthTexture");

	m_depthTexture.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_UNDEFINED,
	    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
#pragma endregion

#pragma region color resolve texture
	m_colorResolveTexture = Texture2D(
	    rw, rw.get().swapchainExtent().width,
	    rw.get().swapchainExtent().height, VK_FORMAT_B8G8R8A8_UNORM,
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
	        // VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1,
	    VK_FILTER_LINEAR, VK_SAMPLE_COUNT_1_BIT);

	vk.debugMarkerSetObjectName(m_colorResolveTexture.image(),
	                            "colorResolveTexture");

	m_colorResolveTexture.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_colorResolveTexture.view();
		imageInfo.sampler = m_colorResolveTexture.sampler();

		vk::WriteDescriptorSet textureWrite;
		textureWrite.descriptorCount = 1;
		textureWrite.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textureWrite.dstArrayElement = 0;
		textureWrite.dstBinding = 0;
		textureWrite.dstSet = m_highlightDescriptorSet;
		textureWrite.pImageInfo = &imageInfo;

		vk.updateDescriptorSets(textureWrite);
	}
#pragma endregion

#pragma region framebuffer
	{
		vk::FramebufferCreateInfo framebufferInfo;
		framebufferInfo.renderPass = m_renderPass;
		std::array attachments = {
			m_colorAttachmentTexture.view(),
			m_objectIDAttachmentTexture.view(),
			m_depthTexture.view(),
			m_colorResolveTexture.view(),
			m_objectIDResolveTexture.view(),
		};
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
	}
#pragma endregion

#pragma region highlight framebuffer
	{
		vk::FramebufferCreateInfo framebufferInfo;
		framebufferInfo.renderPass = m_highlightRenderPass;
		std::array attachments = { m_highlightColorAttachmentTexture.view() };
		framebufferInfo.attachmentCount = uint32_t(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_colorAttachmentTexture.width();
		framebufferInfo.height = m_colorAttachmentTexture.height();
		framebufferInfo.layers = 1;

		m_highlightFramebuffer = vk.create(framebufferInfo);
		if (!m_highlightFramebuffer)
		{
			std::cerr << "error: failed to create highlight framebuffer"
			          << std::endl;
			abort();
		}
	}
#pragma endregion

#undef Locale
#undef FLOAT
#undef VEC2
#undef VEC3
#undef Constant

#define Locale(name, value) auto name = writer.declLocale(#name, value);
#define FLOAT(name) auto name = writer.declLocale<Float>(#name);
#define VEC2(name) auto name = writer.declLocale<Vec2>(#name);
#define VEC3(name) auto name = writer.declLocale<Vec3>(#name);
#define Constant(name, value) auto name = writer.declConstant(#name, value);

#pragma region vertexShader
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
	{
		using namespace sdw;
		PBRForwardFragmentShaderBallShaderLib writer{ m_config };

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

		auto fragColor = writer.declOutput<Vec4>("fragColor", 0);
		auto fragID = writer.declOutput<UInt>("fragID", 1);

		auto toneMapping = writer.implementFunction<Vec4>(
		    "toneMapping",
		    [&](const Vec4& inColor) {
			    writer.returnStmt(inColor / (inColor + vec4(1.0_f)));
		    },
		    InVec4(writer, "inColor"));

		auto gammaCorrection = writer.implementFunction<Vec4>(
		    "gammaCorrection",
		    [&](const Vec4& inColor) {
			    writer.returnStmt(pow(inColor, vec4(1.0_f / 2.2_f)));
		    },
		    InVec4(writer, "inColor"));

		/// AAAAAAAAAAAAAAAH
		// auto parallaxMapping =
		// writer.implementFunction<Vec2>("parallaxMapping", [&](const Vec2& )

		writer.implementMain([&]() {
			auto& pc = writer.pc;
			Locale(scaledUV,
			       (writer.fragUV * vec2(pc.getMember<Float>("uScale"),
			                             pc.getMember<Float>("vScale")) +
			        vec2(pc.getMember<Float>("uShift"),
			             pc.getMember<Float>("vShift"))));

			// Locale(
			//    albedo,
			//    pow(textureLod(albedos[pc.getMember<UInt>("albedoIndex")],
			//                   scaledUV,
			//                   textureQueryLod(
			//                       albedos[pc.getMember<UInt>("albedoIndex")],
			//                       scaledUV)
			//                       .y()),
			//        vec4(2.2_f)));
			// Locale(metalness,
			//       texture(metalnesses[pc.getMember<UInt>("metalnessIndex")],
			//               scaledUV)
			//                   .r() *
			//               pc.getMember<Float>("metalnessScale") +
			//           pc.getMember<Float>("metalnessShift"));
			// Locale(roughness,
			//       texture(roughnesses[pc.getMember<UInt>("roughnessIndex")],
			//               scaledUV)
			//                   .r() *
			//               pc.getMember<Float>("roughnessScale") +
			//           pc.getMember<Float>("roughnessShift"));
			//
			// Locale(N, textureLod(
			//              normals[pc.getMember<UInt>("normalIndex")],
			//              scaledUV, textureQueryLod(
			//                  albedos[pc.getMember<UInt>("normalIndex")],
			//                  scaledUV)
			//                  .y())
			//              .xyz());

			Locale(albedo,
			       pow(texture(albedos[pc.getMember<UInt>("albedoIndex")],
			                   scaledUV),
			           vec4(2.2_f)));
			Locale(metalness,
			       texture(metalnesses[pc.getMember<UInt>("metalnessIndex")],
			               scaledUV)
			                   .r() *
			               pc.getMember<Float>("metalnessScale") +
			           pc.getMember<Float>("metalnessShift"));
			Locale(roughness,
			       texture(roughnesses[pc.getMember<UInt>("roughnessIndex")],
			               scaledUV)
			                   .r() *
			               pc.getMember<Float>("roughnessScale") +
			           pc.getMember<Float>("roughnessShift"));

			Locale(N, texture(normals[pc.getMember<UInt>("normalIndex")],
			                  scaledUV)
			              .xyz());
			N = normalize((N * vec3(2.0_f) - vec3(1.0_f)));

			Locale(color, writer.PBRColor(albedo, metalness, roughness, N));

			color = toneMapping(color);

			color = gammaCorrection(color);

			fragColor = color;
			fragID = pc.getMember<UInt>("objectID");
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
	binding.stride = sizeof(ShaderBallVertex);

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
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_4_BIT;
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

#pragma region highlight vertexShader
	{
		using namespace sdw;
		VertexWriter writer;

		auto in = writer.getIn();
		auto out = writer.getOut();

		// Screen triangle
		// https://discordapp.com/channels/639982952503181322/640089542736609280/678008085960327173
		writer.implementMain([&]() {
			out.vtx.position = vec4(
			    vec2((in.vertexIndex << 1) & 2, in.vertexIndex & 2) * 2.0f -
			        1.0f,
			    0.0f, 1.0f);
		});

		std::vector<uint32_t> bytecode =
		    spirv::serialiseSpirv(writer.getShader());

		vk::ShaderModuleCreateInfo createInfo;
		createInfo.codeSize = bytecode.size() * sizeof(*bytecode.data());
		createInfo.pCode = bytecode.data();

		m_highlightVertexModule = vk.create(createInfo);
		if (!m_highlightVertexModule)
		{
			std::cerr
			    << "error: failed to create highlight vertex shader module"
			    << std::endl;
			abort();
		}
	}
#pragma endregion

#pragma region highlight fragmentShader
	{
		using namespace sdw;
		FragmentWriter writer;

		auto in = writer.getIn();

		auto sceneColorTexture =
		    writer.declSampledImage<FImg2DRgba32>("sceneColorTexture", 0, 0);
		auto sceneIDTexture =
		    writer.declSampledImage<UImg2DR32>("sceneIDTexture", 1, 0);

		auto fragColor = writer.declOutput<Vec4>("fragColor", 0);

		Pcb pc(writer, "pc");
		pc.declMember<UInt>("id");
		pc.end();

		writer.implementMain([&]() {
			Locale(uv, in.fragCoord.xy() /
			               vec2(Float(rw.get().swapchainExtent().width),
			                    Float(rw.get().swapchainExtent().height)));
			Locale(sceneColor, texture(sceneColorTexture, uv));
			Locale(sceneID, texture(sceneIDTexture, uv));

			Locale(id, pc.getMember<UInt>("id"));

			IF(writer, sceneID == id || id == 4294967294_u)
			{
				fragColor = sceneColor;
			}
			ELSE { fragColor = sceneColor / 10.0_f; }
			FI;
		});

		std::vector<uint32_t> bytecode =
		    spirv::serialiseSpirv(writer.getShader());

		vk::ShaderModuleCreateInfo createInfo;
		createInfo.codeSize = bytecode.size() * sizeof(*bytecode.data());
		createInfo.pCode = bytecode.data();

		m_highlightFragmentModule = vk.create(createInfo);
		if (!m_highlightFragmentModule)
		{
			std::cerr
			    << "error: failed to create highlight fragment shader module"
			    << std::endl;
			abort();
		}
	}
#pragma endregion

#pragma region hightlight pipeline
	{
		vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = m_highlightVertexModule;
		vertShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = m_highlightFragmentModule;
		fragShaderStageInfo.pName = "main";

		std::array shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

		// VkVertexInputBindingDescription binding = {};
		// binding.binding = 0;
		// binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		// binding.stride = sizeof(Vertex);

		// VkVertexInputAttributeDescription positionAttribute = {};
		// positionAttribute.binding = 0;
		// positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
		// positionAttribute.location = 0;
		// positionAttribute.offset = 0;

		// VkVertexInputAttributeDescription normalAttribute = {};
		// normalAttribute.binding = 0;
		// normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
		// normalAttribute.location = 1;
		// normalAttribute.offset = sizeof(vector3);

		// VkVertexInputAttributeDescription uvAttribute = {};
		// uvAttribute.binding = 0;
		// uvAttribute.format = VK_FORMAT_R32G32_SFLOAT;
		// uvAttribute.location = 2;
		// uvAttribute.offset = sizeof(vector3) + sizeof(vector3);

		// VkVertexInputAttributeDescription tangentAttribute = {};
		// tangentAttribute.binding = 0;
		// tangentAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
		// tangentAttribute.location = 3;
		// tangentAttribute.offset =
		//	sizeof(vector3) + sizeof(vector3) + sizeof(vector2);

		// std::array attributes = { positionAttribute, normalAttribute,
		// uvAttribute, 	tangentAttribute };

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
		// vertexInputInfo.vertexBindingDescriptionCount = 0;
		// vertexInputInfo.pVertexBindingDescriptions = &binding;
		// vertexInputInfo.vertexAttributeDescriptionCount = 0;
		// uint32_t(attributes.size());
		// vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

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
		rasterizer.cullMode = VK_CULL_MODE_NONE;
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
		// colorBlendAttachment.srcColorBlendFactor =
		// VK_BLEND_FACTOR_SRC_ALPHA; colorBlendAttachment.dstColorBlendFactor
		// =
		//    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		// colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		// colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		// colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		// colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		// VkPipelineColorBlendAttachmentState objectIDBlendAttachment = {};
		// objectIDBlendAttachment.colorWriteMask =
		//	VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		//	VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		// objectIDBlendAttachment.blendEnable = false;
		// objectIDBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		// objectIDBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		// objectIDBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		// objectIDBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		// objectIDBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		// objectIDBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		// colorHDRBlendAttachment.blendEnable = true;
		// colorHDRBlendAttachment.srcColorBlendFactor =
		// VK_BLEND_FACTOR_SRC_ALPHA;
		// colorHDRBlendAttachment.dstColorBlendFactor =
		//    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		// colorHDRBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		// colorHDRBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		// colorHDRBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		// colorHDRBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		std::array colorBlendAttachments{
			colorBlendAttachment,
			// objectIDBlendAttachment
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
		depthStencil.depthTestEnable = false;
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
		// pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pDepthStencilState = nullptr;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr;
		pipelineInfo.layout = m_highlightPipelineLayout;
		pipelineInfo.renderPass = m_highlightRenderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = nullptr;
		pipelineInfo.basePipelineIndex = -1;

		m_highlightPipeline = vk.create(pipelineInfo);
		if (!m_highlightPipeline)
		{
			std::cerr << "error: failed to create graphics hightlight pipeline"
			          << std::endl;
			abort();
		}
	}
#pragma endregion

#pragma region skybox
	m_skybox = std::make_unique<Skybox>(rw, m_renderPass.get(), viewport,
	                                    m_environmentMap);
#pragma endregion

	m_creationTime = rw.get().getTime();
}
}  // namespace cdm
