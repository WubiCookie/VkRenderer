#include "LightTransport.hpp"
#include "MyShaderWriter.hpp"
#include "PipelineFactory.hpp"
#include "cdm_maths.hpp"

#include <CompilerSpirV/compileSpirV.hpp>
#include <ShaderWriter/Intrinsics/Intrinsics.hpp>
#include <ShaderWriter/Source.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "my_imgui_impl_vulkan.h"

#include <array>
#include <iostream>
#include <stdexcept>

constexpr size_t POINT_COUNT = 2000;

#define TEX_SCALE 1

constexpr uint32_t width = 1280 * TEX_SCALE;
constexpr uint32_t height = 720 * TEX_SCALE;
constexpr float widthf = 1280.0f * TEX_SCALE;
constexpr float heightf = 720.0f * TEX_SCALE;

namespace shader
{
struct Ray : public sdw::StructInstance
{
	Ray(ast::Shader* shader, ast::expr::ExprPtr expr)
	    : StructInstance{ shader, std::move(expr) },
	      position{ getMember<sdw::Vec2>("position") },
	      direction{ getMember<sdw::Vec2>("direction") },
	      waveLength{ getMember<sdw::Float>("waveLength") },
	      amplitude{ getMember<sdw::Float>("amplitude") },
	      phase{ getMember<sdw::Float>("phase") },
	      currentRefraction{ getMember<sdw::Float>("currentRefraction") },
	      frequency{ getMember<sdw::Float>("frequency") },
	      speed{ getMember<sdw::Float>("speed") }
	{
	}

	Ray& operator=(Ray const& rhs)
	{
		sdw::StructInstance::operator=(rhs);
		return *this;
	}

	static ast::type::StructPtr makeType(ast::type::TypesCache& cache)
	{
		auto result = cache.getStruct(ast::type::MemoryLayout::eStd140, "Ray");

		if (result->empty())
		{
			result->declMember("position", ast::type::Kind::eVec2F);
			result->declMember("direction", ast::type::Kind::eVec2F);
			result->declMember("waveLength", ast::type::Kind::eFloat);
			result->declMember("amplitude", ast::type::Kind::eFloat);
			result->declMember("phase", ast::type::Kind::eFloat);
			result->declMember("currentRefraction", ast::type::Kind::eFloat);
			result->declMember("frequency", ast::type::Kind::eFloat);
			result->declMember("speed", ast::type::Kind::eFloat);
		}

		return result;
	}

	sdw::Vec2 position;
	sdw::Vec2 direction;
	sdw::Float waveLength;
	sdw::Float amplitude;
	sdw::Float phase;
	sdw::Float currentRefraction;
	sdw::Float frequency;
	sdw::Float speed;

private:
	using sdw::StructInstance::getMember;
	using sdw::StructInstance::getMemberArray;
};

struct SceneStruct : public sdw::StructInstance
{
	// Mandatory members
	SceneStruct(ast::Shader* shader, ast::expr::ExprPtr expr)
	    : StructInstance{ shader, std::move(expr) },
	      spherePos{ getMember<sdw::Vec2>("spherePos") },
	      sphereRadius{ getMember<sdw::Float>("sphereRadius") },
	      seed{ getMember<sdw::Float>("seed") },
	      airRefraction{ getMember<sdw::Float>("airRefraction") },
	      sphereRefraction{ getMember<sdw::Float>("sphereRefraction") },
	      deltaT{ getMember<sdw::Float>("deltaT") },
	      lightsSpeed{ getMember<sdw::Float>("lightsSpeed") },
	      raysCount{ getMember<sdw::UInt>("raysCount") },
	      raySize{ getMember<sdw::Float>("raySize") },
	      airRefractionScale{ getMember<sdw::Float>("airRefractionScale") },
	      sphereRefractionScale{ getMember<sdw::Float>(
		      "sphereRefractionScale") }
	{
	}

	SceneStruct& operator=(SceneStruct const& rhs)
	{
		sdw::StructInstance::operator=(rhs);
		return *this;
	}

	static ast::type::StructPtr makeType(ast::type::TypesCache& cache)
	{
		auto result = std::make_unique<ast::type::Struct>(
		    cache, ast::type::MemoryLayout::eStd140, "SceneStruct");

		if (result->empty())
		{
			result->declMember("spherePos", ast::type::Kind::eVec2F);
			result->declMember("sphereRadius", ast::type::Kind::eFloat);
			result->declMember("seed", ast::type::Kind::eFloat);
			result->declMember("airRefraction", ast::type::Kind::eFloat);
			result->declMember("sphereRefraction", ast::type::Kind::eFloat);
			result->declMember("deltaT", ast::type::Kind::eFloat);
			result->declMember("lightsSpeed", ast::type::Kind::eFloat);
			result->declMember("raysCount", ast::type::Kind::eUInt);
			result->declMember("raySize", ast::type::Kind::eFloat);
			result->declMember("airRefractionScale", ast::type::Kind::eFloat);
			result->declMember("sphereRefractionScale",
			                   ast::type::Kind::eFloat);
		}

		return result;
	}

	// Content of the structure
	sdw::Vec2 spherePos;
	sdw::Float sphereRadius;
	sdw::Float seed;
	sdw::Float airRefraction;
	sdw::Float sphereRefraction;
	sdw::Float deltaT;
	sdw::Float lightsSpeed;
	sdw::UInt raysCount;
	sdw::Float raySize;
	sdw::Float airRefractionScale;
	sdw::Float sphereRefractionScale;

private:
	// In order to prevent users to add members to your struct, I advise you to
	// hide those:
	using sdw::StructInstance::getMember;
	using sdw::StructInstance::getMemberArray;
};

// Optional macro for parameter types definitions.
Writer_Parameter(Ray);
Writer_Parameter(SceneStruct);

// Let's create an enumerator for our custom type (we may need more than one
// custom type, right ?) This enumeration must continue the values from
// ast::type::Kind
enum class TypeName
{
	eRay = int(ast::type::Kind::eCount),
	eSceneStruct,
};
}  // namespace shader

namespace sdw
{
// Add the specialization of sdw::TypeTraits, for our type
template <>
struct TypeTraits<shader::Ray>
{
	static ast::type::Kind constexpr TypeEnum =
	    ast::type::Kind(shader::TypeName::eRay);
};
template <>
struct TypeTraits<shader::SceneStruct>
{
	static ast::type::Kind constexpr TypeEnum =
	    ast::type::Kind(shader::TypeName::eSceneStruct);
};
}  // namespace sdw

namespace cdm
{
static constexpr float Pi{ 3.14159265359f };

template <typename T>
class LightTransportShaderLib : public T
{
public:
	const sdw::Float Pi;

	const sdw::Float Height;
	const sdw::Float Width;

	// sdw::Ubo ubo;

	sdw::Function<sdw::Float, sdw::InOutFloat> randomFloat;
	// sdw::Function<sdw::Mat3, sdw::InVec3> rotationMatrix;
	// sdw::Function<sdw::Float, sdw::InVec3> maxV;
	// sdw::Function<sdw::Vec3, sdw::InOutFloat> randomDir;
	// sdw::Function<sdw::Vec3, sdw::InVec3> backgroundColor;
	// sdw::Function<sdw::Float, sdw::InVec3, sdw::OutVec3, sdw::OutVec3>
	//    distanceEstimation;
	// sdw::Function<sdw::Vec3, sdw::InVec3, sdw::InOutFloat> directLight;
	// sdw::Function<sdw::Vec3, sdw::InVec3, sdw::InVec3, sdw::InOutFloat>
	//    pathTrace;
	// sdw::Function<sdw::Vec2, sdw::InInt, sdw::InFloat, sdw::InOutFloat>
	//    sampleAperture;

	LightTransportShaderLib(LightTransport::Config const& config)
	    : T(),
	      Pi(declConstant<sdw::Float>("Pi", sdw::Float{ cdm::Pi })),
	      Width(declConstant<sdw::Float>("Width", sdw::Float{ widthf })),
	      Height(declConstant<sdw::Float>("Height", sdw::Float{ heightf }))
	//, ubo(sdw::Ubo(*this, "ubo", 1, 0))
	{
		using namespace sdw;

		//(void)ubo.declMember<Vec3>("spherePos");
		//(void)ubo.declMember<Float>("sphereRadius");
		//(void)ubo.declMember<Float>("seed");
		//(void)ubo.declMember<Int>("raysCount");
		// ubo.end();

		randomFloat = implementFunction<Float>(
		    "randomFloat",
		    [&](Float& seed) {
			    auto res = declLocale("res", fract(sin(seed) * 43758.5453_f));
			    seed = seed + 1.0_f;
			    returnStmt(res);
		    },
		    InOutFloat{ *this, "seed" });

		// rotationMatrix = implementFunction<Mat3>(
		//    "rotationMatrix",
		//    [&](const Vec3& rotEuler) {
		//      auto c = declLocale("c", cos(rotEuler.x()));
		//      auto s = declLocale("s", sin(rotEuler.x()));
		//      auto rx = declLocale(
		//          "rx", mat3(vec3(1.0_f, 0.0_f, 0.0_f), vec3(0.0_f, c, -s),
		//                     vec3(0.0_f, s, c)));
		//      rx = transpose(rx);
		//      c = cos(rotEuler.y());
		//      s = sin(rotEuler.y());
		//      auto ry = declLocale(
		//          "ry", mat3(vec3(c, 0.0_f, -s), vec3(0.0_f, 1.0_f, 0.0_f),
		//                     vec3(s, 0.0_f, c)));
		//      ry = transpose(ry);
		//      c = cos(rotEuler.z());
		//      s = sin(rotEuler.z());
		//      auto rz = declLocale(
		//          "rz", mat3(vec3(c, -s, 0.0_f), vec3(s, c, 0.0_f),
		//                     vec3(0.0_f, 0.0_f, 1.0_f)));
		//      rz = transpose(rz);
		//      returnStmt(rz * rx * ry);
		//    },
		//    InVec3{ *this, "rotEuler" });

		// randomDir = implementFunction<Vec3>(
		//    "randomDir",
		//    [&](Float& seed) {
		//      returnStmt(
		//          vec3(1.0_f, 0.0_f, 0.0_f) *
		//          rotationMatrix(vec3(randomFloat(seed) * 2.0_f * Pi, 0.0_f,
		//                              randomFloat(seed) * 2.0_f * Pi)));
		//    },
		//    InOutFloat{ *this, "seed" });
	}
};

void LightTransport::createShaderModules()
{
	auto& vk = rw.get().device();

	VertexShaderHelperResult vertexResult;
	FragmentShaderHelperResult fragmentResult;

	VertexShaderHelperResult blitVertexResult;
	FragmentShaderHelperResult blitFragmentResult;

#pragma region vertexShader
	{
		using namespace sdw;

		VertexWriter writer;

		auto inPosition = writer.declInput<Vec2>("inPosition", 0);
		auto inDirection = writer.declInput<Vec2>("inDirection", 1);
		auto inColor = writer.declInput<Vec4>("inColor", 2);

		auto fragPosition = writer.declOutput<Vec4>("fragPosition", 0u);
		auto fragColor = writer.declOutput<Vec4>("fragColor", 1u);

		auto out = writer.getOut();

		writer.implementMain([&]() {
			auto invAspect = Float(heightf / widthf);
			auto biasCorrection = clamp(length(inDirection)/max(abs(inDirection.x()), abs(inDirection.y())), 1.0_f, 1.414214_f);

			fragPosition = vec4(inPosition.x() * invAspect, inPosition.y(), 0.0_f, 1.0_f);
			//fragPosition = vec4(inPosition.x(), inPosition.y(), 0.0_f, 1.0_f);
			out.vtx.position = fragPosition;

			//fragColor = inColor * biasCorrection;
			fragColor = inColor;
			//fragColor = vec4(biasCorrection, biasCorrection, biasCorrection, 1.0_f);
		});

		vertexResult = writer.createHelperResult(vk);
	}
#pragma endregion

#pragma region fragmentShader
	{
		using namespace sdw;
		LightTransportShaderLib<FragmentWriter> writer(m_config);

		auto in = writer.getIn();

		auto inPosition = writer.declInput<Vec4>("inPosition", 0u);
		auto inColor = writer.declInput<Vec4>("inColor", 1u);

		auto fragColor = writer.declOutput<Vec4>("fragColor", 0u);
		// auto fragColorHDR = writer.declOutput<Vec4>("fragColorHDR", 1u);

		// auto kernelStorageImage =
		//    writer.declImage<RWFImg2DRgba32>("kernelStorageImage", 0, 1);
		//
		// auto kernelImage =
		//    writer.declSampledImage<FImg2DRgba32>("kernelImage", 1, 1);

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
		                bloomV3 += sdw::textureLod(
		                               kernelImage,
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
			// auto uv = writer.declLocale(
			//    "uv", in.fragCoord.xy() / vec2(1280.0_f, 720.0_f));
			//
			// auto col =
			//    writer.declLocale("col", sdw::texture(kernelImage,
			//    uv).rgb());
			//
			// auto bloomAscale1 = writer.declLocale(
			//    "BloomAscale1", writer.ubo.getMember<Float>("bloomAscale1"));
			// auto bloomAscale2 = writer.declLocale(
			//    "BloomAscale2", writer.ubo.getMember<Float>("bloomAscale2"));
			// auto bloomBscale1 = writer.declLocale(
			//    "BloomBscale1", writer.ubo.getMember<Float>("bloomBscale1"));
			// auto bloomBscale2 = writer.declLocale(
			//    "BloomBscale2", writer.ubo.getMember<Float>("bloomBscale2"));
			//
			// auto bloomA = writer.declLocale(
			//    "bloomA",
			//    bloom(bloomAscale1 * writer.Height, 0.0_f,
			//    in.fragCoord.xy()));
			// auto bloomB = writer.declLocale(
			//    "bloomB",
			//    bloom(bloomBscale1 * writer.Height, 0.0_f,
			//    in.fragCoord.xy()));
			//
			//// auto bloomA = writer.declLocale(
			//    "bloomA", bloom(0.15_f, 0.0_f, in.fragCoord.xy()));
			// auto bloomB = writer.declLocale(
			//    "bloomB", bloom(0.05_f, 0.0_f, in.fragCoord.xy()));
			//
			// auto bloomSum =
			//    writer.declLocale("bloomSum", bloomA * vec3(bloomAscale2) +
			//                                      bloomB *
			//                                      vec3(bloomBscale2));

			fragColor = inColor;
			// fragColor = vec4(ACESFilm(col), 1.0_f);
			// fragColor = vec4(ACESFilm(col + bloomSum), 1.0_f);
		});

		fragmentResult = writer.createHelperResult(vk);
	}
#pragma endregion

#pragma region blit vertexShader
	{
		using namespace sdw;

		VertexWriter writer;

		auto fragUV = writer.declOutput<Vec2>("fragUV", 0u);

		auto in = writer.getIn();
		auto out = writer.getOut();

		// auto vertices = writer.declConstantArray<Vec4>("vertices", {
		//	vec4( 3.0_f, -1.0_f, 0.0_f, 1.0_f),
		//	vec4(-1.0_f, -1.0_f, 0.0_f, 1.0_f),
		//	vec4(-1.0_f,  3.0_f, 0.0_f, 1.0_f),
		//	}
		//);
		//
		// auto uvs = writer.declConstantArray<Vec2>("uvs", {
		//	vec2(2.0_f, 0.0_f),
		//	vec2(0.0_f, 0.0_f),
		//	vec2(0.0_f, 2.0_f),
		//	}
		//);

		//auto vertices = writer.declConstantArray<Vec4>(
		//    "vertices", {
		//                    vec4(0.0_f, -0.5_f, -1.0_f, 1.0_f),
		//                    vec4(0.5_f, 0.5_f, 1.0_f, 1.0_f),
		//                    vec4(-0.5_f, 0.5_f, 0.0_f, 1.0_f),
		//                });
		//
		//auto uvs =
		//    writer.declConstantArray<Vec2>("uvs", {
		//                                              vec2(2.0_f, 0.0_f),
		//                                              vec2(0.0_f, 0.0_f),
		//                                              vec2(0.0_f, 2.0_f),
		//                                          });

		writer.implementMain([&]() {
			//fragUV = uvs[in.vertexIndex];
			//out.vtx.position = vertices[in.vertexIndex];
			 fragUV = vec2(writer.cast<Float>((in.vertexIndex << 1) & 2),
			              writer.cast<Float>(in.vertexIndex & 2));
			 out.vtx.position = vec4(fragUV * 2.0f - 1.0f, 0.0f, 1.0f);
		});

		blitVertexResult = writer.createHelperResult(vk);
	}
#pragma endregion

#pragma region blit fragmentShader
	{
		using namespace sdw;

		FragmentWriter writer;

		auto in = writer.getIn();

		auto inUV = writer.declInput<Vec2>("inUV", 0u);

		auto fragColor = writer.declOutput<Vec4>("fragColor", 0u);

		auto HDRImage =
		    writer.declSampledImage<FImg2DRgba32>("HDRImage", 0, 0);

		//*
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

		/*
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
		                bloomV3 += sdw::textureLod(
		                               kernelImage,
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

		auto boxFilter = writer.implementFunction<Vec4>("boxFilter", [&](const Vec2& uv)
			{
				Vec2 textureStep = writer.declConstant<Vec2>("textureStep", vec2(Float(1.0f / widthf), Float(1.0f / heightf)));

				Vec2 uv1 = writer.declLocale<Vec2>("uv1", uv + vec2(-1.0_f, -1.0_f) * textureStep);
				Vec2 uv2 = writer.declLocale<Vec2>("uv2", uv + vec2( 0.0_f, -1.0_f) * textureStep);
				Vec2 uv3 = writer.declLocale<Vec2>("uv3", uv + vec2( 1.0_f, -1.0_f) * textureStep);
									  
				Vec2 uv4 = writer.declLocale<Vec2>("uv4", uv + vec2(-1.0_f,  0.0_f) * textureStep);
				Vec2 uv5 = writer.declLocale<Vec2>("uv5", uv + vec2( 0.0_f,  0.0_f) * textureStep);
				Vec2 uv6 = writer.declLocale<Vec2>("uv6", uv + vec2( 1.0_f,  0.0_f) * textureStep);
									  
				Vec2 uv7 = writer.declLocale<Vec2>("uv7", uv + vec2(-1.0_f,  1.0_f) * textureStep);
				Vec2 uv8 = writer.declLocale<Vec2>("uv8", uv + vec2( 0.0_f,  1.0_f) * textureStep);
				Vec2 uv9 = writer.declLocale<Vec2>("uv9", uv + vec2( 1.0_f,  1.0_f) * textureStep);

				writer.returnStmt<Vec4>((
					texture(HDRImage, uv1) +
					texture(HDRImage, uv2) +
					texture(HDRImage, uv3) +
					texture(HDRImage, uv4) +
					texture(HDRImage, uv5) * 2.0_f +
					texture(HDRImage, uv6) +
					texture(HDRImage, uv7) +
					texture(HDRImage, uv8) +
					texture(HDRImage, uv9)
					) * Float(1.0f / 9.0f));

				//fragColor = texture(HDRImage, inUV);
			}, InVec2{ writer, "uv" });

		writer.implementMain([&]() {

			//Vec2 textureStep = writer.declConstant<Vec2>("textureStep", vec2(Float(1.0f / widthf), Float(1.0f / heightf)));

			fragColor = boxFilter(inUV) * Float(1.0f / float(VERTEX_BUFFER_LINE_COUNT));

			//fragColor = texture(HDRImage, inUV);
			//fragColor = vec4(inUV, 0.0_f, 1.0_f);
			//fragColor = vec4(ACESFilm(fragColor.rgb()), fragColor.a());
		});

		blitFragmentResult = writer.createHelperResult(vk);
	}
#pragma endregion

	GraphicsPipelineFactory factory(vk);
	// factory.addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
	// factory.addDynamicState(VK_DYNAMIC_STATE_SCISSOR);
	factory.clearDynamicStates();
	factory.setViewport(0.0f, 0.0f, widthf, heightf, 0.0f, 1.0f);
	factory.setScissor(0, 0, width, height);

	{
		auto [pipelineLayout, descriptorSetLayouts] =
		    factory.createLayout(vertexResult, fragmentResult);

		m_pipelineLayout = std::move(pipelineLayout);
		m_setLayouts = std::move(descriptorSetLayouts);
	}

	factory.setShaderModules(vertexResult.module, fragmentResult.module);
	factory.setVertexInputHelper(vertexResult.vertexInputHelper);
	factory.setRenderPass(m_renderPass);
	factory.setLayout(m_pipelineLayout);
	factory.clearColorBlendAttachmentStates();
	vk::PipelineColorBlendAttachmentState blendState(true);
    //blendState.blendEnable = true;
	//blendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	//blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	factory.addColorBlendAttachmentState(blendState);
	factory.setTopology(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
	m_pipeline = factory.createPipeline();

	{
		auto [pipelineLayout, descriptorSetLayouts] =
		    factory.createLayout(blitVertexResult, blitFragmentResult);

		m_blitPipelineLayout = std::move(pipelineLayout);
		m_blitSetLayouts = std::move(descriptorSetLayouts);
	}

	factory.setShaderModules(blitVertexResult.module,
	                         blitFragmentResult.module);
	factory.setVertexInputHelper(blitVertexResult.vertexInputHelper);
	factory.setRenderPass(m_blitRenderPass);
	factory.setLayout(m_blitPipelineLayout);
	factory.clearColorBlendAttachmentStates();
	factory.addColorBlendAttachmentState(
	    vk::PipelineColorBlendAttachmentState());
	factory.setTopology(
	    VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	m_blitPipeline = factory.createPipeline();
}
}  // namespace cdm
