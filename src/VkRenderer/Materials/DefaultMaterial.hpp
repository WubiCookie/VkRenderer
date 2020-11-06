#pragma once

#include "Material.hpp"
#include "Buffer.hpp"
#include "Texture2D.hpp"

#include <memory>

namespace cdm
{
class DefaultMaterial : public Material
{
	Buffer m_uniformBuffer;

	struct alignas(16) UBOStruct
	{
		vector4 color;

		float metalness;
		float roughness;

		uint32_t textureIndex;

		// padding
		//float _0{ float(0xcccccccc) };
		float _1{ float(0xcccccccc) };
	};

	std::vector<UBOStruct> m_uboStructs;

	struct FragmentShaderBuildData : FragmentShaderBuildDataBase
	{
		std::unique_ptr<sdw::Ubo> ubo;
		std::unique_ptr<sdw::Array<sdw::SampledImage2DRgba32>> textures;
	};

	Texture2D m_texture;
	std::reference_wrapper<Texture2D> m_textureRef;
	std::vector<std::reference_wrapper<Texture2D>> m_textures;

public:
	DefaultMaterial() = default;
	DefaultMaterial(RenderWindow& rw, PbrShadingModel& shadingModel,
	                uint32_t instancePoolSize = 0);
	DefaultMaterial(const DefaultMaterial&) = delete;
	DefaultMaterial(DefaultMaterial&&) = default;
	~DefaultMaterial() = default;

	DefaultMaterial& operator=(const DefaultMaterial&) = delete;
	DefaultMaterial& operator=(DefaultMaterial&&) = default;

protected:
	float    floatParameter(const std::string& name, uint32_t instanceIndex) override;
	vector4  vec4Parameter (const std::string& name, uint32_t instanceIndex) override;

	void setFloatParameter(const std::string& name, uint32_t instanceIndex, float a)          override;
	void setVec4Parameter(const std::string& name, uint32_t instanceIndex, const vector4& a) override;
	void setTextureParameter(const std::string& name, uint32_t instanceIndex, Texture2D& texture);

public:
	// void vertexFunction(Vec3& inOutPosition, Vec3& inOutNormal);
	MaterialVertexFunction vertexFunction(
	    sdw::VertexWriter& writer,
	    VertexShaderBuildDataBase* shaderBuildData) override;

	// void fragmentFunction(Vec4& inOutAlbedo,
	//                       Vec3& inOutWsPosition,
	//                       Vec3& inOutWsNormal,
	//                       Vec3& inOutWsTangent,
	//                       Float& inOutMetalness,
	//                       Float& inOutRoughness);
	MaterialFragmentFunction fragmentFunction(
	    sdw::FragmentWriter& writer,
	    FragmentShaderBuildDataBase* shaderBuildData) override;

	std::unique_ptr<FragmentShaderBuildDataBase>
	instantiateFragmentShaderBuildData() override;
};
}  // namespace cdm
