#include "LightTransport.hpp"

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
	      currentRefraction{ getMember<sdw::Float>("currentRefraction") }
	{
	}

	Ray& operator=(Ray const& rhs)
	{
		sdw::StructInstance::operator=(rhs);
		return *this;
	}

	static ast::type::StructPtr makeType(ast::type::TypesCache& cache)
	{
		auto result = std::make_unique<ast::type::Struct>(
		    cache, ast::type::MemoryLayout::eStd140, "Ray");

		if (result->empty())
		{
			result->declMember("position", ast::type::Kind::eVec2F);
			result->declMember("direction", ast::type::Kind::eVec2F);
			result->declMember("waveLength", ast::type::Kind::eFloat);
			result->declMember("amplitude", ast::type::Kind::eFloat);
			result->declMember("phase", ast::type::Kind::eFloat);
			result->declMember("currentRefraction", ast::type::Kind::eFloat);
		}

		return result;
	}

	sdw::Vec2 position;
	sdw::Vec2 direction;
	sdw::Float waveLength;
	sdw::Float amplitude;
	sdw::Float phase;
	sdw::Float currentRefraction;

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
	      raysCount{ getMember<sdw::UInt>("raysCount") }
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

void LightTransport::Config::copyTo(void* ptr)
{
	std::memcpy(ptr, this, sizeof(*this));
}

template <typename T>
class MandelbulbShaderLib : public T
{
public:
	const sdw::Float Pi;

	const sdw::Float Height;
	const sdw::Float Width;

	// sdw::Ubo ubo;

	// sdw::Function<sdw::Float, sdw::InOutFloat> randomFloat;
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

	MandelbulbShaderLib(LightTransport::Config const& config)
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

		// randomFloat = implementFunction<Float>(
		//    "randomFloat",
		//    [&](Float& seed) {
		//	    auto res = declLocale("res", fract(sin(seed) * 43758.5453_f));
		//	    seed = seed + 1.0_f;
		//	    returnStmt(res);
		//    },
		//    InOutFloat{ *this, "seed" });

		// rotationMatrix = implementFunction<Mat3>(
		//    "rotationMatrix",
		//    [&](const Vec3& rotEuler) {
		//	    auto c = declLocale("c", cos(rotEuler.x()));
		//	    auto s = declLocale("s", sin(rotEuler.x()));
		//	    auto rx = declLocale(
		//	        "rx", mat3(vec3(1.0_f, 0.0_f, 0.0_f), vec3(0.0_f, c, -s),
		//	                   vec3(0.0_f, s, c)));
		//	    rx = transpose(rx);
		//	    c = cos(rotEuler.y());
		//	    s = sin(rotEuler.y());
		//	    auto ry = declLocale(
		//	        "ry", mat3(vec3(c, 0.0_f, -s), vec3(0.0_f, 1.0_f, 0.0_f),
		//	                   vec3(s, 0.0_f, c)));
		//	    ry = transpose(ry);
		//	    c = cos(rotEuler.z());
		//	    s = sin(rotEuler.z());
		//	    auto rz = declLocale(
		//	        "rz", mat3(vec3(c, -s, 0.0_f), vec3(s, c, 0.0_f),
		//	                   vec3(0.0_f, 0.0_f, 1.0_f)));
		//	    rz = transpose(rz);
		//	    returnStmt(rz * rx * ry);
		//    },
		//    InVec3{ *this, "rotEuler" });

		// randomDir = implementFunction<Vec3>(
		//    "randomDir",
		//    [&](Float& seed) {
		//	    returnStmt(
		//	        vec3(1.0_f, 0.0_f, 0.0_f) *
		//	        rotationMatrix(vec3(randomFloat(seed) * 2.0_f * Pi, 0.0_f,
		//	                            randomFloat(seed) * 2.0_f * Pi)));
		//    },
		//    InOutFloat{ *this, "seed" });
	}
};

struct RayIteration
{
	struct Vec2
	{
		float x = 0;
		float y = 0;
	};
	struct Vec3
	{
		float x = 0;
		float y = 0;
		float z = 0;
		float _ = 0;
	};

	Vec2 position;
	Vec2 direction;

	// Color		Wavelength	Frequency
	// Violet	380–450 nm	680–790 THz
	// Blue		450–485 nm	620–680 THz
	// Cyan		485–500 nm	600–620 THz
	// Green	500–565 nm	530–600 THz
	// Yellow	565–590 nm	510–530 THz
	// Orange	590–625 nm	480–510 THz
	// Red		625–740 nm	405–480 THz
	float waveLength = 500.0f;
	float amplitude = 1.0f;
	float phase = 0.0f;
	float currentRefraction = 1.0f;
};

LightTransport::LightTransport(RenderWindow& renderWindow)
    : rw(renderWindow),
      gen(rd()),
      dis(0.0f, 0.3f)
{
	auto& vk = rw.get().device();

	vk.setLogActive();

	m_config.spherePos.x = widthf / 2.0f;
	m_config.spherePos.y = heightf / 2.0f + 10.0f;
	m_config.sphereRadius = 50.0f;
	m_config.deltaT = 0.5f;

#pragma region renderPass
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = VkFormat::VK_FORMAT_B8G8R8A8_UNORM;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

	// VkAttachmentDescription colorHDRAttachment = {};
	// colorHDRAttachment.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
	// colorHDRAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// colorHDRAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	// colorHDRAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	// colorHDRAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	// colorHDRAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// colorHDRAttachment.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	// colorHDRAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

	// std::array colorAttachments{ colorAttachment, colorHDRAttachment };
	std::array colorAttachments{ colorAttachment };

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// VkAttachmentReference colorHDRAttachmentRef = {};
	// colorHDRAttachmentRef.attachment = 1;
	// colorHDRAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::array colorAttachmentRefs{

		colorAttachmentRef  //, colorHDRAttachmentRef
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

#pragma region vertexShader
	{
		using namespace sdw;

		VertexWriter writer;

		auto inPosition = writer.declInput<Vec2>("inPosition", 0);
		auto fragColor = writer.declOutput<Vec4>("fragColor", 0u);

		auto out = writer.getOut();

		writer.implementMain([&]() {
			out.vtx.position = vec4(inPosition, 0.0_f, 1.0_f);
			fragColor = vec4(inPosition / 2.0_f + 0.5_f, 0.0_f, 1.0_f);
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
		MandelbulbShaderLib<FragmentWriter> writer{ m_config };

		auto in = writer.getIn();

		auto inPosition = writer.declInput<Vec4>("inPosition", 0u);
		auto fragColor = writer.declOutput<Vec4>("fragColor", 0u);
		// auto fragColorHDR = writer.declOutput<Vec4>("fragColorHDR", 1u);

		auto kernelStorageImage =
		    writer.declImage<RWFImg2DRgba32>("kernelStorageImage", 0, 1);

		auto kernelImage =
		    writer.declSampledImage<FImg2DRgba32>("kernelImage", 1, 1);

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
			auto uv = writer.declLocale(
			    "uv", in.fragCoord.xy() / vec2(1280.0_f, 720.0_f));

			auto col =
			    writer.declLocale("col", sdw::texture(kernelImage, uv).rgb());

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

			fragColor = vec4(ACESFilm(col), 1.0_f);
			// fragColor = vec4(ACESFilm(col + bloomSum), 1.0_f);
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
	}  // namespace cdm
#pragma endregion

#pragma region rayComputeShader
	{
		using namespace sdw;

		MandelbulbShaderLib<ComputeWriter> writer{ m_config };

		auto in = writer.getIn();

		writer.inputLayout(1);

		writer.declType<shader::Ray>();
		writer.declType<shader::SceneStruct>();

		ast::type::StructPtr rayType =
		    shader::Ray::makeType(writer.getTypesCache());
		// rayType->end();
		Struct type{ writer, rayType };

		ArraySsboT<shader::Ray> raysSsbo{ writer, "raysSsbo", rayType, 0, 0 };

		// raysSsbo.declStructMember<shader::Ray>("rays", RAYS_COUNT);
		// raysSsbo.end();
		// raysSsbo.declStructMember<shader::Ray>("rays");

		Ubo sceneUbo{ writer, "sceneUbo", 1, 0,
			          ast::type::MemoryLayout::eStd140 };
		sceneUbo.declStructMember<shader::SceneStruct>("data");
		sceneUbo.end();

		// auto kernelImage =
		//    writer.declImage<RWFImg2DRgba32>("kernelImage", 0, 0);

		// auto kernelSampledImage =
		//    writer.declSampledImage<FImg2DRgba32>("kernelSampledImage", 2,
		//    0);

		writer.implementMain([&]() {
			UInt index = in.globalInvocationID.x();

			auto sceneData = writer.declLocale<shader::SceneStruct>(
			    "sceneData", sceneUbo.getMember<shader::SceneStruct>("data"));

			// auto rays = raysSsbo.getMemberArray<shader::Ray>("rays");
			// auto ray = rays[index];

			auto ray = writer.declLocale<shader::Ray>("ray", raysSsbo[index]);
			// auto ray = raysSsbo.getMember<shader::Ray>("rays");

			ray.position += ray.direction * sceneData.airRefraction *
			                sceneData.lightsSpeed * sceneData.deltaT;

			auto refractionAtPos =
			    writer.declLocale("refractionAtPos", sceneData.airRefraction);

			auto sphereNormal = writer.declLocale(
			    "sphereNormal", ray.position - sceneData.spherePos);
			Float angle = writer.declLocale("angle", 0.0_f);
			Float newAngle = writer.declLocale("newAngle", 0.0_f);

			IF(writer, length(sphereNormal) < sceneData.sphereRadius)
			{
				refractionAtPos = sceneData.sphereRefraction;
			}
			FI;

			IF(writer, ray.currentRefraction != refractionAtPos)
			{
				sphereNormal = normalize(sphereNormal);

				angle = acos(dot(sphereNormal, ray.direction));
				// sin(theta) = n1/n2 sin(i)

				newAngle = asin((ray.currentRefraction / refractionAtPos) *
				                sin(angle));

				ray.direction.x() = cos(newAngle);
				ray.direction.y() = sin(newAngle);
				ray.direction = normalize(ray.direction);

				ray.currentRefraction = refractionAtPos;
			}
			FI;
		});

		std::vector<uint32_t> bytecode =
		    spirv::serialiseSpirv(writer.getShader());

		vk::ShaderModuleCreateInfo createInfo;
		createInfo.codeSize = bytecode.size() * sizeof(*bytecode.data());
		createInfo.pCode = bytecode.data();

		m_rayComputeModule = vk.create(createInfo);
		if (!m_rayComputeModule)
		{
			std::cerr << "error: failed to create ray compute shader module"
			          << std::endl;
			abort();
		}

		vk.debugMarkerSetObjectName(m_rayComputeModule, "rayComputeModule");
	}
#pragma endregion

#pragma region colorComputeShader
	{
		using namespace sdw;

		MandelbulbShaderLib<ComputeWriter> writer{ m_config };

		auto in = writer.getIn();

		writer.inputLayout(8, 8);

		writer.declType<shader::Ray>();
		writer.declType<shader::SceneStruct>();

		sdw::Ssbo raysSsbo{ writer, "raysSsbo", 0, 0,
			                ast::type::MemoryLayout::eStd140 };
		// raysSsbo.declStructMember<shader::Ray>("rays", RAYS_COUNT);
		raysSsbo.declStructMember<shader::Ray>("rays");
		raysSsbo.end();

		sdw::Ubo sceneUbo{ writer, "sceneUbo", 1, 0,
			               ast::type::MemoryLayout::eStd140 };
		sceneUbo.declStructMember<shader::SceneStruct>("data");
		sceneUbo.end();

		auto kernelImage =
		    writer.declImage<RWFImg2DRgba32>("kernelImage", 0, 1);

		auto kernelSampledImage =
		    writer.declSampledImage<FImg2DRgba32>("kernelSampledImage", 1, 1);

		// https://stackoverflow.com/questions/3407942/rgb-values-of-visible-spectrum/22681410#22681410
		auto spectral_color = writer.implementFunction<Vec3>(
		    "spectral_color",
		    [&](const Float& waveLength) {
			    // RGB <0,1> <- lambda waveLength <400,700> [nm]

			    Float t = writer.declLocale<Float>("t", 0.0_f);
			    Vec3 color = writer.declLocale<Vec3>("color", vec3(0.0_f));

			    IF(writer, (waveLength >= 400.0_f) && (waveLength < 410.0_f))
			    {
				    t = (waveLength - 400.0_f) / (410.0_f - 400.0_f);
				    color.r() = +(0.33_f * t) - (0.20_f * t * t);
			    }
			    ELSEIF((waveLength >= 410.0_f) && (waveLength < 475.0_f))
			    {
				    t = (waveLength - 410.0_f) / (475.0_f - 410.0_f);
				    color.r() = 0.14_f - (0.13_f * t * t);
			    }
			    ELSEIF((waveLength >= 545.0_f) && (waveLength < 595.0_f))
			    {
				    t = (waveLength - 545.0_f) / (595.0_f - 545.0_f);
				    color.r() = +(1.98_f * t) - (t * t);
			    }
			    ELSEIF((waveLength >= 595.0_f) && (waveLength < 650.0_f))
			    {
				    t = (waveLength - 595.0_f) / (650.0_f - 595.0_f);
				    color.r() = 0.98_f + (0.06_f * t) - (0.40_f * t * t);
			    }
			    ELSEIF((waveLength >= 650.0_f) && (waveLength < 700.0_f))
			    {
				    t = (waveLength - 650.0_f) / (700.0_f - 650.0_f);
				    color.r() = 0.65_f - (0.84_f * t) + (0.20_f * t * t);
			    }
			    FI;

			    IF(writer, (waveLength >= 415.0_f) && (waveLength < 475.0_f))
			    {
				    t = (waveLength - 415.0_f) / (475.0_f - 415.0_f);
				    color.g() = +(0.80_f * t * t);
			    }
			    ELSEIF((waveLength >= 475.0_f) && (waveLength < 590.0_f))
			    {
				    t = (waveLength - 475.0_f) / (590.0_f - 475.0_f);
				    color.g() = 0.8_f + (0.76_f * t) - (0.80_f * t * t);
			    }
			    ELSEIF((waveLength >= 585.0_f) && (waveLength < 639.0_f))
			    {
				    t = (waveLength - 585.0_f) / (639.0_f - 585.0_f);
				    color.g() = 0.84_f - (0.84_f * t);
			    }
			    FI;

			    IF(writer, (waveLength >= 400.0_f) && (waveLength < 475.0_f))
			    {
				    t = (waveLength - 400.0_f) / (475.0_f - 400.0_f);
				    color.b() = +(2.20_f * t) - (1.50_f * t * t);
			    }
			    ELSEIF((waveLength >= 475.0_f) && (waveLength < 560.0_f))
			    {
				    t = (waveLength - 475.0_f) / (560.0_f - 475.0_f);
				    color.b() = 0.7_f - (t) + (0.30_f * t * t);
			    }
			    FI;

			    writer.returnStmt(color);
		    },
		    sdw::InFloat{ writer, "waveLength" });

		writer.implementMain([&]() {
			Float x = writer.declLocale(
			    "x", writer.cast<Float>(in.globalInvocationID.x()));
			Float y = writer.declLocale(
			    "y", writer.cast<Float>(in.globalInvocationID.y()));

			// Float seed = writer.declLocale(
			//"seed", cos(x) + sin(y) + writer.ubo.getMember<Float>("seed"));
			//"seed", (x + y * widthf) + writer.ubo.getMember<Float>("seed"));

			Vec2 xy = writer.declLocale("xy", vec2(x, y));

			IVec2 iuv = writer.declLocale(
			    "iuv", ivec2(writer.cast<Int>(in.globalInvocationID.x()),
			                 writer.cast<Int>(in.globalInvocationID.y())));

			Vec3 resColor = writer.declLocale("resColor", vec3(0.0_f));

			auto sceneData = sceneUbo.getMember<shader::SceneStruct>("data");

			// auto rays = raysSsbo.getMemberArray<shader::Ray>("rays");
			auto ray = raysSsbo.getMember<shader::Ray>("rays");

			Vec2 rayPos = writer.declLocale<Vec2>("rayPos", vec2(0.0_f));
			Float dist = writer.declLocale<Float>("dist", 0.0_f);

			// FOR(writer, UInt, i, 0_u, i < sceneData.raysCount, i++)
			{
				// rayPos = rays[i].position;
				rayPos = ray.position;
				dist = length(rayPos - xy);
				IF(writer, dist < 2.0_f) { resColor = vec3(1.0_f); }
				FI;
			}
			// ROF;

			auto spherespace = xy - sceneData.spherePos;

			IF(writer, length(spherespace) <= sceneData.sphereRadius)
			{
				resColor = resColor + vec3(0.0_f, 0.0_f, 0.01_f);
			}
			FI;

			Vec4 previousColor = writer.declLocale(
			    "previousColor", imageLoad(kernelImage, iuv));

			// Vec3 mixer = writer.declLocale("mixer", vec3(1.0_f / (samples
			// + 1.0_f))); Vec3 mixedColor = writer.declLocale("mixedColor",
			// mix(previousColor.rgb(), resColor, mixer)); Vec4 mixedColor4 =
			// writer.declLocale("mixedColor4", vec4(mixedColor, samples
			// + 1.0_f));

			// Vec4 previousColor = writer.declLocale(
			//    "previousColor", imageLoad(kernelImage, iuv));

			// Float samples = writer.declLocale(
			//    "samples", writer.ubo.getMember<Float>("samples"));
			//"samples", previousColor.a());

			imageStore(kernelImage, iuv,
			           vec4(previousColor.rgb() + resColor, 1.0_f));
			// vec4(seed));
			// vec4(writer.randomFloat(seed), writer.randomFloat(seed),
			// writer.randomFloat(seed), 1.0_f));
		});

		std::vector<uint32_t> bytecode =
		    spirv::serialiseSpirv(writer.getShader());

		vk::ShaderModuleCreateInfo createInfo;
		createInfo.codeSize = bytecode.size() * sizeof(*bytecode.data());
		createInfo.pCode = bytecode.data();

		m_colorComputeModule = vk.create(createInfo);
		if (!m_colorComputeModule)
		{
			std::cerr << "error: failed to create color compute shader module"
			          << std::endl;
			abort();
		}

		vk.debugMarkerSetObjectName(m_colorComputeModule,
		                            "colorComputeModule");
	}
#pragma endregion

#pragma region compute descriptor pool
	std::array poolSizes{
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
	};

	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.maxSets = 2;
	poolInfo.poolSizeCount = uint32_t(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();

	m_computePool = vk.create(poolInfo);
	if (!m_computePool)
	{
		std::cerr << "error: failed to create compute descriptor pool"
		          << std::endl;
		abort();
	}
#pragma endregion

#pragma region ray compute descriptor set layout
	{
		VkDescriptorSetLayoutBinding layoutBindingSsbo{};
		layoutBindingSsbo.binding = 0;
		layoutBindingSsbo.descriptorCount = 1;
		layoutBindingSsbo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		layoutBindingSsbo.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutBinding layoutBindingUbo{};
		layoutBindingUbo.binding = 1;
		layoutBindingUbo.descriptorCount = 1;
		layoutBindingUbo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBindingUbo.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		std::array layoutBindings{ layoutBindingSsbo, layoutBindingUbo };

		vk::DescriptorSetLayoutCreateInfo setLayoutInfo;
		setLayoutInfo.bindingCount = uint32_t(layoutBindings.size());
		setLayoutInfo.pBindings = layoutBindings.data();

		m_rayComputeSetLayout = vk.create(setLayoutInfo);
		if (!m_rayComputeSetLayout)
		{
			std::cerr << "error: failed to create ray compute set layout"
			          << std::endl;
			abort();
		}
	}
#pragma endregion

#pragma region color compute descriptor set layout
	{
		VkDescriptorSetLayoutBinding layoutBindingKernelImage{};
		layoutBindingKernelImage.binding = 0;
		layoutBindingKernelImage.descriptorCount = 1;
		layoutBindingKernelImage.descriptorType =
		    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		layoutBindingKernelImage.stageFlags =
		    VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding layoutBindingImage{};
		layoutBindingImage.binding = 1;
		layoutBindingImage.descriptorCount = 1;
		layoutBindingImage.descriptorType =
		    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindingImage.stageFlags =
		    VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array layoutBindings{ layoutBindingKernelImage,
			                       layoutBindingImage };

		vk::DescriptorSetLayoutCreateInfo setLayoutInfo;
		setLayoutInfo.bindingCount = uint32_t(layoutBindings.size());
		setLayoutInfo.pBindings = layoutBindings.data();

		m_colorComputeSetLayout = vk.create(setLayoutInfo);
		if (!m_colorComputeSetLayout)
		{
			std::cerr << "error: failed to create color compute set layout"
			          << std::endl;
			abort();
		}
	}
#pragma endregion

#pragma region ray compute descriptor set
	{
		vk::DescriptorSetAllocateInfo setAlloc;
		setAlloc.descriptorPool = m_computePool.get();
		setAlloc.descriptorSetCount = 1;
		setAlloc.pSetLayouts = &m_rayComputeSetLayout.get();

		vk.allocate(setAlloc, &m_rayComputeSet.get());

		if (!m_rayComputeSet)
		{
			std::cerr << "error: failed to allocate ray compute descriptor set"
			          << std::endl;
			abort();
		}
	}
#pragma endregion

#pragma region color compute descriptor set
	vk::DescriptorSetAllocateInfo setAlloc;
	setAlloc.descriptorPool = m_computePool.get();
	setAlloc.descriptorSetCount = 1;
	setAlloc.pSetLayouts = &m_colorComputeSetLayout.get();

	vk.allocate(setAlloc, &m_colorComputeSet.get());

	if (!m_colorComputeSet)
	{
		std::cerr << "error: failed to allocate color compute descriptor set"
		          << std::endl;
		abort();
	}
#pragma endregion

#pragma region ray compute pipeline layout
	{
		// VkPushConstantRange pcRange{};
		// pcRange.size = sizeof(float);
		// pcRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		vk::PipelineLayoutCreateInfo computePipelineLayoutInfo;
		computePipelineLayoutInfo.setLayoutCount = 1;
		computePipelineLayoutInfo.pSetLayouts = &m_rayComputeSetLayout.get();
		// computePipelineLayoutInfo.pushConstantRangeCount = 1;
		// computePipelineLayoutInfo.pPushConstantRanges = &pcRange;
		computePipelineLayoutInfo.pushConstantRangeCount = 0;
		computePipelineLayoutInfo.pPushConstantRanges = nullptr;

		m_rayComputePipelineLayout = vk.create(computePipelineLayoutInfo);
		if (!m_rayComputePipelineLayout)
		{
			std::cerr << "error: failed to create ray compute pipeline layout"
			          << std::endl;
			abort();
		}
	}
#pragma endregion

#pragma region color compute pipeline layout
	{
		// VkPushConstantRange pcRange{};
		// pcRange.size = sizeof(float);
		// pcRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		std::array<VkDescriptorSetLayout, 2> layouts = {
			m_rayComputeSetLayout.get(), m_colorComputeSetLayout.get()
		};

		vk::PipelineLayoutCreateInfo computePipelineLayoutInfo;
		computePipelineLayoutInfo.setLayoutCount = uint32_t(layouts.size());
		computePipelineLayoutInfo.pSetLayouts = layouts.data();
		// computePipelineLayoutInfo.pushConstantRangeCount = 1;
		// computePipelineLayoutInfo.pPushConstantRanges = &pcRange;
		computePipelineLayoutInfo.pushConstantRangeCount = 0;
		computePipelineLayoutInfo.pPushConstantRanges = nullptr;

		m_colorComputePipelineLayout = vk.create(computePipelineLayoutInfo);
		if (!m_colorComputePipelineLayout)
		{
			std::cerr
			    << "error: failed to create color compute pipeline layout"
			    << std::endl;
			abort();
		}
	}
#pragma endregion

#pragma region pipeline
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
	binding.stride = sizeof(float) * 2;

	VkVertexInputAttributeDescription attribute = {};
	attribute.binding = 0;
	attribute.format = VK_FORMAT_R32G32_SFLOAT;
	attribute.location = 0;
	attribute.offset = 0;

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &binding;
	vertexInputInfo.vertexAttributeDescriptionCount = 1;
	vertexInputInfo.pVertexAttributeDescriptions = &attribute;

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	// inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	inputAssembly.primitiveRestartEnable = false;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = widthf;
	viewport.height = heightf;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent.width = width;
	scissor.extent.height = height;

	vk::PipelineViewportStateCreateInfo viewportState;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	vk::PipelineRasterizationStateCreateInfo rasterizer;
	rasterizer.depthClampEnable = false;
	rasterizer.rasterizerDiscardEnable = false;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
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
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = m_colorComputePipelineLayout;
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

#pragma region ray compute pipeline
	{
		vk::ComputePipelineCreateInfo computePipelineInfo;
		computePipelineInfo.layout = m_rayComputePipelineLayout.get();
		computePipelineInfo.stage = vk::PipelineShaderStageCreateInfo();
		computePipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		computePipelineInfo.stage.module = m_rayComputeModule.get();
		computePipelineInfo.stage.pName = "main";
		computePipelineInfo.basePipelineHandle = nullptr;
		computePipelineInfo.basePipelineIndex = -1;

		m_rayComputePipeline = vk.create(computePipelineInfo);
		if (!m_rayComputePipeline)
		{
			std::cerr << "error: failed to create ray compute pipeline"
			          << std::endl;
			abort();
		}
	}
#pragma endregion

#pragma region color compute pipeline
	{
		vk::ComputePipelineCreateInfo computePipelineInfo;
		computePipelineInfo.layout = m_colorComputePipelineLayout.get();
		computePipelineInfo.stage = vk::PipelineShaderStageCreateInfo();
		computePipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		computePipelineInfo.stage.module = m_colorComputeModule.get();
		computePipelineInfo.stage.pName = "main";
		computePipelineInfo.basePipelineHandle = nullptr;
		computePipelineInfo.basePipelineIndex = -1;

		m_colorComputePipeline = vk.create(computePipelineInfo);
		if (!m_colorComputePipeline)
		{
			std::cerr << "error: failed to create color compute pipeline"
			          << std::endl;
			abort();
		}
	}
#pragma endregion

#pragma region vertexBuffer
	using Vertex = std::array<float, 2>;
	// std::vector<Vertex> vertices(POINT_COUNT);

	// for (auto& vertex : vertices)
	//{
	//	vertex[0] = dis(gen);
	//	vertex[1] = dis(gen);
	//}

	std::vector<Vertex> vertices{ { -1, -1 }, { 3, -1 }, { -1, 3 } };

	m_vertexBuffer = Buffer(
	    rw, sizeof(Vertex) * vertices.size(),
	    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	    VMA_MEMORY_USAGE_CPU_TO_GPU, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	Vertex* data = m_vertexBuffer.map<Vertex>();
	std::copy(vertices.begin(), vertices.end(), data);
	m_vertexBuffer.unmap();
#pragma endregion

#pragma region ray compute UBO
	{
		m_raysBuffer = Buffer(rw, sizeof(RayIteration) * RAYS_COUNT,
		                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		                      VMA_MEMORY_USAGE_CPU_TO_GPU,
		                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

		std::vector<RayIteration> raysVector(RAYS_COUNT);
		for (size_t i = 0; i < RAYS_COUNT; i++)
		{
			raysVector[i].position = { 10.0f, heightf / 2.0f - 4 + i };
			raysVector[i].direction = { 1.0f, 0.0f };
		}

		RayIteration* rayPtr = m_raysBuffer.map<RayIteration>();
		//*rayPtr = raysVector[0];
		std::memcpy(rayPtr, raysVector.data(),
		            sizeof(*raysVector.data()) * RAYS_COUNT);
		m_raysBuffer.unmap();

		VkDescriptorBufferInfo setBufferInfo{};
		setBufferInfo.buffer = m_raysBuffer.get();
		setBufferInfo.range = sizeof(RayIteration) * RAYS_COUNT;
		setBufferInfo.offset = 0;

		vk::WriteDescriptorSet uboWrite;
		uboWrite.descriptorCount = 1;
		uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		uboWrite.dstArrayElement = 0;
		uboWrite.dstBinding = 0;
		uboWrite.dstSet = m_rayComputeSet.get();
		uboWrite.pBufferInfo = &setBufferInfo;

		vk.updateDescriptorSets(uboWrite);
	}
#pragma endregion

#pragma region color compute UBO
	m_computeUbo = Buffer(
	    rw, sizeof(m_config), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	    VMA_MEMORY_USAGE_CPU_TO_GPU, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	m_config.copyTo(m_computeUbo.map());
	m_computeUbo.unmap();

	VkDescriptorBufferInfo setBufferInfo{};
	setBufferInfo.buffer = m_computeUbo.get();
	setBufferInfo.range = sizeof(m_config);
	setBufferInfo.offset = 0;

	vk::WriteDescriptorSet uboWrite;
	uboWrite.descriptorCount = 1;
	uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboWrite.dstArrayElement = 0;
	uboWrite.dstBinding = 1;
	uboWrite.dstSet = m_rayComputeSet.get();
	uboWrite.pBufferInfo = &setBufferInfo;

	vk.updateDescriptorSets(uboWrite);
#pragma endregion

#pragma region outputImage
	m_outputImage = Texture2D(
	    rw, width, height, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
	    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vk.debugMarkerSetObjectName(m_outputImage.get(), "outputImage");

	m_outputImage.transitionLayoutImmediate(VK_IMAGE_LAYOUT_UNDEFINED,
	                                        VK_IMAGE_LAYOUT_GENERAL);
#pragma endregion

#pragma region outputImageHDR
	m_outputImageHDR = Texture2D(
	    rw, width, height, VK_FORMAT_R32G32B32A32_SFLOAT,
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
	        VK_IMAGE_USAGE_SAMPLED_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, -1);

	vk.debugMarkerSetObjectName(m_outputImageHDR.get(), "outputImageHDR");

	m_outputImageHDR.transitionLayoutImmediate(VK_IMAGE_LAYOUT_UNDEFINED,
	                                           VK_IMAGE_LAYOUT_GENERAL);

	VkDescriptorImageInfo setImageInfo{};
	setImageInfo.imageView = m_outputImageHDR.view();
	setImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkDescriptorImageInfo setImageInfo2{};
	setImageInfo2.imageView = m_outputImageHDR.view();
	setImageInfo2.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	setImageInfo2.sampler = m_outputImageHDR.sampler();

	vk::WriteDescriptorSet write;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	write.dstArrayElement = 0;
	write.dstBinding = 0;
	write.dstSet = m_colorComputeSet.get();
	write.pImageInfo = &setImageInfo;

	vk::WriteDescriptorSet write2;
	write2.descriptorCount = 1;
	write2.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write2.dstArrayElement = 0;
	write2.dstBinding = 1;
	write2.dstSet = m_colorComputeSet.get();
	write2.pImageInfo = &setImageInfo2;

	std::array writes{ write, write2 };

	vk.updateDescriptorSets(2, writes.data());
#pragma endregion

#pragma region framebuffer
	vk::FramebufferCreateInfo framebufferInfo;
	framebufferInfo.renderPass = m_renderPass.get();
	std::array attachments = {
		m_outputImage.view()  //, m_outputImageHDR.view()
	};
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

	vk.setLogInactive();
}

LightTransport::~LightTransport() {}

void LightTransport::render(CommandBuffer& cb)
{
	std::array clearValues = { VkClearValue{}, VkClearValue{} };

	vk::RenderPassBeginInfo rpInfo;
	rpInfo.framebuffer = m_framebuffer;
	rpInfo.renderPass = m_renderPass;
	rpInfo.renderArea.extent.width = width;
	rpInfo.renderArea.extent.height = height;
	rpInfo.clearValueCount = 1;
	rpInfo.pClearValues = clearValues.data();

	vk::SubpassBeginInfo subpassBeginInfo;
	subpassBeginInfo.contents = VK_SUBPASS_CONTENTS_INLINE;

	vk::SubpassEndInfo subpassEndInfo;

	cb.beginRenderPass2(rpInfo, subpassBeginInfo);

	cb.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
	cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS,
	                     m_colorComputePipelineLayout, 1, m_colorComputeSet);
	cb.bindVertexBuffer(m_vertexBuffer);
	// cb.draw(POINT_COUNT);
	cb.draw(3);

	cb.endRenderPass2(subpassEndInfo);
}

void LightTransport::compute(CommandBuffer& cb)
{
	cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE,
	                     m_rayComputePipelineLayout, 0, m_rayComputeSet);
	cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE,
	                     m_colorComputePipelineLayout, 1, m_colorComputeSet);
	cb.bindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, m_rayComputePipeline);
	cb.dispatch(RAYS_COUNT);

	vk::BufferMemoryBarrier barrier;
	barrier.buffer = m_raysBuffer;
	barrier.size = sizeof(RayIteration) * RAYS_COUNT;
	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	cb.pipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
	                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, barrier);

	cb.bindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, m_colorComputePipeline);
	cb.dispatch(width / 8, height / 8);
}

void LightTransport::imgui(CommandBuffer& cb)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Controls");

		bool changed = false;

		// changed |= ImGui::SliderFloat("CamFocalDistance",
		//                              &m_config.camFocalDistance,
		//                              0.1f, 30.0f);
		// changed |= ImGui::SliderFloat("CamFocalLength",
		//                              &m_config.camFocalLength, 0.0f, 20.0f);
		// changed |= ImGui::SliderFloat("CamAperture", &m_config.camAperture,
		//                              0.0f, 5.0f);
		//
		// changed |= ImGui::DragFloat3("rotation", &m_config.camRot.x, 0.01f);
		// changed |= ImGui::DragFloat3("lightDir", &m_config.lightDir.x,
		// 0.01f);
		//
		// changed |= ImGui::SliderFloat("scene radius", &m_config.sceneRadius,
		//                              0.0f, 10.0f);
		//
		// ImGui::SliderFloat("BloomAscale1", &m_config.bloomAscale1,
		// 0.0f, 1.0f); ImGui::SliderFloat("BloomAscale2",
		// &m_config.bloomAscale2, 0.0f, 1.0f);
		// ImGui::SliderFloat("BloomBscale1", &m_config.bloomBscale1,
		// 0.0f, 1.0f); ImGui::SliderFloat("BloomBscale2",
		// &m_config.bloomBscale2, 0.0f, 1.0f);
		//
		// changed |= ImGui::SliderFloat("Power", &m_config.power,
		// 0.0f, 50.0f); changed |= ImGui::DragInt("Iterations",
		// &m_config.iterations, 0.1f, 1, 16);

		if (changed)
			applyImguiParameters();

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
		std::array clearValues = { VkClearValue{}, VkClearValue{} };

		vk::RenderPassBeginInfo rpInfo;
		rpInfo.framebuffer = m_framebuffer.get();
		rpInfo.renderPass = rw.get().imguiRenderPass();
		rpInfo.renderArea.extent.width = width;
		rpInfo.renderArea.extent.height = height;
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

void LightTransport::standaloneDraw()
{
	auto& vk = rw.get().device();

	auto& frame = rw.get().getAvailableCommandBuffer();
	frame.reset();

	auto& cb = frame.commandBuffer;

	cb.reset();
	cb.begin();
	cb.debugMarkerBegin("compute", 1.0f, 0.2f, 0.2f);
	compute(cb);
	cb.debugMarkerEnd();
	cb.end();
	if (vk.queueSubmit(vk.graphicsQueue(), cb.get()) != VK_SUCCESS)
	{
		std::cerr << "error: failed to submit imgui command buffer"
		          << std::endl;
		abort();
	}
	vk.wait(vk.graphicsQueue());

	outputTextureHDR().generateMipmapsImmediate(VK_IMAGE_LAYOUT_GENERAL);

	cb.reset();
	cb.begin();

	cb.debugMarkerBegin("render", 0.2f, 0.2f, 1.0f);

	render(cb);

	// vk::ImageMemoryBarrier barrier;
	// barrier.image = outputTexture();
	// barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	// barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	// barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	// barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	// barrier.subresourceRange.baseMipLevel = 0;
	// barrier.subresourceRange.levelCount = 1;
	// barrier.subresourceRange.baseArrayLayer = 0;
	// barrier.subresourceRange.layerCount = 1;
	// barrier.srcAccessMask = 0;
	// barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	// cb.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	// VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, barrier);

	// barrier.image = outputTexture();
	// barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	// barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	// barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	// barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	// barrier.subresourceRange.baseMipLevel = 0;
	// barrier.subresourceRange.levelCount = 1;
	// barrier.subresourceRange.baseArrayLayer = 0;
	// barrier.subresourceRange.layerCount = 1;
	// barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	// barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	// cb.pipelineBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	// VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, barrier);

	// imgui(cb);

	cb.debugMarkerEnd();
	cb.end();
	if (vk.queueSubmit(vk.graphicsQueue(), cb.get()) != VK_SUCCESS)
	{
		std::cerr << "error: failed to submit imgui command buffer"
		          << std::endl;
		abort();
	}
	vk.wait(vk.graphicsQueue());

	frame.reset();

	rw.get().present(m_outputImage, VK_IMAGE_LAYOUT_GENERAL,
	                 VK_IMAGE_LAYOUT_GENERAL, nullptr);

	setSampleAndRandomize(0.0f);
}

void LightTransport::applyImguiParameters() { auto& vk = rw.get().device(); }

void LightTransport::randomizePoints()
{
	auto& vk = rw.get().device();

	using Vertex = std::array<float, 2>;
	std::vector<Vertex> vertices(POINT_COUNT);

	for (auto& vertex : vertices)
	{
		vertex[0] = dis(gen);
		vertex[1] = dis(gen);
	}

	Vertex* data = m_vertexBuffer.map<Vertex>();
	std::copy(vertices.begin(), vertices.end(), static_cast<Vertex*>(data));
	m_vertexBuffer.unmap();

	m_config.seed = udis(gen);
	m_config.copyTo(m_computeUbo.map());
	m_computeUbo.unmap();
}

void LightTransport::setSampleAndRandomize(float s)
{
	m_config.seed = udis(gen);
	m_config.copyTo(m_computeUbo.map());
	m_computeUbo.unmap();
}
}  // namespace cdm
