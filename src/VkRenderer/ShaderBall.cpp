#include "ShaderBall.hpp"

#include "CommandBufferPool.hpp"
#include "EquirectangularToCubemap.hpp"
#include "TextureFactory.hpp"

#include <CompilerSpirV/compileSpirV.hpp>
#include <ShaderWriter/Intrinsics/Intrinsics.hpp>
#include <ShaderWriter/Source.hpp>

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include <Texas/Texas.hpp>
#include <Texas/VkTools.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "my_imgui_impl_vulkan.h"

#include "astc-codec/astc-codec.h"
#include "load_dds.hpp"
#include "stb_image.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>

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

#undef Locale
#undef FLOAT
#undef VEC2
#undef VEC3
#undef Constant

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

			    Locale(irradiance, irradianceMap.sample(TBN * N));
			    Locale(diffuse, irradiance * albedo);

			    Locale(MAX_REFLECTION_LOD,
			           cast<Float>(prefilteredMap.getLevels()));
			    Locale(prefilteredColor,
			           prefilteredMap.lod(R, roughness * MAX_REFLECTION_LOD));
			    Locale(brdf,
			           brdfLut.sample(vec2(max(dot(N, V), 0.0_f), roughness))
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
	    Buffer(vk, sizeof(ShaderBallVertex) * vertices.size(),
	           VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
	           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	vertexBuffer = Buffer(
	    vk, sizeof(ShaderBallVertex) * vertices.size(),
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
	    Buffer(vk, sizeof(uint32_t) * indices.size(),
	           VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
	           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	indexBuffer = Buffer(
	    vk, sizeof(uint32_t) * indices.size(),
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
      m_shadingModel(rw.get().device(), 1, 1),
      m_defaultMaterial(rw, m_shadingModel, 1000),
      m_scene(renderWindow),
      imguiCB(CommandBuffer(rw.get().device(), rw.get().oneTimeCommandPool())),
      copyHDRCB(
          CommandBuffer(rw.get().device(), rw.get().oneTimeCommandPool()))
{
	auto& vk = rw.get().device();

	LogRRID log(vk);

	Assimp::Importer importer;

#pragma region bunny mesh
	const aiScene* bunnyScene = importer.ReadFile(
	    "../resources/illumination_assets/bunny.obj",
	    aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals |
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

	std::vector<StandardMesh::Vertex> vertices;
	std::vector<uint32_t> indices;

	{
		vertices.clear();
		indices.clear();

		auto& mesh = bunnyScene->mMeshes[0];
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

		indices.reserve(mesh->mNumFaces * 3);
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			const aiFace& face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

		m_bunnyMesh = StandardMesh(rw, vertices, indices);
	}
#pragma endregion

#pragma region sponza mesh
	const aiScene* sponzaScene = importer.ReadFile(
	    "../resources/Vulkan-Samples-Assets/scenes/sponza/Sponza01.gltf",
	    //"../resources/illumination_assets/bistro/BistroExterior.fbx",
	    //"../resources/illumination_assets/bistro/BistroInterior.fbx",
	    //"../resources/illumination_assets/ZeroDay/MEASURE_ONE/MEASURE_ONE.fbx",
	    aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals |
	        aiProcess_CalcTangentSpace | aiProcess_RemoveRedundantMaterials);

	if (!sponzaScene || sponzaScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
	    !sponzaScene->mRootNode)
	{
		std::cerr << "assimp error: " << importer.GetErrorString()
		          << std::endl;
		throw std::runtime_error(std::string("assimp error: ") +
		                         importer.GetErrorString());
	}

	//*
	enum class AlphaMode
	{
		Opaque,
		Blending,
		Cutoff,
	};

	struct LoadedMaterial
	{
		vector4 diffuse;
		vector4 baseColorFactor;
		std::string tex1;
		vector2 texCoord1;
		std::string tex2;
		vector2 texCoord2;
		float metallicFactor;
		float roughnessFactor;
		float shininess;
		vector4 emissive;
		bool twoSided;
		AlphaMode alphaMode;
		float alphaCutoff;
	};

	CommandBufferPool sponzaPool(vk, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

	m_sponzaMaterialInstances.resize(sponzaScene->mNumMaterials);
	for (size_t i = 0; i < sponzaScene->mNumMaterials; i++)
	{
		TextureFactory f(vk);
		f.setUsage(VK_IMAGE_USAGE_SAMPLED_BIT |
		           VK_IMAGE_USAGE_TRANSFER_DST_BIT);

		auto& material = *sponzaScene->mMaterials[i];
		// std::cout << material.GetName().C_Str() << std::endl;

		aiString diffuseTexturePath;
		material.Get(AI_MATKEY_TEXTURE_DIFFUSE(0), diffuseTexturePath);

		if (std::string_view(diffuseTexturePath.C_Str()).empty())
			continue;

		/*
		// std::cout << "diffuse " << diffuseTexturePath.C_Str() << std::endl;
		// material.Get(AI_MATKEY_TEXTURE_DIFFUSE(0), diffuseTexturePath);
		// std::cout << "diffuse " << diffuseTexturePath.C_Str() << std::endl;

		// std::string path = "../resources/illumination_assets/bistro/";
		std::string path =
		    "../resources/illumination_assets/ZeroDay/MEASURE_ONE/";
		path += diffuseTexturePath.C_Str();
		 std::cout << path << std::endl;

		m_sponzaMaterialInstances[i] = m_defaultMaterial.instanciate();

		auto found = m_sponzaTextures.find(path);

		if (found == m_sponzaTextures.end() || found->second == nullptr ||
		    found->second->image() == nullptr)
		{
		    // std::cout << "creating it" << std::endl;

		    m_sponzaTextures[path] = std::make_unique<Texture2D>(
		        texture_loadDDS(path.c_str(), f, sponzaPool));
		    m_sponzaTextures[path]->setName(diffuseTexturePath.C_Str());
		}

		m_sponzaMaterialInstances[i]->setTextureParameter(
		    "", *m_sponzaTextures[path]);

		if (i % 16 == 15)
		    sponzaPool.waitForAllCommandBuffers();
		//*/

		std::string path = "../resources/Vulkan-Samples-Assets/scenes/sponza/";
		path += diffuseTexturePath.C_Str();
		std::cout << path << std::endl;

		m_sponzaMaterialInstances[i] = m_defaultMaterial.instanciate();

		auto found = m_sponzaTextures.find(path);

		if (found == m_sponzaTextures.end() || found->second == nullptr ||
		    found->second->image() == nullptr)
		{
			Texas::ResultValue<Texas::Texture> res =
			    Texas::loadFromPath(path.c_str());

			if (!res)
			{
				std::cerr << "Texas error (" << path
				          << "): " << res.errorMessage() << std::endl;
			}
			else
			{
				const Texas::Texture& tex = res.value();

				const Texas::TextureInfo& textureInfo = tex.textureInfo();
				Texas::FileFormat fileFormat = tex.fileFormat();
				Texas::TextureType textureType = tex.textureType();
				Texas::PixelFormat pixelFormat = tex.pixelFormat();
				Texas::ChannelType channelType = tex.channelType();
				Texas::ColorSpace colorSpace = tex.colorSpace();
				Texas::Dimensions baseDimensions = tex.baseDimensions();
				std::uint8_t mipCount = tex.mipCount();
				std::uint64_t layerCount = tex.layerCount();
				std::uint64_t mipOffset = tex.mipOffset(0);
				// Texas::ConstByteSpan mipSpan = tex.mipSpan(0);
				// std::uint64_t layerOffset = tex.layerOffset(0, 0);
				// Texas::ConstByteSpan layerSpan = tex.layerSpan(0, 0);
				// Texas::ConstByteSpan rawBufferSpan = tex.rawBufferSpan();

				if (pixelFormat != Texas::PixelFormat::ASTC_8x8)
					abort();
				if (channelType != Texas::ChannelType::sRGB)
					abort();
				if (colorSpace != Texas::ColorSpace::sRGB)
					abort();
				if (layerCount != 1)
					abort();
				if (mipOffset != 0)
					abort();

				f.setWidth(baseDimensions.width);
				f.setHeight(baseDimensions.height);
				f.setDepth(baseDimensions.depth);
				// f.setFormat(Texas::toVkFormat(pixelFormat));
				f.setFormat(VK_FORMAT_R8G8B8A8_SRGB);
				// f.setMipLevels(1);
				f.setMipLevels(uint32_t(mipCount));
				f.setLod(0.0f, float(mipCount));
				VkImageSubresourceRange range{};
				range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				range.baseMipLevel = 0;
				range.levelCount = uint32_t(mipCount);
				range.baseArrayLayer = 0;
				range.layerCount = 1;
				f.setViewSubresourceRange(range);
				// for (size_t i = 0; i < mipSpan.size()/16; i++)
				//{
				//	    rgbaData.data() + i,
				//	    reinterpret_cast<const uint8_t*>(mipSpan.data()) +
				//	        i,
				//	    true, 8, 8);
				//}

				auto& texture = m_sponzaTextures[path] =
				    std::make_unique<Texture2D>(f.createTexture2D());
				texture->setName(diffuseTexturePath.C_Str());

				int32_t mipWidth = int32_t(baseDimensions.width);
				int32_t mipHeight = int32_t(baseDimensions.height);

				for (uint32_t i = 0; i < mipCount; i++)
				// uint32_t i = 0;
				{
					std::vector<uint8_t> rgbaData(
					    // baseDimensions.width * baseDimensions.height
					    mipWidth * mipHeight * 4);

					Texas::ConstByteSpan mipSpan = tex.mipSpan(i);

					astc_codec::ASTCDecompressToRGBA(
					    reinterpret_cast<const uint8_t*>(mipSpan.data()),
					    mipSpan.size(),
					    // baseDimensions.width, baseDimensions.height,
					    mipWidth, mipHeight, astc_codec::FootprintType::k8x8,
					    rgbaData.data(), rgbaData.size(),
					    // baseDimensions.width * 4
					    mipWidth * 4);

					// Texas::ConstByteSpan layerSpan = tex.layerSpan(i, 0);

					// Texas::Dimensions mipDimensions =
					// Texas::calcMipDimensions(tex.baseDimensions(), i);

					VkBufferImageCopy region{};
					region.imageExtent.width = mipWidth;
					region.imageExtent.height = mipHeight;
					region.imageExtent.depth = 1;
					region.bufferRowLength = mipWidth;
					region.bufferImageHeight = mipHeight;
					region.imageSubresource.aspectMask =
					    VK_IMAGE_ASPECT_COLOR_BIT;
					region.imageSubresource.baseArrayLayer = 0;
					region.imageSubresource.layerCount = 1;
					region.imageSubresource.mipLevel = i;
					std::cout << "loading level " << i << "/" << +mipCount
					          << " (" << mipWidth << ";" << mipHeight << ")"
					          << std::endl;
					texture->uploadData(
					    rgbaData.data(), rgbaData.size(), region,
					    VK_IMAGE_LAYOUT_UNDEFINED,
					    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sponzaPool);

					if (mipWidth > 1)
						mipWidth /= 2;
					if (mipHeight > 1)
						mipHeight /= 2;
				}
			}

			// texture_loadDDS(path.c_str(), f, sponzaPool));
		}

		m_sponzaMaterialInstances[i]->setTextureParameter(
		    "", *m_sponzaTextures[path]);
		m_sponzaMaterialInstances[i]->setFloatParameter("roughness", 0.9f);
		m_sponzaMaterialInstances[i]->setFloatParameter("metalness", 0.0f);

		if (i % 16 == 15)
			sponzaPool.waitForAllCommandBuffers();

		/*
		for (size_t j = 0; j < material.mNumProperties; j++)
		{
		    auto& prop = *material.mProperties[j];
		    std::cout << "    mKey        " << prop.mKey.C_Str() << std::endl;
		    std::cout << "    mSemantic   " << prop.mSemantic << std::endl;
		    std::cout << "    mIndex      " << prop.mIndex << std::endl;
		    std::cout << "    mDataLength " << prop.mDataLength << std::endl;
		    std::cout << "    mType       ";

		    ai_real f[16];
		    aiString str;

		    switch (prop.mType)
		    {
		    case aiPTI_Float:
		        std::cout << "Float" << std::endl;
		        material.Get(prop.mKey.C_Str(), unsigned int(prop.mType),
		                     prop.mIndex, f);
		        std::cout << "                " << f << std::endl;
		        break;
		    case aiPTI_Double: std::cout << "Double" << std::endl; break;
		    case aiPTI_String:
		        std::cout << "String" << std::endl;
		        material.Get(prop.mKey.C_Str(), prop.mType, prop.mIndex, str);
		        std::cout << "                " << str.C_Str() << std::endl;
		        break;
		    case aiPTI_Integer: std::cout << "Integer" << std::endl; break;
		    case aiPTI_Buffer: std::cout << "Buffer" << std::endl; break;
		    }

		    // std::cout << "    mType       ";
		    std::cout << "                ";

		    // if (prop.mType == )

		    std::cout << std::endl;
		    // $clr.diffuse
		    // $mat.gltf.pbrMetallicRoughness.baseColorFactor
		    // $tex.file
		    // $tex.file.texCoord
		    // $tex.file
		    // $tex.file.texCoord
		    // $mat.gltf.pbrMetallicRoughness.metallicFactor
		    // $mat.gltf.pbrMetallicRoughness.roughnessFactor
		    // $mat.shininess
		    // $clr.emissive
		    // $mat.twosided
		    // $mat.gltf.alphaMode
		    // $mat.gltf.alphaCutoff
		}

		std::cout << std::endl;
		//*/
	}
	//*/

	m_sponzaMeshes.reserve(std::min(sponzaScene->mNumMeshes, 100u));
	for (size_t i = 0; i < std::min(sponzaScene->mNumMeshes, 100u); i++)
	{
		vertices.clear();
		indices.clear();

		auto& mesh = sponzaScene->mMeshes[i];
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

		m_sponzaMeshes.emplace_back(StandardMesh(rw, vertices, indices));

		auto* sponzaSceneObject = &m_scene.instantiateSceneObject();
		sponzaSceneObject->setMesh(m_sponzaMeshes.back());
		if (m_sponzaMaterialInstances[mesh->mMaterialIndex])
			sponzaSceneObject->setMaterial(
			    *m_sponzaMaterialInstances[mesh->mMaterialIndex]);
		m_sponzaSceneObjects.push_back(sponzaSceneObject);
	}
#pragma endregion

	/*
#pragma region sponza
	m_materialInstance4 = m_defaultMaterial.instanciate();
	m_materialInstance4->setFloatParameter("roughness", 1.0f);
	m_materialInstance4->setFloatParameter("metalness", 0.0f);
	m_materialInstance4->setVec4Parameter("color",
	                                      vector4(0.5, 0.5, 0.5, 0.5));
	// m_materialInstance3->setTextureParameter("", m_singleTexture2);

	for (auto& mesh : m_sponzaMeshes)
	{
	    auto* sponzaSceneObject = &m_scene.instantiateSceneObject();
	    sponzaSceneObject->setMesh(mesh);
	    sponzaSceneObject->setMaterial(*m_materialInstance4);
	    m_sponzaSceneObjects.push_back(sponzaSceneObject);
	}
#pragma endregion
	//*/

#pragma region sphere mesh
	const aiScene* sphereScene = importer.ReadFile(
	    "../resources/Vulkan-Samples-Assets/scenes/geosphere.gltf",
	    aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals |
	        aiProcess_CalcTangentSpace);

	if (!sphereScene || sphereScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
	    !sphereScene->mRootNode)
	{
		std::cerr << "assimp error: " << importer.GetErrorString()
		          << std::endl;
		throw std::runtime_error(std::string("assimp error: ") +
		                         importer.GetErrorString());
	}

	// assert(sphereScene->mNumMeshes == 1);

	{
		vertices.clear();
		indices.clear();

		auto& mesh = sphereScene->mMeshes[0];
		vertices.reserve(mesh->mNumVertices);
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			StandardMesh::Vertex vertex;
			vertex.position.x = mesh->mVertices[i].x;
			vertex.position.y = mesh->mVertices[i].y;
			vertex.position.z = mesh->mVertices[i].z;
			vertex.normal = vertex.position.get_normalized();
			// if (mesh->HasNormals())
			//{
			//	vertex.normal.x = mesh->mNormals[i].x;
			//	vertex.normal.y = mesh->mNormals[i].y;
			//	vertex.normal.z = mesh->mNormals[i].z;
			//}
			// else
			//{
			//	vertex.normal = { 1, 0, 0 };
			//}
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
				vertex.tangent.normalize();
			}
			else
			{
				vertex.tangent = { 0, 1, 0 };
			}

			vertices.push_back(vertex);
		}

		indices.reserve(mesh->mNumFaces * 3);
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			const aiFace& face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

		m_sphereMesh = StandardMesh(rw, vertices, indices);
	}
#pragma endregion

#pragma region assimp
	// const aiScene* scene = importer.ReadFile(
	//    "../resources/ShaderBall.fbx",
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
	colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
	colorAttachment.samples = VK_SAMPLE_COUNT_4_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	// colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription objectIDAttachment = {};
	objectIDAttachment.format = VK_FORMAT_R32_UINT;
	objectIDAttachment.samples = VK_SAMPLE_COUNT_4_BIT;
	objectIDAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	// objectIDAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	objectIDAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	objectIDAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	objectIDAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	objectIDAttachment.initialLayout =
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	objectIDAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription normalDepthAttachment = {};
	normalDepthAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	normalDepthAttachment.samples = VK_SAMPLE_COUNT_4_BIT;
	normalDepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	normalDepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	normalDepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	normalDepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	normalDepthAttachment.initialLayout =
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	normalDepthAttachment.finalLayout =
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription positionAttachment = {};
	positionAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	positionAttachment.samples = VK_SAMPLE_COUNT_4_BIT;
	positionAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	positionAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	positionAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	positionAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	positionAttachment.initialLayout =
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	positionAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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
	colorResolveAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
	colorResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// colorResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
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

	VkAttachmentDescription normalDepthResolveAttachment = {};
	normalDepthResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	normalDepthResolveAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	normalDepthResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	normalDepthResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	normalDepthResolveAttachment.stencilLoadOp =
	    VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	normalDepthResolveAttachment.stencilStoreOp =
	    VK_ATTACHMENT_STORE_OP_DONT_CARE;
	normalDepthResolveAttachment.initialLayout =
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	normalDepthResolveAttachment.finalLayout =
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription positionResolveAttachment = {};
	positionResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	positionResolveAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	positionResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	positionResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	positionResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	positionResolveAttachment.stencilStoreOp =
	    VK_ATTACHMENT_STORE_OP_DONT_CARE;
	positionResolveAttachment.initialLayout =
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	positionResolveAttachment.finalLayout =
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::array colorAttachments{ colorAttachment,
		                         objectIDAttachment,
		                         depthAttachment,
		                         colorResolveAttachment,
		                         objectIDResolveAttachment,
		                         normalDepthAttachment,
		                         normalDepthResolveAttachment,
		                         positionAttachment,
		                         positionResolveAttachment };

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

	VkAttachmentReference normalDepthAttachmentRef = {};
	normalDepthAttachmentRef.attachment = 5;
	normalDepthAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference normalDepthResolveAttachmentRef = {};
	normalDepthResolveAttachmentRef.attachment = 6;
	normalDepthResolveAttachmentRef.layout =
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference positionAttachmentRef = {};
	positionAttachmentRef.attachment = 7;
	positionAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference positionResolveAttachmentRef = {};
	positionResolveAttachmentRef.attachment = 8;
	positionResolveAttachmentRef.layout =
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::array colorAttachmentRefs{ colorAttachmentRef  //, depthAttachmentRef
		                            ,
		                            objectIDAttachmentRef,
		                            normalDepthAttachmentRef,
		                            positionAttachmentRef };

	std::array resolveAttachmentRefs{ colorResolveAttachmentRef,
		                              objectIDResolveAttachmentRef,
		                              normalDepthResolveAttachmentRef,
		                              positionResolveAttachmentRef };

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

	vk.debugMarkerSetObjectName(m_renderPass, "ShaderBall render pass");
#pragma endregion

#pragma region hightlight render
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout =
		    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		/*VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = rw.get().depthImageFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_4_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout =
		    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		//VK_IMAGE_LAYOUT_UNDEFINED depthAttachment.finalLayout =
		    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;*/

		std::array colorAttachments{ colorAttachment /*, depthAttachment*/ };

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		/*VkAttachmentReference depthReference = {};
		depthReference.attachment = 1;
		depthReference.layout =
		    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;*/

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
		subpass.pDepthStencilAttachment = nullptr;  // &depthReference;
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
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pcRange;

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

		VkDescriptorSetLayoutBinding layoutBindingNormalDepthTexture{};
		layoutBindingNormalDepthTexture.binding = 2;
		layoutBindingNormalDepthTexture.descriptorCount = 1;
		layoutBindingNormalDepthTexture.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindingNormalDepthTexture.stageFlags =
		    VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding layoutBindingNoiseTexture{};
		layoutBindingNoiseTexture.binding = 3;
		layoutBindingNoiseTexture.descriptorCount = 1;
		layoutBindingNoiseTexture.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindingNoiseTexture.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding layoutBindingpositionTexture{};
		layoutBindingpositionTexture.binding = 4;
		layoutBindingpositionTexture.descriptorCount = 1;
		layoutBindingpositionTexture.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindingpositionTexture.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding layoutBindingUbo{};
		layoutBindingUbo.binding = 5;
		layoutBindingUbo.descriptorCount = 1;
		layoutBindingUbo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBindingUbo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array highlightLayoutBindings{
			layoutBindingHighlightInputColor, layoutBindingHighlightInputID,
			layoutBindingNormalDepthTexture,  layoutBindingNoiseTexture,
			layoutBindingpositionTexture,     layoutBindingUbo
		};

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
	m_uniformBuffer =
	    Buffer(vk, sizeof(UBOStruct), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	           VMA_MEMORY_USAGE_CPU_ONLY,
	           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	m_uniformBuffer.setName("UBO");

	std::uniform_real_distribution<float> randomFloats(
	    0.0, 1.0);  // random floats between [0.0, 1.0]
	std::default_random_engine generator;

	for (uint32_t i = 0; i < 64; ++i)
	{
		vector3 sample(randomFloats(generator) * 2.0f - 1.0f,
		               randomFloats(generator) * 2.0f - 1.0f,
		               randomFloats(generator));

		sample = cdm::normalized(sample);   // Snap to surface of hemisphere
		sample *= randomFloats(generator);  // Space out linearly

		float scale = (float)i / (float)64;
		scale = cdm::lerp(
		    0.1f, 1.0f,
		    scale * scale);  // Bring distribution of samples closer to origin

		m_uboStruct.samples[i] = vector4(sample * scale, 0.0f);
	}
	m_uboStruct.proj =
	    matrix4::perspective(90_deg,
	                         float(rw.get().swapchainExtent().width) /
	                             float(rw.get().swapchainExtent().height),
	                         0.01f, 1000.0f)
	        .get_transposed();

	void* ptr = m_uniformBuffer.map<UBOStruct>();
	std::memcpy(ptr, &m_uboStruct, sizeof(UBOStruct));
	m_uniformBuffer.unmap();

	VkDescriptorBufferInfo setBufferInfo{};
	setBufferInfo.buffer = m_uniformBuffer;
	setBufferInfo.range = sizeof(UBOStruct);
	setBufferInfo.offset = 0;

	vk::WriteDescriptorSet uboWrite;
	uboWrite.descriptorCount = 1;
	uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboWrite.dstArrayElement = 0;
	uboWrite.dstBinding = 5;
	uboWrite.dstSet = m_highlightDescriptorSet;
	uboWrite.pBufferInfo = &setBufferInfo;

	vk.updateDescriptorSets(uboWrite);

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

	TextureFactory f(vk);

#pragma region equirectangularHDR
	int w, h, c;
	float* imageData = stbi_loadf(
	    //"../resources/illumination_assets/PaperMill_Ruins_E/PaperMill_E_3k.hdr",
	    "../resources/illumination_assets/Milkyway/Milkyway_small.hdr",
	    //"../resources/illumination_assets/UV-Testgrid/testgrid.jpg",
	    &w, &h, &c, 4);

	if (!imageData)
		throw std::runtime_error("could not load equirectangular map");

	f.setWidth(w);
	f.setHeight(h);
	f.setFormat(VK_FORMAT_R32G32B32A32_SFLOAT);
	f.setUsage(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

	m_equirectangularTexture = f.createTexture2D();

	// m_equirectangularTexture = Texture2D(
	//    rw, w, h, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
	//    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
	//    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

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
	    imageData, size_t(w) * size_t(h) * 4 * sizeof(float), copy,
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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
		                                "Milkyway_small_irradiance.hdr");

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

		m_prefilteredMap = PrefilteredCubemap(
		    rw, 512, -1, m_environmentMap, "Milkyway_small_prefiltered.hdr");

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
	f.setWidth(1);
	f.setHeight(1);
	f.setFormat(VK_FORMAT_R8G8B8A8_UNORM);
	f.setUsage(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

	m_defaultTexture = f.createTexture2D();

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
			f.setWidth(w);
			f.setHeight(h);
			f.setFormat(VK_FORMAT_R8G8B8A8_UNORM);
			f.setUsage(VK_IMAGE_USAGE_SAMPLED_BIT |
			           VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			           VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
			f.setMipLevels(-1);

			arr[element] = f.createTexture2D();

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

	auto createTexture = [&](std::string_view filename) {
		int w, h, c;
		uint8_t* imageData = stbi_load(filename.data(), &w, &h, &c, 4);

		if (imageData)
		{
			f.setWidth(w);
			f.setHeight(h);
			f.setFormat(VK_FORMAT_R8G8B8A8_UNORM);
			f.setUsage(VK_IMAGE_USAGE_SAMPLED_BIT |
			           VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			           VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
			f.setMipLevels(-1);

			Texture2D tex;

			tex = f.createTexture2D();

			// m_singleTexture = Texture2D(
			//	rw, w, h, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
			//	VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			//	VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			//	VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			//	-1);

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

			tex.uploadDataImmediate(imageData, w * h * 4 * sizeof(uint8_t),
			                        copy, VK_IMAGE_LAYOUT_UNDEFINED,
			                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			stbi_image_free(imageData);

			tex.generateMipmapsImmediate(
			    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			/*VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = m_singleTexture.view();
			imageInfo.sampler = m_singleTexture.sampler();

			vk::WriteDescriptorSet textureWrite;
			textureWrite.descriptorCount = 1;
			textureWrite.descriptorType =
			    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			textureWrite.dstArrayElement = 0;
			textureWrite.dstBinding = 1;
			textureWrite.dstSet = m_defaultMaterial.descriptorSet();
			textureWrite.pImageInfo = &imageInfo;

			vk.updateDescriptorSets(textureWrite);*/

			// m_singleTexture = &tex;
			return tex;
		}
		assert(false);
	};

	// create texture2D
	// call setTexture of DefaultMaterial

	std::string resourcePath = "../resources/illumination_assets/";
	// resourcePath += "Metal007_4K-JPG/";
	// resourcePath += "Marble009_8K-JPG/";
	// resourcePath += "Leather011_8K-JPG/";
	// resourcePath += "ChristmasTreeOrnament006_8K-JPG/";
	// resourcePath += "Chip001_4K-JPG";

	// updateTextureDescriptor(
	//    "../resources/Marble009_2K-JPG/"
	//    "Marble009_2K_Color.jpg",
	//    m_albedos, 1);
	updateTextureDescriptor(resourcePath + "/MetalAlbedo.png", m_albedos, 1);
	updateTextureDescriptor(resourcePath + "/Displacement.jpg",
	                        m_displacements, 1);
	updateTextureDescriptor(resourcePath + "/Metalness.jpg", m_metalnesses, 1);
	updateTextureDescriptor(resourcePath + "/Normal.jpg", m_normals, 1);
	updateTextureDescriptor(resourcePath + "/Roughness.jpg", m_roughnesses, 1);

	updateTextureDescriptor(
	    "../resources/illumination_assets/MathieuMaurel ShaderBall "
	    "2017/Textures/ShaderBall_A_INBALL.png",
	    m_albedos, 2);
	updateTextureDescriptor(
	    "../resources/illumination_assets/MathieuMaurel ShaderBall "
	    "2017/Textures/ShaderBall_A_REPERE.png",
	    m_albedos, 8);
	updateTextureDescriptor(
	    "../resources/illumination_assets/MathieuMaurel ShaderBall "
	    "2017/Textures/ShaderBall_A_CYCLO.png",
	    m_albedos, 9);

	m_singleTexture = createTexture(resourcePath + "Fox.jpg");
	m_singleTexture2 = createTexture(resourcePath + "MetalAlbedo.png");
	// m_meshes[9].materialData.uScale = 20.0f;
	// m_meshes[9].materialData.vScale = 20.0f;
	// m_meshes[2].materialData.uScale = 2.0f;
	// m_meshes[2].materialData.vScale = 2.0f;
	// m_meshes[2].materialData.uShift = 0.5f;
	// m_meshes[2].materialData.vShift = 0.5f;
#pragma endregion

#pragma region LTC lut
	{
		TextureFactory f(vk);
		f.setUsage(VK_IMAGE_USAGE_SAMPLED_BIT |
		           VK_IMAGE_USAGE_TRANSFER_DST_BIT);

		CommandBufferPool pool(vk, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

		m_ltcMat = texture_loadDDS("../runtime_cache/ltc_mat.dds", f, pool);
		m_ltcAmp = texture_loadDDS("../runtime_cache/ltc_amp.dds", f, pool);

		pool.waitForAllCommandBuffers();
	}
#pragma endregion

#pragma region bunny
	m_bunnySceneObject = &m_scene.instantiateSceneObject();
	m_bunnySceneObject->setMesh(m_bunnyMesh);
	m_bunnySceneObject->setMaterial(m_defaultMaterial);

	m_materialInstance1 = m_defaultMaterial.instanciate();
	m_materialInstance1->setFloatParameter("roughness", 0.5f);
	m_materialInstance1->setFloatParameter("metalness", 0.0f);
	m_materialInstance1->setVec4Parameter("color", vector4(1, 0, 0, 1));
	m_materialInstance1->setTextureParameter("", m_singleTexture);

	m_materialInstance2 = m_defaultMaterial.instanciate();
	m_materialInstance2->setFloatParameter("roughness", 0.001f);
	m_materialInstance2->setFloatParameter("metalness", 0.999f);
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

		VkDescriptorImageInfo ltcMatImageInfo{};
		ltcMatImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ltcMatImageInfo.imageView = m_ltcMat.view();
		ltcMatImageInfo.sampler = m_ltcMat.sampler();

		vk::WriteDescriptorSet ltcMatTextureWrite;
		ltcMatTextureWrite.descriptorCount = 1;
		ltcMatTextureWrite.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		ltcMatTextureWrite.dstArrayElement = 0;
		ltcMatTextureWrite.dstBinding = 6;
		ltcMatTextureWrite.dstSet = m_shadingModel.m_descriptorSet;
		ltcMatTextureWrite.pImageInfo = &ltcMatImageInfo;

		VkDescriptorImageInfo ltcAmpImageInfo{};
		ltcAmpImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ltcAmpImageInfo.imageView = m_ltcAmp.view();
		ltcAmpImageInfo.sampler = m_ltcAmp.sampler();

		vk::WriteDescriptorSet ltcAmpTextureWrite;
		ltcAmpTextureWrite.descriptorCount = 1;
		ltcAmpTextureWrite.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		ltcAmpTextureWrite.dstArrayElement = 0;
		ltcAmpTextureWrite.dstBinding = 7;
		ltcAmpTextureWrite.dstSet = m_shadingModel.m_descriptorSet;
		ltcAmpTextureWrite.pImageInfo = &ltcAmpImageInfo;

		vk.updateDescriptorSets(
		    { irradianceMapTextureWrite, prefilteredMapTextureWrite,
		      brdfLutTextureWrite, ltcMatTextureWrite, ltcAmpTextureWrite });
	}
#pragma endregion

#pragma region sphere
	m_materialInstance3 = m_defaultMaterial.instanciate();
	m_materialInstance3->setFloatParameter("roughness", 0.3f);
	m_materialInstance3->setFloatParameter("metalness", 0.0f);
	m_materialInstance3->setVec4Parameter("color", vector4(1, 1, 1, 1));

	m_sphereSceneObject = &m_scene.instantiateSceneObject();
	m_sphereSceneObject->setMesh(m_sphereMesh);
	m_sphereSceneObject->setMaterial(*m_materialInstance3);
#pragma endregion
}

ShaderBall::~ShaderBall() = default;

void ShaderBall::renderOpaque(CommandBuffer& cb)
{
	{
		cb.debugMarkerBegin("shadows", 0.2f, 0.2f, 0.4f);
		m_scene.drawShadowmapPass(cb);
		cb.debugMarkerEnd();

		VkClearValue clearColor{};
		clearColor.color.float32[0] = 0X27 / 255.0f;
		clearColor.color.float32[1] = 0X28 / 255.0f;
		clearColor.color.float32[2] = 0X22 / 255.0f;

		VkClearValue clearNormalDepth{};
		clearNormalDepth.color.float32[0] = 0.0f;
		clearNormalDepth.color.float32[1] = 0.0f;
		clearNormalDepth.color.float32[2] = 0.0f;
		clearNormalDepth.color.float32[3] = 0.0f;

		VkClearValue clearID{};
		clearID.color.uint32[0] = 0xffff;
		clearID.color.uint32[1] = 0xffff;
		clearID.color.uint32[2] = 0xffff;
		clearID.color.uint32[3] = 0xffff;

		VkClearValue clearDepth{};
		clearDepth.depthStencil.depth = 1.0f;

		std::array clearValues = { clearColor,       clearID,
			                       clearDepth,       clearColor,
			                       clearID,          clearNormalDepth,
			                       clearNormalDepth, clearColor,
			                       clearColor };

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

		cb.debugMarkerBegin("skybox", 0.8f, 0.8f, 1.0f);
		m_skybox->render(cb);
		cb.debugMarkerEnd();

		// cb.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
		// cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS,
		// m_pipelineLayout,
		//                     0, m_descriptorSet);

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

		cb.debugMarkerBegin("main", 0.3f, 0.8f, 0.4f);
		m_scene.draw(cb, m_renderPass, std::make_optional(viewport),
		             std::make_optional(scissor));
		cb.debugMarkerEnd();

		// m_bunnySceneObject->draw(cb, m_renderPass, viewport, scissor);
		// m_bunnySceneObject2->draw(cb, m_renderPass, viewport, scissor);
		// m_bunnySceneObject3->draw(cb, m_renderPass, viewport, scissor);

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

		barrier.image = m_normalDepthResolveTexture;
		cb.pipelineBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		                   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, barrier);

		barrier.image = m_positionResolveTexture;
		cb.pipelineBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		                   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, barrier);

		VkClearValue clearColor{};
		clearColor.color.float32[0] = 0X27 / 255.0f;
		clearColor.color.float32[1] = 0X28 / 255.0f;
		clearColor.color.float32[2] = 0X22 / 255.0f;

		// VkClearValue clearID{};
		// clearID.color.uint32[0] = -1u;
		// clearID.color.uint32[1] = -1u;
		// clearID.color.uint32[2] = -1u;
		// clearID.color.uint32[3] = -1u;

		VkClearValue clearDepth{};
		clearDepth.depthStencil.depth = 1.0f;

		std::array clearValues = { clearColor, /*clearID,*/ clearDepth };

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

		cb.debugMarkerBegin("post process", 8.0f, 0.3f, 0.4f);
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

		barrier.image = m_normalDepthResolveTexture;
		cb.pipelineBarrier(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, barrier);

		barrier.image = m_positionResolveTexture;
		cb.pipelineBarrier(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, barrier);

		cb.debugMarkerEnd();
	}
}

vector3 lightDir{ 0.42f, -0.9f, -0.116f };
transform3d lightTr = transform3d(vector3(-43, 222, 16), {}, { 1, 1, 1 });
bool directionalEnabled = true;

vector3 lightPos{ 0, 20, 0 };
bool pointEnabled = true;
bool pointAtCameraEnabled = false;

bool showSSAO = false;
bool showChanged = false;

void ShaderBall::imgui(CommandBuffer& cb)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Lights");

		ImGui::DragFloat("shadow bias", &m_scene.shadowBias, 0.0001f);
		ImGui::DragFloat("R", &m_scene.R, 0.01f);
		ImGui::SliderFloat("sigma", &m_scene.sigma, -Pi / 2.0f, Pi / 2.0f);
		ImGui::SliderFloat("roughness", &m_scene.roughness, 0.0f, 0.7f);
		ImGui::DragFloat3("LTDM 0", &m_scene.LTDM.m00, 0.01f);
		ImGui::DragFloat3("LTDM 1", &m_scene.LTDM.m01, 0.01f);
		ImGui::DragFloat3("LTDM 2", &m_scene.LTDM.m02, 0.01f);
		ImGui::DragFloat("param0", &m_scene.param0, 0.01f);

		ImGui::Checkbox("directional", &directionalEnabled);

		if (directionalEnabled)
		{
			ImGui::DragFloat3("direction", &lightDir.x, 0.01f);
			ImGui::DragFloat3("position", &lightTr.position.x, 0.01f);
		}

		ImGui::Checkbox("point", &pointEnabled);

		if (pointEnabled)
		{
			ImGui::Checkbox("at camera", &pointAtCameraEnabled);
			if (!pointAtCameraEnabled)
				ImGui::DragFloat3("position", &lightPos.x, 0.1f);
		}

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

		showChanged = ImGui::Checkbox("show SSAO only", &showSSAO);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
		            1000.0f / ImGui::GetIO().Framerate,
		            ImGui::GetIO().Framerate);

		// ImGui::Image(
		//    (ImTextureID)(intptr_t)m_scene.shadowmap().image(),
		//    ImVec2(m_scene.shadowmap().width(),
		//    m_scene.shadowmap().height()));

		ImGui::End();

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
	barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
	                        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
	                        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
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
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;
	cb.pipelineBarrier(
	    // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
	    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, barrier);
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
	                         0.01f, 1000.0f)
	        .get_transposed();

	// float aspect = float(rw.get().swapchainExtent().height) /
	//               float(rw.get().swapchainExtent().width);

	// m_config.proj = matrix4::orthographic(-20, 20, 20 * aspect, -20 *
	// aspect,
	//                                      0.01f, 1000.0f)
	//                    .get_transposed();

	m_config.viewPos = cameraTr.position;

	m_bunnySceneObject->transform.position = { 0, 0, -20 };
	m_bunnySceneObject->transform.scale = { 6, 6, 6 };
	m_bunnySceneObject2->transform.position = { 20, 0, -20 };
	m_bunnySceneObject2->transform.scale = { 6, 6, 6 };
	m_bunnySceneObject3->transform.position = { -20, 0, -20 };
	m_bunnySceneObject3->transform.scale = { 6, 6, 6 };

	m_sphereSceneObject->transform.position = { 0.0f, 2.5f, 0.0f };
	m_sphereSceneObject->transform.scale = { 0.5f, 0.5f, 0.5f };
	m_sphereSceneObject->transform.rotation =
	    quaternion(vector3(1, 0, 0), 90.0_deg);

	for (auto& obj : m_sponzaSceneObjects)
	{
		obj->transform.scale = { 0.1f, 0.1f, 0.1f };
		obj->transform.rotation = quaternion(vector3(1, 0, 0), 90.0_deg);
	}

	lightTr.scale = { 1, 1, 1 };

	if (pointAtCameraEnabled)
	{
		lightTr = cameraTr;
		lightDir = (cameraTr.rotation * vector3(0, 0, -1)).get_normalized();
	}

	auto* shadingModelData =
	    m_shadingModel.m_shadingModelStaging
	        .map<PbrShadingModel::ShadingModelUboStruct>();
	shadingModelData->pointLightsCount = pointEnabled;
	shadingModelData->directionalLightsCount = directionalEnabled;
	m_shadingModel.m_shadingModelStaging.unmap();
	m_shadingModel.uploadShadingModelDataStaging();

	auto* pointLights = m_shadingModel.m_pointLightsStaging
	                        .map<PbrShadingModel::PointLightUboStruct>();
	pointLights->color = vector4(1.0f, 1.0f, 1.0f, 1.0f) * 80.f;
	pointLights->intensity = 1.0f;
	if (pointAtCameraEnabled)
		lightPos = cameraTr.position;

	pointLights->position = lightPos;
	m_shadingModel.m_pointLightsStaging.unmap();
	m_shadingModel.uploadPointLightsStaging();

	auto* directionalLights =
	    m_shadingModel.m_directionalLightsStaging
	        .map<PbrShadingModel::DirectionalLightUboStruct>();
	directionalLights->color = vector4(1.0f, 1.0f, 1.0f, 1.0f) * 2.0f;
	directionalLights->intensity = 1.0f;
	directionalLights->direction = lightDir.get_normalized();
	// pointLights->position = vector3(0, 10, 0);
	m_shadingModel.m_directionalLightsStaging.unmap();
	m_shadingModel.uploadDirectionalLightsStaging();

	m_uboStruct.proj =
	    matrix4::perspective(90_deg,
	                         float(rw.get().swapchainExtent().width) /
	                             float(rw.get().swapchainExtent().height),
	                         0.01f, 1000.0f)
	        .get_transposed();

	void* ptr = m_uniformBuffer.map<UBOStruct>();
	std::memcpy(ptr, &m_uboStruct, sizeof(UBOStruct));
	m_uniformBuffer.unmap();

	m_scene.uploadTransformMatrices(
	    cameraTr, m_config.proj,
	    // transform3d(pointLights->position, r, {1,1,1}));
	    lightTr);

	m_skybox->setMatrices(m_config.proj, m_config.view);

	auto& frame = rw.get().getAvailableCommandBuffer();
	frame.reset();
	// vk.resetFence(frame.fence);
	auto& cb = frame.commandBuffer;

	// cb.reset();
	cb.begin();
	renderOpaque(cb);
	cb.debugMarkerBegin("imgui", 0.2f, 0.2f, 1.0f);
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

	if (vk.queueSubmit(vk.graphicsQueue(), cb, VkSemaphore(frame.semaphore),
	                   VkFence(frame.fence)) != VK_SUCCESS)
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
	return rw.get().swapchainCreationTime() > m_creationTime || showChanged;
}

void ShaderBall::rebuild()
{
	showChanged = false;
	auto& vk = rw.get().device();
	vk.wait();

	LogRRID log(vk);

	TextureFactory f(vk);

#pragma region color attachment texture
	f.setExtent(rw.get().swapchainExtent());
	f.setFormat(VK_FORMAT_R8G8B8A8_UNORM);
	// f.setFormat(VK_FORMAT_B8G8R8A8_UNORM);
	f.setUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
	           // VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	f.setFilters(VK_FILTER_LINEAR, VK_FILTER_LINEAR);
	f.setSamples(VK_SAMPLE_COUNT_4_BIT);

	m_colorAttachmentTexture = f.createTexture2D();

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
	f.setExtent(rw.get().swapchainExtent());
	f.setFormat(VK_FORMAT_R8G8B8A8_UNORM);
	f.setUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
	           // VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	f.setFilters(VK_FILTER_LINEAR, VK_FILTER_LINEAR);
	f.setSamples(VK_SAMPLE_COUNT_1_BIT);

	m_highlightColorAttachmentTexture = f.createTexture2D();

	vk.debugMarkerSetObjectName(m_highlightColorAttachmentTexture.image(),
	                            "highlightColorAttachmentTexture");

	m_highlightColorAttachmentTexture.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
#pragma endregion

#pragma region object ID attachment texture
	f.setExtent(rw.get().swapchainExtent());
	f.setFormat(VK_FORMAT_R32_UINT);
	f.setUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
	           // VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	f.setFilters(VK_FILTER_NEAREST, VK_FILTER_NEAREST);
	f.setSamples(VK_SAMPLE_COUNT_4_BIT);

	m_objectIDAttachmentTexture = f.createTexture2D();

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
	f.setExtent(rw.get().swapchainExtent());
	f.setFormat(VK_FORMAT_R32_UINT);
	f.setUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
	           // VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	f.setFilters(VK_FILTER_NEAREST, VK_FILTER_NEAREST);
	f.setSamples(VK_SAMPLE_COUNT_1_BIT);

	m_objectIDResolveTexture = f.createTexture2D();

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
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
	        VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1,
	    VK_SAMPLE_COUNT_4_BIT);

	vk.debugMarkerSetObjectName(m_depthTexture.image(), "depthTexture");

	m_depthTexture.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_UNDEFINED,
	    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

#pragma endregion

#pragma region normalDepth resolve texture
	f.setExtent(rw.get().swapchainExtent());
	f.setFormat(VK_FORMAT_R32G32B32A32_SFLOAT);
	f.setUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
	           // VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	f.setFilters(VK_FILTER_LINEAR, VK_FILTER_LINEAR);
	f.setSamples(VK_SAMPLE_COUNT_4_BIT);

	m_normalDepthTexture = f.createTexture2D();

	vk.debugMarkerSetObjectName(m_normalDepthTexture.image(),
	                            "normalDepthTexture");

	m_normalDepthTexture.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	f.setSamples(VK_SAMPLE_COUNT_1_BIT);

	m_normalDepthResolveTexture = f.createTexture2D();

	vk.debugMarkerSetObjectName(m_normalDepthResolveTexture.image(),
	                            "normalDepthResolveTexture");

	m_normalDepthResolveTexture.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_normalDepthResolveTexture.view();
		imageInfo.sampler = m_normalDepthResolveTexture.sampler();

		vk::WriteDescriptorSet textureWrite;
		textureWrite.descriptorCount = 1;
		textureWrite.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textureWrite.dstArrayElement = 0;
		textureWrite.dstBinding = 2;
		textureWrite.dstSet = m_highlightDescriptorSet;
		textureWrite.pImageInfo = &imageInfo;

		vk.updateDescriptorSets(textureWrite);
	}
#pragma endregion

#pragma region position resolve texture
	f.setExtent(rw.get().swapchainExtent());
	f.setFormat(VK_FORMAT_R32G32B32A32_SFLOAT);
	f.setUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
	           // VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	f.setFilters(VK_FILTER_LINEAR, VK_FILTER_LINEAR);
	f.setSamples(VK_SAMPLE_COUNT_4_BIT);

	m_positionTexture = f.createTexture2D();

	vk.debugMarkerSetObjectName(m_positionTexture.image(), "positionTexture");

	m_positionTexture.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	f.setSamples(VK_SAMPLE_COUNT_1_BIT);

	m_positionResolveTexture = f.createTexture2D();

	vk.debugMarkerSetObjectName(m_positionResolveTexture.image(),
	                            "positionResolveTexture");

	m_positionResolveTexture.transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_positionResolveTexture.view();
		imageInfo.sampler = m_positionResolveTexture.sampler();

		vk::WriteDescriptorSet textureWrite;
		textureWrite.descriptorCount = 1;
		textureWrite.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textureWrite.dstArrayElement = 0;
		textureWrite.dstBinding = 4;
		textureWrite.dstSet = m_highlightDescriptorSet;
		textureWrite.pImageInfo = &imageInfo;

		vk.updateDescriptorSets(textureWrite);
	}
#pragma endregion

#pragma region color resolve texture
	f.setExtent(rw.get().swapchainExtent());
	f.setFormat(VK_FORMAT_R8G8B8A8_UNORM);
	f.setUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
	           // VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	f.setFilters(VK_FILTER_LINEAR, VK_FILTER_LINEAR);
	f.setSamples(VK_SAMPLE_COUNT_1_BIT);

	m_colorResolveTexture = f.createTexture2D();

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

#pragma region noise texture
	int dimension = 4;
	std::vector<vector4> noise = std::vector<vector4>(dimension * dimension);

	std::uniform_real_distribution<float> randomFloats(
	    0.0, 1.0);  // random floats between [0.0, 1.0]
	std::default_random_engine generator;

	for (vector4& noiseSample : noise)
	{
		// Random rotations around z-axis
		noiseSample =
		    vector4{ randomFloats(generator) * 2.0f - 1.0f,
			         randomFloats(generator) * 2.0f - 1.0f, 0.f, 0.f };
	}
	// uint8_t* imageData = stbi_load(filename.data(), &w, &h, &c, 4);

	f.setWidth(dimension);
	f.setHeight(dimension);
	f.setFormat(VK_FORMAT_R32G32B32A32_SFLOAT);
	f.setUsage(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	           VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	f.setMipLevels(-1);

	m_noiseTexture = f.createTexture2D();

	vk.debugMarkerSetObjectName(m_noiseTexture.image(), "noiseTexture");

	VkBufferImageCopy copy{};
	copy.bufferImageHeight = dimension;
	copy.bufferRowLength = dimension;
	copy.imageExtent.width = dimension;
	copy.imageExtent.height = dimension;
	copy.imageExtent.depth = 1;
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.imageSubresource.baseArrayLayer = 0;
	copy.imageSubresource.layerCount = 1;
	copy.imageSubresource.mipLevel = 0;

	void* buffer = noise.data();
	uint32_t bufferSize = (uint32_t)noise.size() * sizeof(vector4);
	m_noiseTexture.uploadDataImmediate(
	    buffer, bufferSize, copy, VK_IMAGE_LAYOUT_UNDEFINED,
	    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	m_noiseTexture.generateMipmapsImmediate(
	    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = m_noiseTexture.view();
	imageInfo.sampler = m_noiseTexture.sampler();

	vk::WriteDescriptorSet textureWrite;
	textureWrite.descriptorCount = 1;
	textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureWrite.dstArrayElement = 0;
	textureWrite.dstBinding = 3;
	textureWrite.dstSet = m_highlightDescriptorSet;
	textureWrite.pImageInfo = &imageInfo;

	vk.updateDescriptorSets(textureWrite);
#pragma endregion

#pragma region framebuffer
	{
		vk::FramebufferCreateInfo framebufferInfo;
		framebufferInfo.renderPass = m_renderPass;
		std::array attachments = { m_colorAttachmentTexture.view(),
			                       m_objectIDAttachmentTexture.view(),
			                       m_depthTexture.view(),
			                       m_colorResolveTexture.view(),
			                       m_objectIDResolveTexture.view(),
			                       m_normalDepthTexture.view(),
			                       m_normalDepthResolveTexture.view(),
			                       m_positionTexture.view(),
			                       m_positionResolveTexture.view() };
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

		vk.debugMarkerSetObjectName(m_framebuffer, "ShaderBall framebuffer");
	}
#pragma endregion

#pragma region highlight framebuffer
	{
		vk::FramebufferCreateInfo framebufferInfo;
		framebufferInfo.renderPass = m_highlightRenderPass;
		std::array attachments = {
			m_highlightColorAttachmentTexture.view() /*,
			 m_highlightDepthTexture.view()*/
		};
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
		auto fragDistance = writer.declOutput<Float>("fragDistance", 7);

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
			fragDistance = (view * model * vec4(inPosition, 1.0_f)).z();

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

		// auto fragPosition = writer.declInput<Vec3>("fragPosition", 0);
		auto fragDistance = writer.declInput<Float>("fragDistance", 7);
		auto fragColor = writer.declOutput<Vec4>("fragColor", 0);
		auto fragID = writer.declOutput<UInt>("fragID", 1);
		auto fragNormalDepth = writer.declOutput<Vec4>("fragNormalDepth", 2);
		auto fragPos = writer.declOutput<Vec3>("fragPos", 3);

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
			       pow(albedos[pc.getMember<UInt>("albedoIndex")].sample(
			               scaledUV),
			           vec4(2.2_f)));
			Locale(metalness, metalnesses[pc.getMember<UInt>("metalnessIndex")]
			                              .sample(scaledUV)
			                              .r() *
			                          pc.getMember<Float>("metalnessScale") +
			                      pc.getMember<Float>("metalnessShift"));
			Locale(roughness, roughnesses[pc.getMember<UInt>("roughnessIndex")]
			                              .sample(scaledUV)
			                              .r() *
			                          pc.getMember<Float>("roughnessScale") +
			                      pc.getMember<Float>("roughnessShift"));

			Locale(N, normals[pc.getMember<UInt>("normalIndex")]
			              .sample(scaledUV)
			              .xyz());
			N = normalize((N * vec3(2.0_f) - vec3(1.0_f)));

			Locale(color, writer.PBRColor(albedo, metalness, roughness, N));

			color = toneMapping(color);

			color = gammaCorrection(color);

			fragColor = color;
			fragID = pc.getMember<UInt>("objectID");
			fragNormalDepth.xyz() = N;
			fragNormalDepth.w() = fragDistance;
			fragPos = writer.fragPosition;
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

	VkPipelineColorBlendAttachmentState positionBlendAttachment = {};
	positionBlendAttachment.colorWriteMask =
	    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
	    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	positionBlendAttachment.blendEnable = false;
	positionBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	positionBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	positionBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	positionBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	positionBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	positionBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

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
		                              positionBlendAttachment };

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
		auto normalDepthTexture =
		    writer.declSampledImage<FImg2DRgba32>("normaldepthTexture", 2, 0);
		auto noiseTexture =
		    writer.declSampledImage<FImg2DRgba32>("noiseTexture", 3, 0);
		auto positionTexture =
		    writer.declSampledImage<FImg2DRgba32>("positionTexture", 4, 0);

		sdw::Ubo ubo = sdw::Ubo(writer, "UBO", 5, 0);
		ubo.declMember<Vec4>("samples", uint32_t(64));
		ubo.declMember<Mat4>("proj");
		ubo.end();

		auto fragColor = writer.declOutput<Vec4>("fragColor", 0);

		Pcb pc(writer, "pc");
		pc.declMember<UInt>("id");
		pc.end();

		writer.implementMain([&]() {
			Locale(uv,
			       in.fragCoord.xy() /
			           vec2(Float(float(rw.get().swapchainExtent().width)),
			                Float(float(rw.get().swapchainExtent().height))));
			Locale(sceneColor, sceneColorTexture.sample(uv));
			Locale(sceneID, sceneIDTexture.sample(uv));

			Locale(id, pc.getMember<UInt>("id"));

			// SSAO
			Locale(depth, normalDepthTexture.sample(uv).a());

			if (showSSAO)
			{
				 IF(writer, depth == 0.0f)
				{
					fragColor = vec4(1.0_f);
					writer.returnStmt();
				}
				 FI;
			}

			Locale(position, positionTexture.sample(uv).rgb());
			Locale(normal, normalize(normalDepthTexture.sample(uv).rgb()));

			Locale(depthTexSize, normalDepthTexture.getSize(0_i));
			Locale(noiseTexSize, noiseTexture.getSize(0_i));
			Locale(renderScale, 0.5_f);

			Locale(noiseUV, vec2(writer.cast<Float>(depthTexSize.x()) /
			                         writer.cast<Float>(noiseTexSize.x()),
			                     writer.cast<Float>(depthTexSize.y()) /
			                         writer.cast<Float>(noiseTexSize.y())) *
			                    uv * renderScale);
			Locale(randomVec, noiseTexture.sample(noiseUV).rgb());

			// create TBN change-of-basis matrix: from tangent-space to
			// view-space
			Locale(tangent,
			       normalize(randomVec - normal * dot(randomVec, normal)));
			Locale(bitangent, cross(tangent, normal));
			Locale(TBN, mat3(tangent, bitangent, normal));

			Locale(kernelSize, 64_u);
			Locale(radius, 100.0_f);
			Locale(bias, 0.01_f);
			Locale(occlusion, 0.0_f);
			Locale(sampleCount, 0_u);

			// iterate over the sample kernel and calculate occlusion factor
			FOR(writer, UInt, i, 0_u, i < kernelSize, ++i)
			{
				// from tangent to view - space
				Locale(samplePos,
				       TBN * ubo.getMemberArray<Vec4>("samples")[i].xyz());
				samplePos = position + samplePos * radius;

				// project sample position (to sample texture) (to get position
				// on screen / texture)
				Locale(offset, vec4(samplePos, 1.0_f));
				offset = ubo.getMember<Mat4>("proj") * offset;
				offset.xy() /= offset.w();
				offset.xy() = offset.xy() * 0.5_f + 0.5_f;
				offset.y() = 1.0_f - offset.y();

				Locale(reconstructedPos,
				       positionTexture.sample(offset.xy()).rgb());
				Locale(
				    sampledNormal,
				    normalize(normalDepthTexture.sample(offset.xy()).rgb()));

				IF(writer, dot(sampledNormal, normal) > 0.99_f)
				{
					++sampleCount;
				}
				ELSE
				{
					Locale(rangeCheck,
					       smoothStep(0.0_f, 1.0_f,
					                  radius / abs(reconstructedPos.z() -
					                               samplePos.z() - bias)));
					occlusion +=
					    TERNARY(writer, Float,
					            reconstructedPos.z() <= samplePos.z() - bias,
					            1.0_f, 0.0_f) *
					    rangeCheck;
					++sampleCount;
				}
				FI;
			}
			ROF;

			occlusion =
			    1.0_f - (occlusion / writer.cast<Float>(max(sampleCount, 1_u)));

			//IF(writer, sceneID == id || id == 4294967294_u)
			{
				fragColor = vec4(occlusion);
			}
			//ELSE { fragColor = sceneColor * occlusion / 10.0_f; }
			//FI;

			if (!showSSAO)
				fragColor = sceneColor;
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
		// vertexInputInfo.vertexBindingDescriptionCount = 1;
		// vertexInputInfo.pVertexBindingDescriptions = &binding;
		// vertexInputInfo.vertexAttributeDescriptionCount =
		//    uint32_t(attributes.size());
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
