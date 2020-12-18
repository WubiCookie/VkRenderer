#include "MandelBulb.hpp"

#include "TextureFactory.hpp"

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

#define mandelbulbTexScale 1

constexpr uint32_t width = 1280 * mandelbulbTexScale;
constexpr uint32_t height = 720 * mandelbulbTexScale;
constexpr float widthf = 1280.0f * mandelbulbTexScale;
constexpr float heightf = 720.0f * mandelbulbTexScale;

namespace cdm
{
static constexpr float Pi{ 3.14159265359f };

void Mandelbulb::Config::copyTo(void* ptr)
{
	stepSize =
	    std::min(1.0f / (volumePrecision * density), sceneRadius / 2.0f);

	std::memcpy(ptr, this, sizeof(*this));
}

template <typename T>
class MandelbulbShaderLib : public T
{
public:
	const sdw::Float Pi;

	const sdw::Float Height;
	const sdw::Float Width;

	sdw::Ubo ubo;

	sdw::Function<sdw::Float, sdw::InOutFloat> randomFloat;
	sdw::Function<sdw::Mat3, sdw::InVec3> rotationMatrix;
	sdw::Function<sdw::Float, sdw::InVec3> maxV;
	sdw::Function<sdw::Vec3, sdw::InOutFloat> randomDir;
	sdw::Function<sdw::Vec3, sdw::InVec3> backgroundColor;
	sdw::Function<sdw::Float, sdw::InVec3, sdw::OutVec3, sdw::OutVec3>
	    distanceEstimation;
	sdw::Function<sdw::Vec3, sdw::InVec3, sdw::InOutFloat> directLight;
	sdw::Function<sdw::Vec3, sdw::InVec3, sdw::InVec3, sdw::InOutFloat>
	    pathTrace;
	sdw::Function<sdw::Vec2, sdw::InInt, sdw::InFloat, sdw::InOutFloat>
	    sampleAperture;

	MandelbulbShaderLib(Mandelbulb::Config const& config)
	    : T(),
	      Pi(declConstant<sdw::Float>("Pi", sdw::Float{ cdm::Pi })),
	      Width(declConstant<sdw::Float>("Width", sdw::Float{ float(width) })),
	      Height(
	          declConstant<sdw::Float>("Height", sdw::Float{ float(height) })),
	      ubo(sdw::Ubo(*this, "ubo", 1, 0))
	{
		using namespace sdw;

		(void)ubo.declMember<Float>("camAperture");
		(void)ubo.declMember<Float>("camFocalDistance");
		(void)ubo.declMember<Float>("camFocalLength");
		(void)ubo.declMember<Float>("density");
		(void)ubo.declMember<Float>("maxAbso");
		(void)ubo.declMember<Float>("maxShadowAbso");
		(void)ubo.declMember<Float>("power");
		(void)ubo.declMember<Float>("samples");
		(void)ubo.declMember<Float>("sceneRadius");
		(void)ubo.declMember<Float>("seed");
		(void)ubo.declMember<Float>("stepSize");
		(void)ubo.declMember<Float>("stepsSkipShadow");
		(void)ubo.declMember<Float>("volumePrecision");
		(void)ubo.declMember<Int>("maxSteps");
		(void)ubo.declMember<Vec3>("camRot");
		(void)ubo.declMember<Vec3>("lightColor");
		(void)ubo.declMember<Vec3>("lightDir");
		(void)ubo.declMember<Float>("bloomAscale1");
		(void)ubo.declMember<Float>("bloomAscale2");
		(void)ubo.declMember<Float>("bloomBscale1");
		(void)ubo.declMember<Float>("bloomBscale2");
		(void)ubo.declMember<Int>("iterations");
		ubo.end();

		randomFloat = implementFunction<Float>(
		    "randomFloat",
		    [&](Float seed) {
			    auto res = declLocale("res", fract(sin(seed) * 43758.5453_f));
			    seed = seed + 1.0_f;
			    returnStmt(res);
		    },
		    InOutFloat{ *this, "seed" });

		rotationMatrix = implementFunction<Mat3>(
		    "rotationMatrix",
		    [&](const Vec3& rotEuler) {
			    auto c = declLocale("c", cos(rotEuler.x()));
			    auto s = declLocale("s", sin(rotEuler.x()));
			    auto rx = declLocale(
			        "rx", mat3(vec3(1.0_f, 0.0_f, 0.0_f), vec3(0.0_f, c, -s),
			                   vec3(0.0_f, s, c)));
			    rx = transpose(rx);
			    c = cos(rotEuler.y());
			    s = sin(rotEuler.y());
			    auto ry = declLocale(
			        "ry", mat3(vec3(c, 0.0_f, -s), vec3(0.0_f, 1.0_f, 0.0_f),
			                   vec3(s, 0.0_f, c)));
			    ry = transpose(ry);
			    c = cos(rotEuler.z());
			    s = sin(rotEuler.z());
			    auto rz = declLocale(
			        "rz", mat3(vec3(c, -s, 0.0_f), vec3(s, c, 0.0_f),
			                   vec3(0.0_f, 0.0_f, 1.0_f)));
			    rz = transpose(rz);
			    returnStmt(rz * rx * ry);
		    },
		    InVec3{ *this, "rotEuler" });

		maxV = implementFunction<Float>(
		    "maxV",
		    [&](const Vec3& v) {
			    returnStmt(ternary(v.x() > v.y(),
			                       ternary(v.x() > v.z(), v.x(), v.z()),
			                       ternary(v.y() > v.z(), v.y(), v.z())));
		    },
		    InVec3{ *this, "v" });

		randomDir = implementFunction<Vec3>(
		    "randomDir",
		    [&](Float seed) {
			    returnStmt(
			        vec3(1.0_f, 0.0_f, 0.0_f) *
			        rotationMatrix(vec3(randomFloat(seed) * 2.0_f * Pi, 0.0_f,
			                            randomFloat(seed) * 2.0_f * Pi)));
		    },
		    InOutFloat{ *this, "seed" });

		backgroundColor = implementFunction<Vec3>(
		    "backgroundColor", [&](const Vec3&) { returnStmt(vec3(0.0_f)); },
		    InVec3{ *this, "unused" });

		distanceEstimation = implementFunction<Float>(
		    "distanceEstimation",
		    [&](const Vec3& pos_arg, Vec3 volumeColor, Vec3 emissionColor) {
			    // Vec3 pos = declLocale<Vec3>*this, ("pos_local");
			    auto pos = declLocale("pos", pos_arg);
			    auto basePos = declLocale("basePos", vec3(0.0_f));
			    auto scale = declLocale("scale", 1.0_f);

			    // pos = pos_arg;
			    // Vec3 basePos = declLocale<Vec3>*this, ("basePos");
			    //   basePos = vec3(0.0_f);
			    //   Float scale = 1.0_f;

			    //   pos /= scale;
			    //   pos += basePos;

			    volumeColor = vec3(0.0_f);
			    emissionColor = vec3(0.0_f);

			    pos.yz() = vec2(pos.z(), pos.y());

			    auto r = declLocale("r", length(pos));
			    auto z = declLocale("z", pos);
			    auto c = declLocale("c", pos);
			    auto dr = declLocale("dr", 1.0_f);
			    auto theta = declLocale("theta", 0.0_f);
			    auto phi = declLocale("phi", 0.0_f);
			    auto orbitTrap = declLocale("orbitTrap", vec3(1.0_f));

			    // Float r = length(pos);
			    // Vec3 z = pos;
			    // Vec3 c = pos;
			    // Float dr = 1.0_f, theta = 0.0_f, phi = 0.0_f;
			    // Vec3 orbitTrap = vec3(1.0_f);

			    auto SceneRadius = declLocale(
			        "SceneRadius", ubo.getMember<Float>("sceneRadius"));
			    auto Power =
			        declLocale("Power", ubo.getMember<Float>("power"));

				auto Iterations =
			        declLocale("Iteration", ubo.getMember<Int>("iterations"));

			    //FOR(*this, Int, i, 0_i, i < 8_i, ++i)
			    //{
				   // r = length(z);
				   // IF(*this, r > SceneRadius) { loopBreakStmt(); }
				   // FI;
				   // orbitTrap = min(abs(z) * 1.2_f, orbitTrap);
				   // theta = acos(z.y() / r);
				   // // phi = atan(z.z(), z.x());
				   // phi = atan2(z.z(), z.x());
				   // dr = pow(r, Power - 1.0_f) * Power * dr + 1.0_f;
				   // theta *= Power;
				   // phi *= Power;
				   // z = pow(r, Power) * vec3(sin(theta) * cos(phi), cos(theta),
				   //                          sin(phi) * sin(theta)) +
				   //     c;
			    //}
			    //ROF;

				//[{r = Sqrt[x^2 + y^2 + z^2], theta = n ArcTan[x, y], phi}, phi = n ArcSin[z/r]; r^n{Cos[theta]Cos[phi], Sin[theta]Cos[phi], Sin[phi]}];

				//TriplexPow[{x_, y_, z_}, n_] :=
				//Module[{
				//         r = Sqrt[x^2 + y^2 + z^2],
				//         theta = n ArcTan[x, y],
				//         phi},
				// phi = n ArcSin[z/r];
				// r^n { Cos[theta] Cos[phi] , Sin[theta] Cos[phi], Sin[phi] } ];

			    FOR(*this, Int, i, 0_i, i < Iterations, ++i)
			    {
				    r = length(z);
				    IF(*this, r > SceneRadius) { loopBreakStmt(); }
				    FI;
				    orbitTrap = min(abs(z) * 1.2_f, orbitTrap);

				    theta = atan2(z.x(), z.z());
				    phi = asin(z.z() / r);

				    dr = pow(r, Power - 1.0_f) * Power * dr + 1.0_f;
				    theta *= Power;
				    phi *= Power;

				    z = pow(r, Power) * vec3(cos(theta) * cos(phi),
				                             sin(theta) * cos(phi),
				                             sin(phi)) +
				        c;
			    }
			    ROF;

			    auto dist =
			        declLocale("dist", 0.5_f * log(r) * r / dr * scale);

			    volumeColor = (1.0_f - orbitTrap) * 0.98_f;

			    emissionColor =
			        vec3(ternary(orbitTrap.z() < 0.0001_f, 20.0_f, 0.0_f));

			    returnStmt(dist);
		    },
		    InVec3{ *this, "pos_arg" }, OutVec3{ *this, "volumeColor" },
		    OutVec3{ *this, "emissionColor" });

		directLight = implementFunction<Vec3>(
		    "directLight",
		    [&](const Vec3& pos_arg, Float seed) {
			    auto pos = declLocale("pos", pos_arg);

			    auto absorption = declLocale("absorption", vec3(1.0_f));
			    auto volumeColor = declLocale("volumeColor", vec3(0.0_f));
			    auto emissionColor = declLocale("emissionColor", vec3(0.0_f));

			    auto dist = declLocale<Float>("dist");

			    auto MaxSteps =
			        declLocale("MaxSteps", ubo.getMember<Int>("maxSteps"));
			    auto StepSize =
			        declLocale("StepSize", ubo.getMember<Float>("stepSize"));
			    auto SceneRadius = declLocale(
			        "SceneRadius", ubo.getMember<Float>("sceneRadius"));
			    auto LightDir =
			        declLocale("LightDir", ubo.getMember<Vec3>("lightDir"));
			    auto LightColor = declLocale(
			        "LightColor", ubo.getMember<Vec3>("lightColor"));
			    auto MaxShadowAbso = declLocale(
			        "MaxShadowAbso", ubo.getMember<Float>("maxShadowAbso"));
			    auto Density =
			        declLocale("Density", ubo.getMember<Float>("density"));

			    LightDir = normalize(LightDir);

			    FOR(*this, Int, i, 0_i, i < MaxSteps, ++i)
			    {
				    // auto dist = declLocale(
				    //"dist",
				    dist = distanceEstimation(pos, volumeColor, emissionColor);
				    pos -= LightDir * max(dist, StepSize);

				    IF(*this, dist < StepSize)
				    {
					    auto abStep =
					        declLocale("abStep", StepSize * randomFloat(seed));
					    pos -= LightDir * (abStep - StepSize);
					    IF(*this, dist < 0.0_f)
					    {
						    auto absorbance = declLocale(
						        "absorbance", exp(-Density * abStep));
						    absorption *= absorbance;
						    IF(*this, maxV(absorption) < 1.0_f - MaxShadowAbso)
						    {
							    loopBreakStmt();
						    }
						    FI;
					    }
					    FI;
				    }
				    FI;

				    IF(*this, length(pos) > SceneRadius) { loopBreakStmt(); }
				    FI;
			    }
			    ROF;

			    returnStmt(
			        LightColor *
			        max((absorption + MaxShadowAbso - 1.0_f) / MaxShadowAbso,
			            vec3(0.0_f)));
		    },
		    InVec3{ *this, "pos_arg" }, InOutFloat{ *this, "seed" });

		pathTrace = implementFunction<Vec3>(
		    "pathTrace",
		    [&](const Vec3& rayPos_arg, const Vec3& rayDir_arg, Float seed) {
			    auto rayPos = declLocale("rayPos", rayPos_arg);
			    auto rayDir = declLocale("rayDir", rayDir_arg);

			    auto MaxSteps =
			        declLocale("MaxSteps", ubo.getMember<Int>("maxSteps"));
			    auto StepsSkipShadow = declLocale(
			        "StepsSkipShadow", ubo.getMember<Int>("stepsSkipShadow"));
			    auto StepSize =
			        declLocale("StepSize", ubo.getMember<Float>("stepSize"));
			    auto SceneRadius = declLocale(
			        "SceneRadius", ubo.getMember<Float>("sceneRadius"));
			    auto Density =
			        declLocale("Density", ubo.getMember<Float>("density"));
			    auto MaxAbso =
			        declLocale("MaxAbso", ubo.getMember<Float>("maxAbso"));

			    rayPos += rayDir * max(length(rayPos) - SceneRadius, 0.0_f);

			    auto outColor = declLocale("outColor", vec3(0.0_f));
			    auto absorption = declLocale("absorption", vec3(1.0_f));

			    auto volumeColor = declLocale("volumeColor", vec3(0.0_f));
			    auto emissionColor = declLocale("emissionColor", vec3(0.0_f));

			    auto dist = declLocale<Float>("dist");

			    FOR(*this, Int, i, 0_i, i < MaxSteps, ++i)
			    {
				    // auto dist = declLocale( "dist",
				    dist =
				        distanceEstimation(rayPos, volumeColor, emissionColor);
				    rayPos += rayDir * max(dist, StepSize);
				    IF(*this, dist < StepSize && length(rayPos) < SceneRadius)
				    {
					    auto abStep =
					        declLocale("abStep", StepSize * randomFloat(seed));
					    rayPos += rayDir * (abStep - StepSize);
					    IF(*this, dist < 0.0_f)
					    {
						    auto absorbance = declLocale(
						        "absorbance", exp(-Density * abStep));
						    auto transmittance = declLocale(
						        "transmittance", 1.0_f - absorbance);

						    // surface glow for a nice additional effect
						    // if(dist > -.0001) outColor += absorption *
						    // vec3(.2, .2, .2);

						    // if(randomFloat() < ShadowRaysPerStep)
						    // emissionColor
						    // += 1.0/ShadowRaysPerStep * volumeColor *
						    // directLight(rayPos);

						    auto i_f = declLocale("i_f", cast<Float>(i));

						    IF(*this,
						       mod(i_f, Float(StepsSkipShadow)) == 0.0_f)
						    {
							    emissionColor += Float(StepsSkipShadow) *
							                     volumeColor *
							                     directLight(rayPos, seed);
						    }
						    FI;

						    outColor +=
						        absorption * transmittance * emissionColor;

						    IF(*this, maxV(absorption) < 1.0_f - MaxAbso)
						    {
							    loopBreakStmt();
						    }
						    FI;

						    IF(*this, randomFloat(seed) > absorbance)
						    {
							    rayDir = randomDir(seed);
							    absorption *= volumeColor;
						    }
						    FI;
					    }
					    FI;
				    }
				    FI;

				    IF(*this, length(rayPos) > SceneRadius &&
				                  dot(rayDir, rayPos) > 0.0_f)
				    {
					    returnStmt(outColor +
					               backgroundColor(rayDir) * absorption);
				    }
				    FI;
			    }
			    ROF;

			    returnStmt(outColor);
		    },
		    InVec3{ *this, "rayPos_arg" }, InVec3{ *this, "rayDir_arg" },
		    InOutFloat{ *this, "seed" });

		// n-blade aperture
		sampleAperture = implementFunction<Vec2>(
		    "sampleAperture",
		    [&](const Int& nbBlades, const Float& rotation, Float seed) {
			    auto alpha =
			        declLocale("alpha", 2.0_f * Pi / cast<Float>(nbBlades));
			    auto side = declLocale("side", sin(alpha / 2.0_f));

			    auto blade = declLocale(
			        "blade",
			        cast<Int>(randomFloat(seed) * cast<Float>(nbBlades)));

			    auto tri = declLocale(
			        "tri", vec2(randomFloat(seed), -randomFloat(seed)));
			    IF(*this, tri.x() + tri.y() > 0.0_f)
			    {
				    tri = vec2(tri.x() - 1.0_f, -1.0_f - tri.y());
			    }
			    FI;
			    tri.x() *= side;
			    tri.y() *= sqrt(1.0_f - side * side);

			    auto angle =
			        declLocale("angle", rotation + cast<Float>(blade) /
			                                           cast<Float>(nbBlades) *
			                                           2.0_f * Pi);

			    returnStmt(vec2(tri.x() * cos(angle) + tri.y() * sin(angle),
			                    tri.y() * cos(angle) - tri.x() * sin(angle)));
		    },
		    InInt{ *this, "nbBlades" }, InFloat{ *this, "rotation" },
		    InOutFloat{ *this, "seed" });
	}
};

Mandelbulb::Mandelbulb(RenderWindow& renderWindow)
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
		    writer.declImage<RWFImg2DRgba32>("kernelStorageImage", 0, 0);

		auto kernelImage =
		    writer.declSampledImage<FImg2DRgba32>("kernelImage", 2, 0);

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
					    bloomV3 += kernelImage.lod(

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

			auto col = writer.declLocale("col", kernelImage.sample(uv).rgb());

			auto bloomAscale1 = writer.declLocale(
			    "BloomAscale1", writer.ubo.getMember<Float>("bloomAscale1"));
			auto bloomAscale2 = writer.declLocale(
			    "BloomAscale2", writer.ubo.getMember<Float>("bloomAscale2"));
			auto bloomBscale1 = writer.declLocale(
			    "BloomBscale1", writer.ubo.getMember<Float>("bloomBscale1"));
			auto bloomBscale2 = writer.declLocale(
			    "BloomBscale2", writer.ubo.getMember<Float>("bloomBscale2"));

			auto bloomA = writer.declLocale(
			    "bloomA",
			    bloom(bloomAscale1 * writer.Height, 0.0_f, in.fragCoord.xy()));
			auto bloomB = writer.declLocale(
			    "bloomB",
			    bloom(bloomBscale1 * writer.Height, 0.0_f, in.fragCoord.xy()));

			// auto bloomA = writer.declLocale(
			//    "bloomA", bloom(0.15_f, 0.0_f, in.fragCoord.xy()));
			// auto bloomB = writer.declLocale(
			//    "bloomB", bloom(0.05_f, 0.0_f, in.fragCoord.xy()));

			auto bloomSum =
			    writer.declLocale("bloomSum", bloomA * vec3(bloomAscale2) +
			                                      bloomB * vec3(bloomBscale2));

			// fragColor = vec4(bloomA, 1.0_f);
			// fragColor = vec4(ACESFilm(col), 1.0_f);
			fragColor = vec4(ACESFilm(col + bloomSum), 1.0_f);
			// fragColor = vec4((col + bloomSum), 1.0_f);
			// fragColor = vec4(col, 1.0_f);
			// fragColor = vec4(in.fragCoord.xy() / vec2(writer.Width,
			// writer.Height), 0.0_f, 1.0_f);
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

#pragma region computeShader
	{
		using namespace sdw;

		MandelbulbShaderLib<ComputeWriter> writer{ m_config };

		auto in = writer.getIn();

		writer.inputLayout(8, 8);

		auto kernelSampledImage =
		    writer.declSampledImage<FImg2DRgba32>("kernelSampledImage", 2, 0);

		auto kernelImage =
		    writer.declImage<RWFImg2DRgba32>("kernelImage", 0, 0);

		writer.implementMain([&]() {
			Float x = writer.declLocale(
			    "x", writer.cast<Float>(in.globalInvocationID.x()));
			Float y = writer.declLocale(
			    "y", writer.cast<Float>(in.globalInvocationID.y()));

			Vec2 xy = writer.declLocale("xy", vec2(x, y));

			IVec2 iuv = writer.declLocale(
			    "iuv", ivec2(writer.cast<Int>(in.globalInvocationID.x()),
			                 writer.cast<Int>(in.globalInvocationID.y())));

			//*
			Vec4 previousColor = writer.declLocale("previousColor", kernelImage.load(iuv));

			Float samples = writer.declLocale(
			    "samples", writer.ubo.getMember<Float>("samples"));
			//"samples", previousColor.a());

			IF(writer, samples == -1.0_f)
			{
				kernelImage.store(iuv, vec4(0.0_f));
			}
			ELSE
			{
				Float seed = writer.declLocale(
				    "seed",
				    cos(x) + sin(y) + writer.ubo.getMember<Float>("seed"));

				auto CamFocalDistance = writer.declLocale(
				    "CamFocalDistance",
				    writer.ubo.getMember<Float>("camFocalDistance"));
				auto CamFocalLength = writer.declLocale(
				    "CamFocalLength",
				    writer.ubo.getMember<Float>("camFocalLength"));
				auto CamAperture = writer.declLocale(
				    "CamAperture", writer.ubo.getMember<Float>("camAperture"));
				auto CamRot = writer.declLocale(
				    "CamRot", writer.ubo.getMember<Vec3>("camRot"));

				Vec2 fragCoord = writer.declLocale(
				    "fragCoord", vec2(x, -y + writer.Height - 1.0_f));

				Vec2 uv = writer.declLocale(
				    "uv",
				    (fragCoord +
				     vec2(writer.randomFloat(seed), writer.randomFloat(seed)) -
				     vec2(writer.Width, writer.Height) / vec2(2.0_f)) /
				        vec2(writer.Height, writer.Height));

				Vec3 focalPoint = writer.declLocale(
				    "focalPoint", vec3(uv * CamFocalDistance / CamFocalLength,
				                       CamFocalDistance));
				Vec3 aperture = writer.declLocale(
				    "aperture",
				    CamAperture *
				        vec3(writer.sampleAperture(6_i, 0.0_f, seed), 0.0_f));
				Vec3 rayDir = writer.declLocale(
				    "rayDir", normalize(focalPoint - aperture));

				Vec3 CamPos =
				    writer.declLocale("CamPos", vec3(0.0_f, 0.0_f, -15.0_f));

				Mat3 CamMatrix = writer.declLocale(
				    "CamMatrix", writer.rotationMatrix(CamRot));
				CamPos = CamMatrix * CamPos;
				rayDir = CamMatrix * rayDir;
				aperture = CamMatrix * aperture;

				// clang-format off
				Vec3 newColor = writer.declLocale("newColor", writer.pathTrace(CamPos + aperture, rayDir, seed));
				Vec3 mixer = writer.declLocale("mixer", vec3(1.0_f / (samples + 1.0_f)));
				Vec3 mixedColor = writer.declLocale("mixedColor", mix(previousColor.rgb(), newColor, mixer));
				Vec4 mixedColor4 = writer.declLocale("mixedColor4", vec4(mixedColor, samples + 1.0_f));
				// clang-format on

				kernelImage.store(iuv, mixedColor4);
				// imageStore(kernelImage, iuv, vec4(uv, 0.0_f, 1.0_f));
				// imageStore(kernelImage, iuv, vec4(0.0_f, fragCoord.y(),
				// 0.0_f, 1.0_f)); imageStore(kernelImage, iuv,
				// vec4(fragCoord.x(), 0.0_f, 0.0_f, 0.0_f));
				// imageStore(kernelImage, iuv, vec4(x, 0.0_f, 0.0_f, 0.0_f));
				// imageStore(kernelImage, iuv,
				//           vec4(uv, 0.0_f, 0.0_f));
			}
			FI;
			//*/

			//kernelImage.store(iuv, vec4(1.0_f));
		});

		std::vector<uint32_t> bytecode =
		    spirv::serialiseSpirv(writer.getShader());

		vk::ShaderModuleCreateInfo createInfo;
		createInfo.codeSize = bytecode.size() * sizeof(*bytecode.data());
		createInfo.pCode = bytecode.data();

		m_computeModule = vk.create(createInfo);
		if (!m_computeModule)
		{
			std::cerr << "error: failed to create compute shader module"
			          << std::endl;
			abort();
		}
	}
#pragma endregion

#pragma region compute descriptor pool
	std::array poolSizes{
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
	};

	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.maxSets = 1;
	poolInfo.poolSizeCount = 3;
	poolInfo.pPoolSizes = poolSizes.data();

	m_computePool = vk.create(poolInfo);
	if (!m_computePool)
	{
		std::cerr << "error: failed to create compute descriptor pool"
		          << std::endl;
		abort();
	}
#pragma endregion

#pragma region compute descriptor set layout
	VkDescriptorSetLayoutBinding layoutBindingKernelImage{};
	layoutBindingKernelImage.binding = 0;
	layoutBindingKernelImage.descriptorCount = 1;
	layoutBindingKernelImage.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	layoutBindingKernelImage.stageFlags =
	    VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding layoutBindingUbo{};
	layoutBindingUbo.binding = 1;
	layoutBindingUbo.descriptorCount = 1;
	layoutBindingUbo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBindingUbo.stageFlags =
	    VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding layoutBindingImage{};
	layoutBindingImage.binding = 2;
	layoutBindingImage.descriptorCount = 1;
	layoutBindingImage.descriptorType =
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindingImage.stageFlags =
	    VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array layoutBindings{ layoutBindingKernelImage, layoutBindingUbo,
		                       layoutBindingImage };

	vk::DescriptorSetLayoutCreateInfo setLayoutInfo;
	setLayoutInfo.bindingCount = 3;
	setLayoutInfo.pBindings = layoutBindings.data();

	m_computeSetLayout = vk.create(setLayoutInfo);
	if (!m_computeSetLayout)
	{
		std::cerr << "error: failed to create compute set layout" << std::endl;
		abort();
	}
#pragma endregion

#pragma region compute descriptor set
	vk::DescriptorSetAllocateInfo setAlloc;
	setAlloc.descriptorPool = m_computePool.get();
	setAlloc.descriptorSetCount = 1;
	setAlloc.pSetLayouts = &m_computeSetLayout.get();

	vk.allocate(setAlloc, &m_computeSet.get());

	if (!m_computeSet)
	{
		std::cerr << "error: failed to allocate compute descriptor set"
		          << std::endl;
		abort();
	}
#pragma endregion

#pragma region compute pipeline layout
	// VkPushConstantRange pcRange{};
	// pcRange.size = sizeof(float);
	// pcRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	vk::PipelineLayoutCreateInfo computePipelineLayoutInfo;
	computePipelineLayoutInfo.setLayoutCount = 1;
	computePipelineLayoutInfo.pSetLayouts = &m_computeSetLayout.get();
	// computePipelineLayoutInfo.pushConstantRangeCount = 1;
	// computePipelineLayoutInfo.pPushConstantRanges = &pcRange;
	computePipelineLayoutInfo.pushConstantRangeCount = 0;
	computePipelineLayoutInfo.pPushConstantRanges = nullptr;

	m_computePipelineLayout = vk.create(computePipelineLayoutInfo);
	if (!m_computePipelineLayout)
	{
		std::cerr << "error: failed to create compute pipeline layout"
		          << std::endl;
		abort();
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
	pipelineInfo.layout = m_computePipelineLayout.get();
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

#pragma region compute pipeline
	vk::ComputePipelineCreateInfo computePipelineInfo;
	computePipelineInfo.layout = m_computePipelineLayout.get();
	computePipelineInfo.stage = vk::PipelineShaderStageCreateInfo();
	computePipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computePipelineInfo.stage.module = m_computeModule.get();
	computePipelineInfo.stage.pName = "main";
	computePipelineInfo.basePipelineHandle = nullptr;
	computePipelineInfo.basePipelineIndex = -1;

	m_computePipeline = vk.create(computePipelineInfo);
	if (!m_computePipeline)
	{
		std::cerr << "error: failed to create compute pipeline" << std::endl;
		abort();
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
	    vk, sizeof(Vertex) * vertices.size(),
	    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	    VMA_MEMORY_USAGE_CPU_TO_GPU, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	Vertex* data = m_vertexBuffer.map<Vertex>();
	std::copy(vertices.begin(), vertices.end(), data);
	m_vertexBuffer.unmap();
#pragma endregion

#pragma region compute UBO
	m_computeUbo = Buffer(
	    vk, sizeof(m_config), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	    VMA_MEMORY_USAGE_CPU_TO_GPU, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	m_config.copyTo(m_computeUbo.map());
	m_computeUbo.unmap();

	CamAperture = m_config.camAperture;
	CamFocalDistance = m_config.camFocalDistance;
	CamFocalLength = m_config.camFocalLength;

	VkDescriptorBufferInfo setBufferInfo{};
	setBufferInfo.buffer = m_computeUbo.get();
	setBufferInfo.range = sizeof(m_config);
	setBufferInfo.offset = 0;

	vk::WriteDescriptorSet uboWrite;
	uboWrite.descriptorCount = 1;
	uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboWrite.dstArrayElement = 0;
	uboWrite.dstBinding = 1;
	uboWrite.dstSet = m_computeSet.get();
	uboWrite.pBufferInfo = &setBufferInfo;

	vk.updateDescriptorSets(uboWrite);
#pragma endregion

	TextureFactory f(vk);

#pragma region outputImage
	f.setWidth(width);
	f.setHeight(height);
	f.setFormat(VK_FORMAT_R8G8B8A8_UNORM);
	f.setUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
	           VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	//f.setMipLevels(-1);

	m_outputImage = f.createTexture2D();

	vk.debugMarkerSetObjectName(m_outputImage.image(), "outputImage");

	//m_outputImage = Texture2D(
	//    rw, width, height, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
	//    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	//        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	//    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_outputImage.transitionLayoutImmediate(VK_IMAGE_LAYOUT_UNDEFINED,
	                                        VK_IMAGE_LAYOUT_GENERAL);
#pragma endregion

#pragma region outputImageHDR
	//f.setWidth(width);
	//f.setHeight(height);
	f.setFormat(VK_FORMAT_R32G32B32A32_SFLOAT);
	f.setUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
	           VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
	           VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	f.setMipLevels(11);
	f.setLod(0, 10);
	f.setMipmapMode(VK_SAMPLER_MIPMAP_MODE_LINEAR);

	m_outputImageHDR = f.createTexture2D();

	vk.debugMarkerSetObjectName(m_outputImageHDR.image(), "outputImageHDR");

	//m_outputImageHDR = Texture2D(
	//    rw, width, height, VK_FORMAT_R32G32B32A32_SFLOAT,
	//    VK_IMAGE_TILING_OPTIMAL,
	//    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	//        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
	//        VK_IMAGE_USAGE_SAMPLED_BIT,
	//    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, -1);

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
	write.dstSet = m_computeSet.get();
	write.pImageInfo = &setImageInfo;

	vk::WriteDescriptorSet write2;
	write2.descriptorCount = 1;
	write2.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write2.dstArrayElement = 0;
	write2.dstBinding = 2;
	write2.dstSet = m_computeSet.get();
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
}

Mandelbulb::~Mandelbulb() {}

void Mandelbulb::render(CommandBuffer& cb)
{
	std::array clearValues = { VkClearValue{}, VkClearValue{} };

	vk::RenderPassBeginInfo rpInfo;
	rpInfo.framebuffer = m_framebuffer.get();
	rpInfo.renderPass = m_renderPass.get();
	rpInfo.renderArea.extent.width = width;
	rpInfo.renderArea.extent.height = height;
	rpInfo.clearValueCount = 1;
	rpInfo.pClearValues = clearValues.data();

	vk::SubpassBeginInfo subpassBeginInfo;
	subpassBeginInfo.contents = VK_SUBPASS_CONTENTS_INLINE;

	vk::SubpassEndInfo subpassEndInfo;

	cb.beginRenderPass2(rpInfo, subpassBeginInfo);

	cb.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.get());
	cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS,
	                     m_computePipelineLayout.get(), 0, m_computeSet.get());
	cb.bindVertexBuffer(m_vertexBuffer.get());
	// cb.draw(POINT_COUNT);
	cb.draw(3);

	cb.endRenderPass2(subpassEndInfo);
}

void Mandelbulb::compute(CommandBuffer& cb)
{
	cb.bindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline.get());
	cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE,
	                     m_computePipelineLayout.get(), 0, m_computeSet.get());
	cb.dispatch(width / 8, height / 8);
}

void Mandelbulb::imgui(CommandBuffer& cb)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Controls");

		bool changed = false;

		changed |= ImGui::SliderFloat("CamFocalDistance",
		                              &m_config.camFocalDistance, 0.1f, 30.0f);
		changed |= ImGui::SliderFloat("CamFocalLength",
		                              &m_config.camFocalLength, 0.0f, 20.0f);
		changed |= ImGui::SliderFloat("CamAperture", &m_config.camAperture,
		                              0.0f, 5.0f);

		changed |= ImGui::DragFloat3("rotation", &m_config.camRot.x, 0.01f);
		changed |= ImGui::DragFloat3("lightDir", &m_config.lightDir.x, 0.01f);

		changed |= ImGui::SliderFloat("scene radius", &m_config.sceneRadius,
		                              0.0f, 10.0f);

		ImGui::SliderFloat("BloomAscale1", &m_config.bloomAscale1, 0.0f, 1.0f);
		ImGui::SliderFloat("BloomAscale2", &m_config.bloomAscale2, 0.0f, 1.0f);
		ImGui::SliderFloat("BloomBscale1", &m_config.bloomBscale1, 0.0f, 1.0f);
		ImGui::SliderFloat("BloomBscale2", &m_config.bloomBscale2, 0.0f, 1.0f);

		changed |= ImGui::SliderFloat("Power", &m_config.power, 0.0f, 50.0f);
		changed |= ImGui::DragInt("Iterations", &m_config.iterations, 0.1f, 1, 16);

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

void Mandelbulb::standaloneDraw()
{
	auto& vk = rw.get().device();

	if (mustClear)
	{
		mustClear = false;
		m_config.samples = -1.0f;

		outputTexture().transitionLayoutImmediate(
		    VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		cb.reset();
		cb.begin();

		vk::ImageMemoryBarrier barrier;
		barrier.image = outputImage();
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = outputTexture().mipLevels();
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		cb.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
		                   VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);

		VkClearColorValue clearColor{};
		VkImageSubresourceRange range{};
		range.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		range.layerCount = 1;
		range.levelCount = 1;
		cb.clearColorImage(outputImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		                   &clearColor, range);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		cb.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
		                   VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);

		cb.end();

		if (vk.queueSubmit(vk.graphicsQueue(), imguiCB.get()) != VK_SUCCESS)
		{
			std::cerr << "error: failed to submit clear command buffer"
			          << std::endl;
			abort();
		}

		vk.wait(vk.graphicsQueue());

		outputTexture().transitionLayoutImmediate(
		    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_GENERAL);

		m_config.samples += 1.0f;
	}

	setSampleAndRandomize(m_config.samples);

	computeCB.reset();
	computeCB.begin();
	computeCB.debugMarkerBegin("compute", 1.0f, 0.2f, 0.2f);
	compute(computeCB);
	computeCB.debugMarkerEnd();
	computeCB.end();
	if (vk.queueSubmit(vk.graphicsQueue(), computeCB.get()) != VK_SUCCESS)
	{
		std::cerr << "error: failed to submit imgui command buffer"
		          << std::endl;
		abort();
	}
	vk.wait(vk.graphicsQueue());

	outputTextureHDR().generateMipmapsImmediate(VK_IMAGE_LAYOUT_GENERAL);

	outputTexture().transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	imguiCB.reset();
	imguiCB.begin();
	imguiCB.debugMarkerBegin("render", 0.2f, 0.2f, 1.0f);
	// mandelbulb.compute(imguiCB);
	render(imguiCB);
	imgui(imguiCB);
	imguiCB.debugMarkerEnd();
	imguiCB.end();
	if (vk.queueSubmit(vk.graphicsQueue(), imguiCB.get()) != VK_SUCCESS)
	{
		std::cerr << "error: failed to submit imgui command buffer"
		          << std::endl;
		abort();
	}
	vk.wait(vk.graphicsQueue());

	m_config.samples += 1.0f;

	auto swapImages = rw.get().swapchainImages();

	vk.debugMarkerSetObjectName(copyHDRCB.get(),
	                            VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
	                            "copyHDRCB");

	rw.get().present(m_outputImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, nullptr);

	vk.wait();

	outputTexture().transitionLayoutImmediate(
	    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

	/*
	copyHDRCB.reset();
	copyHDRCB.begin();

	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = outputImage();
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

	    copyHDRCB.blitImage(outputImage(),
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
	barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
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
	//*/
}

void Mandelbulb::applyImguiParameters()
{
	auto& vk = rw.get().device();

	m_config.samples = -1.0f;
	mustClear = true;
}

void Mandelbulb::randomizePoints()
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

void Mandelbulb::setSampleAndRandomize(float s)
{
	m_config.samples = s;
	m_config.seed = udis(gen);
	m_config.copyTo(m_computeUbo.map());
	m_computeUbo.unmap();
}
}  // namespace cdm
