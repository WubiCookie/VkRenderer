#include "MandelBulb.hpp"

#include "CompilerSpirV/compileSpirV.hpp"
#include "ShaderWriter/Intrinsics/Intrinsics.hpp"
#include "ShaderWriter/Source.hpp"

#include <array>
#include <iostream>
#include <stdexcept>

constexpr size_t POINT_COUNT = 2000;

constexpr uint32_t width = 1280;
constexpr uint32_t height = 720;
constexpr float widthf = 1280.0f;
constexpr float heightf = 720.0f;

namespace cdm
{
template <typename T>
class MandelbulbShaderLib : public T
{
public:
	sdw::Float Pi;
	sdw::Float VolumePrecision;
	sdw::Float SceneRadius;
	sdw::Float StepsSkipShadow;
	sdw::Int MaxSteps;
	sdw::Float MaxAbso;
	sdw::Float MaxShadowAbso;

	sdw::Float Width;
	sdw::Float Height;

	sdw::Vec3 CamRot;

	sdw::Float CamFocalLength;
	sdw::Float CamFocalDistance;
	sdw::Float CamAperture;

	sdw::Vec3 LightColor;
	sdw::Vec3 LightDir;

	sdw::Float Power;
	sdw::Float Density;

	sdw::Float StepSize;

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

	MandelbulbShaderLib()
	    : T(),
	      Pi(declConstant<sdw::Float>("Pi", 3.14159265359_f)),
	      VolumePrecision(declConstant<sdw::Float>("VolumePrecision", 0.35_f)),
	      SceneRadius(declConstant<sdw::Float>("SceneRadius", 1.5_f)),
	      StepsSkipShadow(declConstant<sdw::Float>("StepsSkipShadow", 4.0_f)),
	      MaxSteps(declConstant<sdw::Int>("MaxSteps", 500_i)),
	      MaxAbso(declConstant<sdw::Float>("MaxAbso", 0.7_f)),
	      MaxShadowAbso(declConstant<sdw::Float>("MaxShadowAbso", 0.7_f)),
	      Width(declConstant<sdw::Float>("Width", sdw::Float(float(width)))),
	      Height(
	          declConstant<sdw::Float>("Height", sdw::Float(float(height)))),
	      CamRot(
	          declConstant<sdw::Vec3>("CamRot", vec3(0.7_f, -2.4_f, 0.0_f))),
	      CamFocalLength(declConstant<sdw::Float>("CamFocalLength", 5.0_f)),
	      CamFocalDistance(
	          declConstant<sdw::Float>("CamFocalDistance", 14.2_f)),
	      CamAperture(declConstant<sdw::Float>("CamAperture", 0.1_f)),
	      LightColor(declConstant<sdw::Vec3>("LightColor", vec3(3.0_f))),
	      LightDir(declConstant<sdw::Vec3>("LightDir",
	                                       vec3(-1.0_f, -2.0_f, 0.0_f))),
	      Power(declConstant<sdw::Float>("Power", 8.0_f)),
	      Density(declConstant<sdw::Float>("Density", 500.0_f)),
	      StepSize(declConstant<sdw::Float>(
	          "StepSize", sdw::min(1.0_f / (VolumePrecision * Density),
	                               SceneRadius / 2.0_f)))
	{
		using namespace sdw;

		randomFloat = implementFunction<Float>(
		    "randomFloat",
		    [&](Float& seed) {
			    auto res = declLocale("res", fract(sin(seed) * 43758.5453_f));
			    seed = seed + 1.0_f;
			    returnStmt(res);
		    },
		    InOutFloat{ *this, "seed" });

		rotationMatrix = implementFunction<Mat3>(
		    "rotationMatrix",
		    [&](const Vec3& rotEuler) {
			    auto c = declLocale("c", cos(rotEuler.x()));
			    auto s = declLocale("s", (rotEuler.x()));
			    auto rx = declLocale(
			        "rx", mat3(vec3(1.0_f, 0.0_f, 0.0_f), vec3(0.0_f, c, -s),
			                   vec3(0.0_f, s, c)));
			    c = cos(rotEuler.y());
			    s = sin(rotEuler.y());
			    auto ry = declLocale(
			        "ry", mat3(vec3(c, 0.0_f, -s), vec3(0.0_f, 1.0_f, 0.0_f),
			                   vec3(s, 0.0_f, c)));
			    c = cos(rotEuler.z());
			    s = sin(rotEuler.z());
			    auto rz = declLocale(
			        "rz", mat3(vec3(c, -s, 0.0_f), vec3(s, c, 0.0_f),
			                   vec3(0.0_f, 0.0_f, 1.0_f)));
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
		    [&](Float& seed) {
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
		    [&](const Vec3& pos_arg, Vec3& volumeColor, Vec3& emissionColor) {
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

			    FOR(*this, Int, i, 0_i, i < 8_i, ++i)
			    {
				    r = length(z);
				    IF(*this, r > SceneRadius) { loopBreakStmt(); }
				    FI;
				    orbitTrap = min(abs(z) * 1.2_f, orbitTrap);
				    theta = acos(z.y() / r);
				    // phi = atan(z.z(), z.x());
				    phi = atan2(z.z(), z.x());
				    dr = pow(r, Power - 1.0_f) * Power * dr + 1.0_f;
				    theta *= Power;
				    phi *= Power;
				    z = pow(r, Power) * vec3(sin(theta) * cos(phi), cos(theta),
				                             sin(phi) * sin(theta)) +
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
		    [&](const Vec3& pos_arg, Float& seed) {
			    auto pos = declLocale("pos", pos_arg);

			    auto absorption = declLocale("absorption", vec3(1.0_f));
			    auto volumeColor = declLocale("volumeColor", vec3(0.0_f));
			    auto emissionColor = declLocale("emissionColor", vec3(0.0_f));

			    FOR(*this, Int, i, 0_i, i < MaxSteps, ++i)
			    {
				    auto dist = declLocale(
				        "dist",
				        distanceEstimation(pos, volumeColor, emissionColor));
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
		    [&](const Vec3& rayPos_arg, const Vec3& rayDir_arg, Float& seed) {
			    auto rayPos = declLocale("rayPos", rayPos_arg);
			    auto rayDir = declLocale("rayDir", rayDir_arg);

			    rayPos += rayDir * max(length(rayPos) - SceneRadius, 0.0_f);

			    auto outColor = declLocale("outColor", vec3(0.0_f));
			    auto absorption = declLocale("absorption", vec3(1.0_f));

			    auto volumeColor = declLocale("volumeColor", vec3(0.0_f));
			    auto emissionColor = declLocale("emissionColor", vec3(0.0_f));

			    FOR(*this, Int, i, 0_i, i < MaxSteps, ++i)
			    {
				    auto dist = declLocale(
				        "dist", distanceEstimation(rayPos, volumeColor,
				                                   emissionColor));
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
		    [&](const Int& nbBlades, const Float& rotation, Float& seed) {
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
      dis(0.0f, 0.3f)
{
	auto& vk = rw.get().device();

#pragma region renderPass
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = VkFormat::VK_FORMAT_B8G8R8A8_UNORM;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

	VkAttachmentDescription colorHDRAttachment = {};
	colorHDRAttachment.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
	colorHDRAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorHDRAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorHDRAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorHDRAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorHDRAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorHDRAttachment.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	colorHDRAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

	std::array colorAttachments{ colorAttachment, colorHDRAttachment };

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorHDRAttachmentRef = {};
	colorHDRAttachmentRef.attachment = 1;
	colorHDRAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::array colorAttachmentRefs{

		colorAttachmentRef, colorHDRAttachmentRef
	};

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 2;
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
	renderPassInfo.attachmentCount = 2;
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
#pragma endregion renderPass

#pragma region vertexShaders
	{
		using namespace sdw;

		VertexWriter writer;

		auto inPosition = writer.declInput<Vec2>("inPosition", 0);
		auto outColor = writer.declOutput<Vec4>("fragColor", 0u);

		auto out = writer.getOut();

		// writer.implementFunction<Void>("main", [&]() {
		writer.implementMain([&]() {
			out.vtx.position = vec4(inPosition, 0.0_f, 1.0_f);
			outColor = vec4(inPosition / 2.0_f + 0.5_f, 0.0_f, 1.0_f);
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
#pragma endregion vertexShaders

#pragma region fragmentShaders
	{
		using namespace sdw;

		MandelbulbShaderLib<FragmentWriter> writer;

		auto in = writer.getIn();

		auto inPosition = writer.declInput<Vec4>("inPosition", 0u);
		auto outColorHDR = writer.declOutput<Vec4>("outColorHDR", 1u);
		auto outColor = writer.declOutput<Vec4>("outColor", 0u);

		writer.implementMain([&]() {
			Float seed = writer.declLocale(
			    "seed", cos(in.fragCoord.x()) + sin(in.fragCoord.y()));

			Vec2 uv = writer.declLocale(
			    "uv", (in.fragCoord.xy() +
			           vec2(writer.randomFloat(seed) / writer.Width,
			                writer.randomFloat(seed) / writer.Height) -
			           vec2(writer.Width, writer.Height) / 2.0_f) /
			              vec2(writer.Height));

			Vec3 focalPoint = writer.declLocale(
			    "focalPoint",
			    vec3(uv * writer.CamFocalDistance / writer.CamFocalLength,
			         writer.CamFocalDistance));
			Vec3 aperture = writer.declLocale(
			    "aperture",
			    writer.CamAperture *
			        vec3(writer.sampleAperture(6_i, 0.0_f, seed), 0.0_f));
			Vec3 rayDir =
			    writer.declLocale("rayDir", normalize(focalPoint - aperture));

			Vec3 CamPos =
			    writer.declLocale("CamPos", vec3(0.0_f, 0.0_f, -15.0_f));

			Mat3 CamMatrix = writer.declLocale(
			    "CamMatrix", writer.rotationMatrix(writer.CamRot));
			CamPos = CamMatrix * CamPos;
			rayDir = CamMatrix * rayDir;
			aperture = CamMatrix * aperture;

			outColorHDR.rgb() =
			    writer.pathTrace(vec3(0.0_f, 0.0_f, 0.0_f) + CamPos + aperture,
			                     rayDir, seed) /
			    15.0_f;
			outColorHDR.a() = 0.5_f;

			outColor = outColorHDR;
		});

		/*
		void mainImage(out vec4 fragColor, in vec2 fragCoord)
		{
		    StepSize =
		        min(1.0 / (VolumePrecision * Density), SceneRadius / 2.0);

		    seed = sin(iTime) + cos(fragCoord.x) + sin(fragCoord.y);

		    vec2 uv = (fragCoord + vec2(randomFloat(), randomFloat()) -
		               iResolution.xy / 2.0) /
		              iResolution.y;

		    float samples = texelFetch(iChannel0, ivec2(0, 0), 0).a;
		    if (iFrame > 0)
		        CamRot = getVector(1).xyz;
		    vec4 prevMouse = getVector(2);

		    fragColor = texelFetch(iChannel0, ivec2(fragCoord), 0);

		    bool mouseDragged =
		        iMouse.z >= 0.0 && prevMouse.z >= 0.0 && iMouse != prevMouse;

		    if (mouseDragged)
		        CamRot.yx += (prevMouse.xy - iMouse.xy) / iResolution.y * 2.0;

		    if (iFrame == 0 || mouseDragged)
		    {
		        fragColor = vec4(0.0);
		        samples = 0.0;
		    }

		    setVector(1, vec4(CamRot, 0), fragCoord, fragColor);
		    setVector(2, iMouse, fragCoord, fragColor);
		    if (fragCoord - vec2(.5) == vec2(0))
		        fragColor.a = samples + 1.0;

		    vec3 focalPoint =
		        vec3(uv * CamFocalDistance / CamFocalLength, CamFocalDistance);
		    vec3 aperture = CamAperture * vec3(sampleAperture(6, 0.0), 0.0);
		    vec3 rayDir = normalize(focalPoint - aperture);

		    mat3 CamMatrix = rotationMatrix(CamRot);
		    CamPos *= CamMatrix;
		    rayDir *= CamMatrix;
		    aperture *= CamMatrix;

		    fragColor.rgb =
		        mix(fragColor.rgb,
		            pathTrace(vec3(.0, .0, .0) + CamPos + aperture, rayDir),
		            1.0 / (samples + 1.0));
		}
		//*/

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
#pragma endregion fragmentShaders

#pragma region computeShaders
	{
		using namespace sdw;

		MandelbulbShaderLib<ComputeWriter> writer;

		auto in = writer.getIn();

		writer.inputLayout(8, 8);

		auto ubo = sdw::Ubo(writer, "ubo", 1, 0);
		ubo.declMember<Float>("samples");
		ubo.end();

		auto kernelImage =
		    writer.declImage<RWFImg2DRgba32>("kernelImage", 0, 0);

		writer.implementMain([&]() {
			Float x = writer.declLocale(
			    "x", writer.cast<Float>(in.globalInvocationID.x()));
			Float y = writer.declLocale(
			    "y", writer.cast<Float>(in.globalInvocationID.y()));

			Float samples = writer.declLocale(
			    "samples", ubo.getMember<Float>("samples"));

			Vec2 xy = writer.declLocale(
			    "xy", vec2(x, y) * vec2(samples, samples * samples));

			Float seed = writer.declLocale("seed", cos(x) + sin(y));

			Vec2 uv = writer.declLocale(
			    "uv", (xy +
			           vec2(writer.randomFloat(seed) / writer.Width,
			                writer.randomFloat(seed) / writer.Height) -
			           vec2(writer.Width, writer.Height) / 2.0_f) /
			              vec2(writer.Height));

			IVec2 iuv = writer.declLocale(
			    "iuv",
			    ivec2(writer.cast<Int>(uv.x()), writer.cast<Int>(uv.y())));

			Vec3 focalPoint = writer.declLocale(
			    "focalPoint",
			    vec3(uv * writer.CamFocalDistance / writer.CamFocalLength,
			         writer.CamFocalDistance));
			Vec3 aperture = writer.declLocale(
			    "aperture",
			    writer.CamAperture *
			        vec3(writer.sampleAperture(6_i, 0.0_f, seed), 0.0_f));
			Vec3 rayDir =
			    writer.declLocale("rayDir", normalize(focalPoint - aperture));

			Vec3 CamPos =
			    writer.declLocale("CamPos", vec3(0.0_f, 0.0_f, -15.0_f));

			Mat3 CamMatrix = writer.declLocale(
			    "CamMatrix", writer.rotationMatrix(writer.CamRot));
			CamPos = CamMatrix * CamPos;
			rayDir = CamMatrix * rayDir;
			aperture = CamMatrix * aperture;

			// clang-format off
			Vec3 previousColor = writer.declLocale("previousColor", imageLoad(kernelImage, iuv).rgb());
			Vec3 newColor = writer.declLocale("newColor", writer.pathTrace(CamPos + aperture, rayDir, seed));
			Vec3 mixer = writer.declLocale("mixer", vec3(1.0_f / (samples + 1.0_f)));
			Vec3 mixedColor = writer.declLocale("mixedColor", mix(previousColor, newColor, mixer));
			Vec4 mixedColor4 = writer.declLocale("mixedColor4", vec4(mixedColor, samples + 1.0_f));
			// clang-format on

			/// AAAAAAAAAAAH
			imageStore(kernelImage, iuv, mixedColor4);
		});

		/*
		void mainImage(out vec4 fragColor, in vec2 fragCoord)
		{
		    StepSize =
		        min(1.0 / (VolumePrecision * Density), SceneRadius / 2.0);

		    seed = sin(iTime) + cos(fragCoord.x) + sin(fragCoord.y);

		    vec2 uv = (fragCoord + vec2(randomFloat(), randomFloat()) -
		               iResolution.xy / 2.0) /
		              iResolution.y;

		    float samples = texelFetch(iChannel0, ivec2(0, 0), 0).a;
		    if (iFrame > 0)
		        CamRot = getVector(1).xyz;
		    vec4 prevMouse = getVector(2);

		    fragColor = texelFetch(iChannel0, ivec2(fragCoord), 0);

		    bool mouseDragged =
		        iMouse.z >= 0.0 && prevMouse.z >= 0.0 && iMouse != prevMouse;

		    if (mouseDragged)
		        CamRot.yx += (prevMouse.xy - iMouse.xy) / iResolution.y * 2.0;

		    if (iFrame == 0 || mouseDragged)
		    {
		        fragColor = vec4(0.0);
		        samples = 0.0;
		    }

		    setVector(1, vec4(CamRot, 0), fragCoord, fragColor);
		    setVector(2, iMouse, fragCoord, fragColor);
		    if (fragCoord - vec2(.5) == vec2(0))
		        fragColor.a = samples + 1.0;

		    vec3 focalPoint =
		        vec3(uv * CamFocalDistance / CamFocalLength, CamFocalDistance);
		    vec3 aperture = CamAperture * vec3(sampleAperture(6, 0.0), 0.0);
		    vec3 rayDir = normalize(focalPoint - aperture);

		    mat3 CamMatrix = rotationMatrix(CamRot);
		    CamPos *= CamMatrix;
		    rayDir *= CamMatrix;
		    aperture *= CamMatrix;

		    fragColor.rgb =
		        mix(fragColor.rgb,
		            pathTrace(vec3(.0, .0, .0) + CamPos + aperture, rayDir),
		            1.0 / (samples + 1.0));
		}
		//*/

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
#pragma endregion computeShaders

#pragma region pipelineLayout
	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	m_pipelineLayout = vk.create(pipelineLayoutInfo);
	if (!m_pipelineLayout)
	{
		std::cerr << "error: failed to create pipeline layout" << std::endl;
		abort();
	}
#pragma endregion pipelineLayout

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
	// inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
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
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
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
	// colorBlendAttachment.blendEnable = false;
	// colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	// colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	// colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	// colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	// colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	// colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	colorBlendAttachment.blendEnable = true;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor =
	    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

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

	std::array colorBlendAttachments{ colorBlendAttachment,
		                              colorHDRBlendAttachment };

	vk::PipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 2;
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
#pragma endregion pipeline

#pragma region compute descriptor pool
	std::array poolSizes{
		VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
	};

	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.maxSets = 1;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = poolSizes.data();

	m_computePool = vk.create(poolInfo);
	if (!m_computePool)
	{
		std::cerr << "error: failed to create compute descriptor pool"
		          << std::endl;
		abort();
	}
#pragma endregion compute descriptor pool

#pragma region compute descriptor set layout
	VkDescriptorSetLayoutBinding layoutBindingKernelImage;
	layoutBindingKernelImage.binding = 0;
	layoutBindingKernelImage.descriptorCount = 1;
	layoutBindingKernelImage.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	layoutBindingKernelImage.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding layoutBindingUbo;
	layoutBindingUbo.binding = 1;
	layoutBindingUbo.descriptorCount = 1;
	layoutBindingUbo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBindingUbo.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	std::array layoutBindings{
		layoutBindingKernelImage,
		layoutBindingUbo
	};

	vk::DescriptorSetLayoutCreateInfo setLayoutInfo;
	setLayoutInfo.bindingCount = 2;
	setLayoutInfo.pBindings = layoutBindings.data();

	m_computeSetLayout = vk.create(setLayoutInfo);
	if (!m_computeSetLayout)
	{
		std::cerr << "error: failed to create compute set layout" << std::endl;
		abort();
	}
#pragma endregion compute descriptor set layout

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
#pragma endregion compute descriptor set

#pragma region compute pipeline layout
	VkPushConstantRange pcRange{};
	pcRange.size = sizeof(float);
	pcRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	vk::PipelineLayoutCreateInfo computePipelineLayoutInfo;
	computePipelineLayoutInfo.setLayoutCount = 1;
	computePipelineLayoutInfo.pSetLayouts = &m_computeSetLayout.get();
	computePipelineLayoutInfo.pushConstantRangeCount = 1;
	computePipelineLayoutInfo.pPushConstantRanges = &pcRange;

	m_computePipelineLayout = vk.create(computePipelineLayoutInfo);
	if (!m_computePipelineLayout)
	{
		std::cerr << "error: failed to create compute pipeline layout"
		          << std::endl;
		abort();
	}
#pragma endregion compute pipeline layout

#pragma region compute pipeline
	vk::ComputePipelineCreateInfo computePipelineInfo;
	computePipelineInfo.layout = m_pipelineLayout.get();
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
#pragma endregion compute pipeline

#pragma region vertexBuffer
	using Vertex = std::array<float, 2>;
	std::vector<Vertex> vertices(POINT_COUNT);
	// std::vector<Vertex> vertices(3);

	// vertices[0] = { -1, -1 };
	// vertices[1] = { 2, -1 };
	// vertices[2] = { -1, 2 };

	for (auto& vertex : vertices)
	{
		vertex[0] = dis(gen);
		vertex[1] = dis(gen);
	}

	vk::BufferCreateInfo vbInfo;
	vbInfo.size = sizeof(Vertex) * vertices.size();
	vbInfo.usage =
	    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vbInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo vbAllocCreateInfo = {};
	vbAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	// vbAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	vbAllocCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

	VmaAllocationInfo stagingVertexBufferAllocInfo = {};
	vmaCreateBuffer(vk.allocator(), &vbInfo, &vbAllocCreateInfo,
	                &m_vertexBuffer.get(), &m_vertexBufferAllocation.get(),
	                &stagingVertexBufferAllocInfo);

	void* data;

	vmaMapMemory(vk.allocator(), m_vertexBufferAllocation.get(), &data);

	std::copy(vertices.begin(), vertices.end(), static_cast<Vertex*>(data));

	vmaUnmapMemory(vk.allocator(), m_vertexBufferAllocation.get());
#pragma endregion vertexBuffer

#pragma region compute UBO
	vk::BufferCreateInfo uboInfo;
	uboInfo.size = sizeof(float);
	uboInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	uboInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo uboAllocCreateInfo = {};
	uboAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	// uboAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	uboAllocCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

	VmaAllocationInfo stagingUboAllocInfo = {};
	vmaCreateBuffer(vk.allocator(), &vbInfo, &vbAllocCreateInfo,
	                &m_vertexBuffer.get(), &m_vertexBufferAllocation.get(),
	                &stagingUboAllocInfo);

	{
		void* data;

		vmaMapMemory(vk.allocator(), m_vertexBufferAllocation.get(), &data);

		*static_cast<float*>(data) = 1.0f;

		//std::copy(vertices.begin(), vertices.end(), static_cast<Vertex*>(data));

		vmaUnmapMemory(vk.allocator(), m_vertexBufferAllocation.get());
	}

	VkDescriptorBufferInfo setBufferInfo{};
	setBufferInfo.buffer = m_computeUbo.get();
	setBufferInfo.range = sizeof(float);

	vk::WriteDescriptorSet uboWrite;
	uboWrite.descriptorCount = 1;
	uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboWrite.dstArrayElement = 0;
	uboWrite.dstBinding = 1;
	uboWrite.dstSet = m_computeSet.get();
	uboWrite.pBufferInfo = &setBufferInfo;

	vk.updateDescriptorSets(uboWrite);
#pragma endregion compute UBO

#pragma region outputImage
	vk::ImageCreateInfo imageInfo;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	// VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.usage =
	    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0;

	VmaAllocationCreateInfo imageAllocCreateInfo = {};
	imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	imageAllocCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	vmaCreateImage(vk.allocator(), &imageInfo, &imageAllocCreateInfo,
	               &m_outputImage.get(), &m_outputImageAllocation.get(),
	               nullptr);

	vk.debugMarkerSetObjectName(m_outputImage.get(),
	                            VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
	                            "mandelbulb_output_image");

	CommandBuffer transitionCB(vk, rw.get().commandPool());

	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	transitionCB.begin(beginInfo);
	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_outputImage.get();
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;
	transitionCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
	                             VK_DEPENDENCY_DEVICE_GROUP_BIT, barrier);
	if (transitionCB.end() != VK_SUCCESS)
	{
		std::cerr << "error: failed to record command buffer" << std::endl;
		abort();
	}

	vk::SubmitInfo submitInfo;

	// VkSemaphore waitSemaphores[] = {
	//	rw.get().currentImageAvailableSemaphore()
	//};
	// VkPipelineStageFlags waitStages[] = {
	//	VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	//};
	// submitInfo.waitSemaphoreCount = 1;
	// submitInfo.pWaitSemaphores = waitSemaphores;
	// submitInfo.pWaitDstStageMask = waitStages;

	// auto cb = m_commandBuffers[rw.get().imageIndex()].commandBuffer();
	auto cb = transitionCB.commandBuffer();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cb;
	// VkSemaphore signalSemaphores[] = {
	//	m_renderFinishedSemaphores[rw.get().currentFrame()]
	//};
	// submitInfo.signalSemaphoreCount = 1;
	// submitInfo.pSignalSemaphores = signalSemaphores;

	// VkFence inFlightFence = rw.get().currentInFlightFences();

	// vk.ResetFences(vk.vkDevice(), 1, &inFlightFence);

	if (vk.queueSubmit(vk.graphicsQueue(), submitInfo) != VK_SUCCESS)
	{
		std::cerr << "error: failed to submit draw command buffer"
		          << std::endl;
		abort();
	}
	vk.wait(vk.graphicsQueue());

	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = m_outputImage.get();
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	m_outputImageView = vk.create(viewInfo);
	if (!m_outputImageView)
	{
		std::cerr << "error: failed to create image view" << std::endl;
		abort();
	}
#pragma endregion outputImage

#pragma region outputImageHDR
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	// VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
	                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
	                  VK_IMAGE_USAGE_STORAGE_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0;

	imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	imageAllocCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	vmaCreateImage(vk.allocator(), &imageInfo, &imageAllocCreateInfo,
	               &m_outputImageHDR.get(), &m_outputImageHDRAllocation.get(),
	               nullptr);

	vk.debugMarkerSetObjectName(m_outputImageHDR.get(),
	                            VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
	                            "mandelbulb_output_image_HDR");

	{
		CommandBuffer transitionCB(vk, rw.get().commandPool());

		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		transitionCB.begin(beginInfo);
		vk::ImageMemoryBarrier barrier;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_outputImageHDR.get();
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;
		transitionCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		                             VK_DEPENDENCY_DEVICE_GROUP_BIT, barrier);
		if (transitionCB.end() != VK_SUCCESS)
		{
			std::cerr << "error: failed to record command buffer" << std::endl;
			abort();
		}

		vk::SubmitInfo submitInfo;

		// VkSemaphore waitSemaphores[] = {
		//	rw.get().currentImageAvailableSemaphore()
		//};
		// VkPipelineStageFlags waitStages[] = {
		//	VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		//};
		// submitInfo.waitSemaphoreCount = 1;
		// submitInfo.pWaitSemaphores = waitSemaphores;
		// submitInfo.pWaitDstStageMask = waitStages;

		// auto cb = m_commandBuffers[rw.get().imageIndex()].commandBuffer();
		auto cb = transitionCB.commandBuffer();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cb;
		// VkSemaphore signalSemaphores[] = {
		//	m_renderFinishedSemaphores[rw.get().currentFrame()]
		//};
		// submitInfo.signalSemaphoreCount = 1;
		// submitInfo.pSignalSemaphores = signalSemaphores;

		// VkFence inFlightFence = rw.get().currentInFlightFences();

		// vk.ResetFences(vk.vkDevice(), 1, &inFlightFence);

		if (vk.queueSubmit(vk.graphicsQueue(), submitInfo) != VK_SUCCESS)
		{
			std::cerr << "error: failed to submit draw command buffer"
			          << std::endl;
			abort();
		}
		vk.wait(vk.graphicsQueue());
	}

	// vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = m_outputImageHDR.get();
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	m_outputImageHDRView = vk.create(viewInfo);
	if (!m_outputImageHDRView)
	{
		std::cerr << "error: failed to create image view" << std::endl;
		abort();
	}

	VkDescriptorImageInfo setImageInfo{};
	setImageInfo.imageView = m_outputImageHDRView.get();
	setImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	vk::WriteDescriptorSet write;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	write.dstArrayElement = 0;
	write.dstBinding = 0;
	write.dstSet = m_computeSet.get();
	write.pImageInfo = &setImageInfo;

	vk.updateDescriptorSets(write);
#pragma endregion outputImageHDR

#pragma region framebuffer
	vk::FramebufferCreateInfo framebufferInfo;
	framebufferInfo.renderPass = m_renderPass.get();
	framebufferInfo.attachmentCount = 2;
	std::array attachments = { m_outputImageView.get(),
		                       m_outputImageHDRView.get() };
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
#pragma endregion framebuffer
}  // namespace cdm

Mandelbulb::~Mandelbulb()
{
	auto& vk = rw.get().device();

	vmaDestroyImage(vk.allocator(), m_outputImage.get(),
	                m_outputImageAllocation.get());
	vmaDestroyImage(vk.allocator(), m_outputImageHDR.get(),
	                m_outputImageHDRAllocation.get());
	vmaDestroyBuffer(vk.allocator(), m_vertexBuffer.get(),
	                 m_vertexBufferAllocation.get());
}

void Mandelbulb::render(CommandBuffer& cb)
{
	// uint32_t width = 1280 * 4;
	// uint32_t height = 720 * 4;

	std::array clearValues = { VkClearValue{}, VkClearValue{} };

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
	cb.bindVertexBuffer(m_vertexBuffer.get());
	cb.draw(POINT_COUNT);
	// cb.draw(3);

	cb.endRenderPass2(subpassEndInfo);
}

void Mandelbulb::compute(CommandBuffer& cb)
{
	cb.bindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline.get());
	cb.bindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE,
	                     m_computePipelineLayout.get(), 0, m_computeSet.get());
	cb.dispatch(8, 8);
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

	void* data;

	vmaMapMemory(vk.allocator(), m_vertexBufferAllocation.get(), &data);

	std::copy(vertices.begin(), vertices.end(), static_cast<Vertex*>(data));

	vmaUnmapMemory(vk.allocator(), m_vertexBufferAllocation.get());
}
}  // namespace cdm
