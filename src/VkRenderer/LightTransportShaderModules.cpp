#include "LightTransport.hpp"
#include "MyShaderWriter.hpp"
#include "PipelineFactory.hpp"
#include "cdm_maths.hpp"

#include <CompilerGLSL/compileGlsl.hpp>
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
		  polarDirection{ getMember<sdw::Float>("polarDirection") },
		  waveLength{ getMember<sdw::Float>("waveLength") },
		  frequency{ getMember<sdw::Float>("frequency") },
		  rng{ getMember<sdw::Float>("rng") }
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
			result->declMember("polarDirection", ast::type::Kind::eFloat);
			result->declMember("waveLength", ast::type::Kind::eFloat);
			result->declMember("frequency", ast::type::Kind::eFloat);
			result->declMember("rng", ast::type::Kind::eFloat);
		}

		return result;
	}

	sdw::Vec2 position;
	sdw::Vec2 direction;
	sdw::Float polarDirection;
	sdw::Float waveLength;
	sdw::Float frequency;
	sdw::Float rng;

private:
	using sdw::StructInstance::getMember;
	using sdw::StructInstance::getMemberArray;
};

struct RayVertices : public sdw::StructInstance
{
	RayVertices(ast::Shader* shader, ast::expr::ExprPtr expr)
		: StructInstance{ shader, std::move(expr) },
		  posA{ getMember<sdw::Vec2>("posA") },
		  dirA{ getMember<sdw::Vec2>("dirA") },
		  colA{ getMember<sdw::Vec4>("colA") },
		  posB{ getMember<sdw::Vec2>("posB") },
		  dirB{ getMember<sdw::Vec2>("dirB") },
		  colB{ getMember<sdw::Vec4>("colB") }
	{
	}

	RayVertices& operator=(RayVertices const& rhs)
	{
		sdw::StructInstance::operator=(rhs);
		return *this;
	}

	static ast::type::StructPtr makeType(ast::type::TypesCache& cache)
	{
		auto result =
			cache.getStruct(ast::type::MemoryLayout::eStd140, "RayVertices");

		if (result->empty())
		{
			result->declMember("posA", ast::type::Kind::eVec2F);
			result->declMember("dirA", ast::type::Kind::eVec2F);
			result->declMember("colA", ast::type::Kind::eVec4F);
			result->declMember("posB", ast::type::Kind::eVec2F);
			result->declMember("dirB", ast::type::Kind::eVec2F);
			result->declMember("colB", ast::type::Kind::eVec4F);
		}

		return result;
	}

	sdw::Vec2 posA;
	sdw::Vec2 dirA;
	sdw::Vec4 colA;
	sdw::Vec2 posB;
	sdw::Vec2 dirB;
	sdw::Vec4 colB;

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

Writer_Parameter(Ray);
Writer_Parameter(RayVertices);
Writer_Parameter(SceneStruct);

enum class TypeName
{
	eRay = int(ast::type::Kind::eCount),
	eRayVertices,
	eSceneStruct,
};
}  // namespace shader

namespace sdw
{
template <>
struct TypeTraits<shader::Ray>
{
	static ast::type::Kind constexpr TypeEnum =
		ast::type::Kind(shader::TypeName::eRay);
};
template <>
struct TypeTraits<shader::RayVertices>
{
	static ast::type::Kind constexpr TypeEnum =
		ast::type::Kind(shader::TypeName::eRayVertices);
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

	ComputeShaderHelperResult traceComputeResult;

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

			auto biasCorrection = writer.declLocale(
				"biasCorrection",
				1.0_f / max(abs(inDirection.x()), abs(inDirection.y())));

			fragPosition =
				// vec4(inPosition.x() * invAspect, inPosition.y(),
				// 0.0_f, 1.0_f);
				vec4(((inPosition.x() / Float(widthf)) * 2.0_f - 1.0_f),
					 ((inPosition.y() / Float(heightf)) * 2.0_f - 1.0_f),
					 0.0_f, 1.0_f);

			out.vtx.position = fragPosition;

			//fragColor = inColor * biasCorrection;
			fragColor = inColor;
			fragColor.a() = 1.0_f;
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

		writer.implementMain([&]() {
			fragColor = inColor;
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

		// auto vertices = writer.declConstantArray<Vec4>(
		//    "vertices", {
		//                    vec4(0.0_f, -0.5_f, -1.0_f, 1.0_f),
		//                    vec4(0.5_f, 0.5_f, 1.0_f, 1.0_f),
		//                    vec4(-0.5_f, 0.5_f, 0.0_f, 1.0_f),
		//                });
		//
		// auto uvs =
		//    writer.declConstantArray<Vec2>("uvs", {
		//                                              vec2(2.0_f, 0.0_f),
		//                                              vec2(0.0_f, 0.0_f),
		//                                              vec2(0.0_f, 2.0_f),
		//                                          });

		writer.implementMain([&]() {
			// fragUV = uvs[in.vertexIndex];
			// out.vtx.position = vertices[in.vertexIndex];
			fragUV = vec2(writer.cast<Float>((in.vertexIndex << 1) & 2),
						  writer.cast<Float>(in.vertexIndex & 2));
			out.vtx.position = vec4(fragUV * 2.0f - 1.0f, 0.0f, 1.0f);
			fragUV = vec2(fragUV.x(), 1.0_f - fragUV.y());
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

		auto boxFilter = writer.implementFunction<Vec4>(
			"boxFilter",
			[&](const Vec2& uv) {
				Vec2 textureStep = writer.declConstant<Vec2>(
					"textureStep", vec2(Float(1.0f / (widthf * HDR_SCALE)),
										Float(1.0f / (heightf * HDR_SCALE))));

				Vec2 uv1 = writer.declLocale<Vec2>(
					"uv1", uv + vec2(-1.0_f, -1.0_f) * textureStep);
				Vec2 uv2 = writer.declLocale<Vec2>(
					"uv2", uv + vec2(0.0_f, -1.0_f) * textureStep);
				Vec2 uv3 = writer.declLocale<Vec2>(
					"uv3", uv + vec2(1.0_f, -1.0_f) * textureStep);

				Vec2 uv4 = writer.declLocale<Vec2>(
					"uv4", uv + vec2(-1.0_f, 0.0_f) * textureStep);
				Vec2 uv5 = writer.declLocale<Vec2>(
					"uv5", uv + vec2(0.0_f, 0.0_f) * textureStep);
				Vec2 uv6 = writer.declLocale<Vec2>(
					"uv6", uv + vec2(1.0_f, 0.0_f) * textureStep);

				Vec2 uv7 = writer.declLocale<Vec2>(
					"uv7", uv + vec2(-1.0_f, 1.0_f) * textureStep);
				Vec2 uv8 = writer.declLocale<Vec2>(
					"uv8", uv + vec2(0.0_f, 1.0_f) * textureStep);
				Vec2 uv9 = writer.declLocale<Vec2>(
					"uv9", uv + vec2(1.0_f, 1.0_f) * textureStep);

				writer.returnStmt<Vec4>(
			        (HDRImage.sample(uv1) + HDRImage.sample(uv2) +
			         HDRImage.sample(uv3) + HDRImage.sample(uv4) +
			         HDRImage.sample(uv5) * 1.0_f + HDRImage.sample(uv6) +
			         HDRImage.sample(uv7) + HDRImage.sample(uv8) +
					 HDRImage.sample(uv9)) *
					Float(1.0f / 9.0f));

				// fragColor = texture(HDRImage, inUV);
			},
			InVec2{ writer, "uv" });

		auto pcb = writer.declPushConstantsBuffer("pcb");
		pcb.declMember<Float>("exposure");
		pcb.end();

		writer.implementMain([&]() {
			fragColor = HDRImage.sample(inUV);
			 //fragColor = boxFilter(inUV);

			fragColor =
				vec4(pow((fragColor.rgb() * pcb.getMember<Float>("exposure")),
						 vec3(Float(1.0f / 2.2f))),
					 1.0_f);
		});

		blitFragmentResult = writer.createHelperResult(vk);
	}
#pragma endregion

#pragma region trace comopute shader
	{
		using namespace sdw;

		LightTransportShaderLib<ComputeWriter> writer(m_config);

		writer.declType<shader::Ray>();
		ast::type::StructPtr rayType =
			shader::Ray::makeType(writer.getTypesCache());
		ArraySsboT<shader::Ray> raysSsbo{ writer, "raysSsbo", rayType, 0, 0 };
		writer.addDescriptor(0, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

		writer.declType<shader::RayVertices>();
		ast::type::StructPtr rayVerticesType =
			shader::RayVertices::makeType(writer.getTypesCache());
		// Struct type{ writer, rayType };
		ArraySsboT<shader::RayVertices> rayVerticesSsbo{
			writer, "rayVerticesSsbo", rayVerticesType, 1, 0
		};
		writer.addDescriptor(1, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

		auto in = writer.getIn();

		auto pcb =
			writer.declPushConstantsBuffer("pcb"
										   //, ast::type::MemoryLayout::eStd140
			);
		m_pc.addPcbMembers(pcb);

		auto sampleVisibleNormal = writer.implementFunction<Float>(
			"sampleVisibleNormal",
			[&](const Float& s, const Float& xi, const Float& thetaMin,
				const Float& thetaMax) {
				auto a = writer.declLocale<Float>(
					"a", tanh(thetaMin / (2.0_f * s)));
				auto b = writer.declLocale<Float>(
					"b", tanh(thetaMax / (2.0_f * s)));
				writer.returnStmt(2.0_f * s * atanh(a + (b - a) * xi));
				writer.returnStmt(2.0_f * s * atanh((1.0_f - xi) * a + xi * b));
			},
			InFloat{ writer, "s" }, InFloat{ writer, "xi" },
			InFloat{ writer, "thetaMin" }, InFloat{ writer, "thetaMax" });

		auto sampleRoughMirror = writer.implementFunction<Vec2>(
			"sampleRoughMirror",
			[&](const Vec2& w_o, const Float& s, Float& rng) {
				auto thetaMin = writer.declLocale<Float>(
					"thetaMin",
					max(asin(w_o.x()), 0.0_f) - (writer.Pi / 2.0_f));
				auto thetaMax = writer.declLocale<Float>(
					"thetaMax",
					min(asin(w_o.x()), 0.0_f) + (writer.Pi / 2.0_f));

				auto thetaM = writer.declLocale<Float>(
					"thetaM", sampleVisibleNormal(s, writer.randomFloat(rng),
												  thetaMin, thetaMax));

				auto m = writer.declLocale<Vec2>(
					"m", vec2(sin(thetaM), cos(thetaM)));

				writer.returnStmt(m * (dot(w_o, m) * 2.0_f) - w_o);
			},
			InVec2{ writer, "w_o" }, InFloat{ writer, "s" },
			InOutFloat{ writer, "rng" });

		auto intersectPlane = writer.implementFunction<Void>(
			"intersectPlane",
			[&](const Vec2& a, const Vec2& b, const Vec2& pos, const Vec2& dir,
				Float& tMin, Float& tMax, Vec2& normal) {
				// auto a = writer.declLocale<Vec2>("a", vec2(0.0_f,
				// Float(heightf))); auto b = writer.declLocale<Vec2>("b",
				// vec2(Float(widthf), Float(heightf)));

				auto sT = writer.declLocale<Vec2>("sT", b - a);
				auto sN = writer.declLocale<Vec2>("sN", vec2(-sT.y(), sT.x()));

				auto t = writer.declLocale<Float>(
					"t", dot(sN, a - pos) / dot(sN, dir));
				auto u =
					writer.declLocale<Float>("u", dot(sT, pos + dir * t - a));

				IF(writer, t < tMin || t >= tMax || u < 0.0 || u > dot(sT, sT))
				{
					writer.returnStmt();
				}
				FI;

				tMax = t;
				normal = normalize(sN);
				// mat = matId;
			},
			InVec2{ writer, "a" }, InVec2{ writer, "b" },
			InVec2{ writer, "pos" }, InVec2{ writer, "dir" },
			InOutFloat{ writer, "tMin" }, InOutFloat{ writer, "tMax" },
			InOutVec2{ writer, "normal" });

		writer.inputLayout(THREAD_COUNT);
		writer.implementMain([&]() {
			Int init = pcb.getMember<Int>("init");
			Float rng =
				writer.declLocale<Float>("seed", pcb.getMember<Float>("rng"));
			Float s = writer.declLocale<Float>("s", m_pc.getSmoothness(pcb));

			Locale(bumpsCount, m_pc.getBumpsCount(pcb));

			UInt rayIndex =
				writer.declLocale("rayIndex", in.globalInvocationID.x());
			UInt vertexIndex = writer.declLocale(
				"vertexIndex", rayIndex * m_pc.getBumpsCount(pcb));
			Float indexf =
				writer.declLocale("indexf", writer.cast<Float>(rayIndex));

			Float saltedrng =
				writer.declLocale<Float>("saltedrng", rng + indexf);

			auto& ray = raysSsbo[rayIndex];

			IF(writer, init == 1_i)
			{
				auto& vertex = rayVerticesSsbo[vertexIndex];
				 vertex.colA = vec4(1.0_f);
				 vertex.colB = vec4(1.0_f);
			}
			ELSE
			{
				/*
				{
					auto& vertex = rayVerticesSsbo[vertexIndex];

					Float tMin = writer.declLocale("tMin", 1.0e-4_f);
					Float tMax = writer.declLocale("tMax", 1.0e30_f);
					Vec2 normal = writer.declLocale("normal", vec2(0.0_f));
					// Float polarNormal = writer.declLocale("polarNormal",
					// 0.0_f);
					Locale(polarNormal, 0.0_f);
					Locale(polarDirection, 0.0_f);
					Locale(cartesianDirection, vec2(0.0_f));

					auto a = writer.declLocale<Vec2>("a", vec2(0.0_f, 0.0_f));
					auto b = writer.declLocale<Vec2>(
						"b", vec2(Float(widthf), 0.0_f));
					auto c = writer.declLocale<Vec2>(
						"c", vec2(Float(widthf), Float(heightf)));
					auto d = writer.declLocale<Vec2>(
						"d", vec2(0.0_f, Float(heightf)));

					intersectPlane(a, b, ray.position, ray.direction, tMin,
								   tMax, normal);
					intersectPlane(b, c, ray.position, ray.direction, tMin,
								   tMax, normal);
					intersectPlane(c, d, ray.position, ray.direction, tMin,
								   tMax, normal);
					intersectPlane(d, a, ray.position, ray.direction, tMin,
								   tMax, normal);

					IF(writer, tMax == 1.0e30_f)
					{
						vertex.posA = ray.position;
						vertex.posB = ray.position + ray.direction * tMax;
						vertex.dirA = ray.direction;
						vertex.dirB = ray.direction;
					}
					ELSE
					{
						vertex.posA = ray.position;
						vertex.posB = ray.position + ray.direction * tMax;
						vertex.dirA = ray.direction;
						vertex.dirB = ray.direction;
						vertex.colA = vertex.colA * 0.9_f;
						vertex.colB = vertex.colB * 0.9_f;

						ray.position = vertex.posB;

						polarNormal = atan2(normal.y(), normal.x());
						polarDirection =
							atan2(ray.direction.y(), ray.direction.x());
						polarDirection = polarDirection - polarNormal;
						cartesianDirection =
							vec2(cos(polarDirection), sin(polarDirection));

						Float r = writer.declLocale<Float>("r", ray.rng);

						Locale(newDirection,
							   sampleRoughMirror(cartesianDirection, s, r));
						polarDirection =
							atan2(newDirection.y(), newDirection.x());
						polarDirection = polarDirection + polarNormal;

						ray.direction =
							vec2(cos(polarDirection), sin(polarDirection));

						ray.rng = r;
					}
					FI;
				}

				vertexIndex++;
				//*/

				Float tMin = writer.declLocale("tMin", 1.0e-4_f);
				Float tMax = writer.declLocale("tMax", 1.0e30_f);
				Vec2 normal = writer.declLocale("normal", vec2(0.0_f));

				FOR(writer, UInt, i, vertexIndex, i < vertexIndex + bumpsCount, i++)
				{
					//auto& previousVertex = rayVerticesSsbo[i - 1_u];
					auto& vertex = rayVerticesSsbo[i];

					tMin = 1.0e-4_f;
					tMax = 1.0e30_f;
					normal = vec2(0.0_f);

					// Float polarNormal = writer.declLocale("polarNormal",
					// 0.0_f);
					Locale(polarNormal, 0.0_f);
					Locale(polarDirection, 0.0_f);
					Locale(cartesianDirection, vec2(0.0_f));
					Locale(rayPosition, ray.position);
					Locale(rayDirection, ray.direction);

					auto a = writer.declLocale<Vec2>("a", vec2(0.0_f, 0.0_f));
					auto b = writer.declLocale<Vec2>(
						"b", vec2(Float(widthf), 0.0_f));
					auto c = writer.declLocale<Vec2>(
						"c", vec2(Float(widthf), Float(heightf)));
					auto d = writer.declLocale<Vec2>(
						"d", vec2(0.0_f, Float(heightf)));

					intersectPlane(a, b, rayPosition, rayDirection, tMin, tMax, normal);
					intersectPlane(b, c, rayPosition, rayDirection, tMin, tMax, normal);
					intersectPlane(c, d, rayPosition, rayDirection, tMin, tMax, normal);
					intersectPlane(d, a, rayPosition, rayDirection, tMin, tMax, normal);

					IF(writer, tMax == 1.0e30_f)
					{
						vertex.posA = rayPosition;
						vertex.posB = rayPosition + rayDirection * tMax;
						vertex.dirA = rayDirection;
						vertex.dirB = rayDirection;
					}
					ELSE
					{
						vertex.posA = rayPosition;
						vertex.posB = rayPosition + rayDirection * tMax;
						vertex.dirA = rayDirection;
						vertex.dirB = rayDirection;
						vertex.colA = vec4(1.0_f);//vertex.colA;// * 0.9_f;
						vertex.colB = vec4(1.0_f);//vertex.colB;// * 0.9_f;

						ray.position = vertex.posB;

						polarNormal = atan2(normal.y(), normal.x());
						//polarDirection = atan2(rayDirection.y(), rayDirection.x());
						polarDirection = ray.polarDirection;
						polarDirection = polarDirection - polarNormal;
						cartesianDirection = vec2(cos(polarDirection), sin(polarDirection));

						Float r = writer.declLocale<Float>("r", ray.rng);

						Locale(newDirection, sampleRoughMirror(cartesianDirection, s, r));
						polarDirection = atan2(newDirection.y(), newDirection.x());
						polarDirection = polarDirection + polarNormal;

						ray.direction = vec2(cos(polarDirection), sin(polarDirection));
						ray.polarDirection = polarDirection;

						ray.rng = r;
					}
					FI;
				}
				ROF;
			}
			FI;
		});

		// ast::SpecialisationInfo si;
		// glsl::GlslConfig c;
		// std::cout << glsl::compileGlsl(writer.getShader(), si, c) <<
		// std::endl; std::cout << spirv::writeSpirv(writer.getShader()) <<
		// std::endl;

		traceComputeResult = writer.createHelperResult(vk);
	}
#pragma endregion

	GraphicsPipelineFactory factory(vk);
	// factory.addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
	// factory.addDynamicState(VK_DYNAMIC_STATE_SCISSOR);
	factory.clearDynamicStates();
	factory.setViewport(0.0f, 0.0f, widthf * HDR_SCALE, heightf * HDR_SCALE,
						0.0f, 1.0f);
	factory.setScissor(0, 0, uint32_t(width * HDR_SCALE),
	                   uint32_t(height * HDR_SCALE));

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
	// blendState.blendEnable = true;
	// blendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	// blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	factory.addColorBlendAttachmentState(blendState);
	factory.setTopology(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
	m_pipeline = factory.createPipeline();

	{
		std::vector<VkPushConstantRange> pushConstants;

		pushConstants.push_back(
			{ VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, 0,
			  sizeof(float) });

		auto [pipelineLayout, descriptorSetLayouts] = factory.createLayout(
			blitVertexResult, blitFragmentResult, pushConstants);

		m_blitPipelineLayout = std::move(pipelineLayout);
		m_blitSetLayouts = std::move(descriptorSetLayouts);
	}

	factory.setViewport(0.0f, 0.0f, widthf, heightf, 0.0f, 1.0f);
	factory.setScissor(0, 0, width, height);
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

	ComputePipelineFactory cfactory(vk);

	{
		std::vector<VkPushConstantRange> pushConstants;

		pushConstants.push_back(m_pc.getRange());
		//{ VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT, 0,
		// sizeof(int32_t) + sizeof(float) + sizeof(float) });
		// pushConstants.push_back(
		//    { VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT,
		//      sizeof(int32_t), sizeof(float) });

		auto [pipelineLayout, descriptorSetLayouts] =
			cfactory.createLayout(traceComputeResult, pushConstants);

		m_tracePipelineLayout = std::move(pipelineLayout);
		m_traceSetLayouts = std::move(descriptorSetLayouts);
	}

	cfactory.setShaderModule(traceComputeResult.module);
	cfactory.setLayout(m_tracePipelineLayout);

	m_tracePipeline = cfactory.createPipeline();
}
}  // namespace cdm
