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

		FragmentWriter writer;

		auto in = writer.getIn();

		auto inPosition = writer.declInput<Vec4>("inPosition", 0u);
		auto outColorHDR = writer.declOutput<Vec4>("outColorHDR", 1u);
		auto outColor = writer.declOutput<Vec4>("outColor", 0u);

		//*

		// clang-format off
#define LocaleFloat( Writer, Name )\
	Float Name = Writer.declLocale< Float >( #Name );\
	Name

#define LocaleDouble( Writer, Name )\
	Double Name = Writer.declLocale< Double >( #Name );\
	Name

#define LocaleInt( Writer, Name )\
	Int Name = Writer.declLocale< Int >( #Name );\
	Name

#define LocaleUInt( Writer, Name )\
	UInt Name = Writer.declLocale< UInt >( #Name );\
	Name

#define LocaleVec2( Writer, Name )\
	Vec2 Name = Writer.declLocale< Vec2 >( #Name );\
	Name

#define LocaleVec3( Writer, Name )\
	Vec3 Name = Writer.declLocale< Vec3 >( #Name );\
	Name

#define LocaleVec4( Writer, Name )\
	Vec4 Name = Writer.declLocale< Vec4 >( #Name );\
	Name

#define LocaleDVec2( Writer, Name )\
	DVec2 Name = Writer.declLocale< DVec2 >( #Name );\
	Name

#define LocaleDVec3( Writer, Name )\
	DVec3 Name = Writer.declLocale< DVec3 >( #Name );\
	Name

#define LocaleDVec4( Writer, Name )\
	DVec4 Name = Writer.declLocale< DVec4 >( #Name );\
	Name

#define LocaleIVec2( Writer, Name )\
	IVec2 Name = Writer.declLocale< IVec2 >( #Name );\
	Name

#define LocaleIVec3( Writer, Name )\
	IVec3 Name = Writer.declLocale< IVec3 >( #Name );\
	Name

#define LocaleIVec4( Writer, Name )\
	IVec4 Name = Writer.declLocale< IVec4 >( #Name );\
	Name

#define LocaleUVec2( Writer, Name )\
	UVec2 Name = Writer.declLocale< UVec2 >( #Name );\
	Name

#define LocaleUVec3( Writer, Name )\
	UVec3 Name = Writer.declLocale< UVec3 >( #Name );\
	Name

#define LocaleUVec4( Writer, Name )\
	UVec4 Name = Writer.declLocale< UVec4 >( #Name );\
	Name

#define LocaleBVec2( Writer, Name )\
	BVec2 Name = Writer.declLocale< BVec2 >( #Name );\
	Name

#define LocaleBVec3( Writer, Name )\
	BVec3 Name = Writer.declLocale< BVec3 >( #Name );\
	Name

#define LocaleBVec4( Writer, Name )\
	BVec4 Name = Writer.declLocale< BVec4 >( #Name );\
	Name

#define LocaleMat2( Writer, Name )\
	Mat2 Name = Writer.declLocale< Mat2 >( #Name );\
	Name

#define LocaleMat2x3( Writer, Name )\
	Mat2x3 Name = Writer.declLocale< Mat2x3 >( #Name );\
	Name

#define LocaleMat2x4( Writer, Name )\
	Mat2x4 Name = Writer.declLocale< Mat2x4 >( #Name );\
	Name

#define LocaleMat3( Writer, Name )\
	Mat3 Name = Writer.declLocale< Mat3 >( #Name );\
	Name

#define LocaleMat3x2( Writer, Name )\
	Mat3x2 Name = Writer.declLocale< Mat3x2 >( #Name );\
	Name

#define LocaleMat3x4( Writer, Name )\
	Mat3x4 Name = Writer.declLocale< Mat3x4 >( #Name );\
	Name

#define LocaleMat4( Writer, Name )\
	Mat4 Name = Writer.declLocale< Mat4 >( #Name );\
	Name

#define LocaleMat4x2( Writer, Name )\
	Mat4x2 Name = Writer.declLocale< Mat4x2 >( #Name );\
	Name

#define LocaleMat4x3( Writer, Name )\
	Mat4x3 Name = Writer.declLocale< Mat4x3 >( #Name );\
	Name

#define LocaleDMat2( Writer, Name )\
	DMat2 Name = Writer.declLocale< DMat2 >( #Name );\
	Name

#define LocaleDMat2x3( Writer, Name )\
	DMat2x3 Name = Writer.declLocale< DMat2x3 >( #Name );\
	Name

#define LocaleDMat2x4( Writer, Name )\
	DMat2x4 Name = Writer.declLocale< DMat2x4 >( #Name );\
	Name

#define LocaleDMat3( Writer, Name )\
	DMat3 Name = Writer.declLocale< DMat3 >( #Name );\
	Name

#define LocaleDMat3x2( Writer, Name )\
	DMat3x2 Name = Writer.declLocale< DMat3x2 >( #Name );\
	Name

#define LocaleDMat3x4( Writer, Name )\
	DMat3x4 Name = Writer.declLocale< DMat3x4 >( #Name );\
	Name

#define LocaleDMat4( Writer, Name )\
	DMat4 Name = Writer.declLocale< DMat4 >( #Name );\
	Name

#define LocaleDMat4x2( Writer, Name )\
	DMat4x2 Name = Writer.declLocale< DMat4x2 >( #Name );\
	Name

#define LocaleDMat4x3( Writer, Name )\
	DMat4x3 Name = Writer.declLocale< DMat4x3 >( #Name );\
	Name
		// clang-format on

#define Pi 3.14159265359_f

#define VolumePrecision 0.35_f
#define SceneRadius 1.5_f
#define StepsSkipShadow 4.0_f
#define MaxSteps 500_i
#define MaxAbso 0.7_f
#define MaxShadowAbso 0.7_f

		auto Width = writer.declConstant<Float>("Width", Float(float(width)));
		auto Height =
		    writer.declConstant<Float>("Height", Float(float(height)));

		auto CamRot =
		    writer.declConstant<Vec3>("CamRot", vec3(0.7_f, -2.4_f, 0.0_f));

#define CamFocalLength 5.0_f
#define CamFocalDistance 14.2_f
#define CamAperture 0.1_f

		auto LightColor = writer.declConstant<Vec3>("LightColor", vec3(3.0_f));
		auto LightDir =
		    writer.declConstant<Vec3>("LightDir", vec3(-1.0_f, -2.0_f, 0.0_f));

#define Power 8.0_f

#define Density 500.0_f

#define StepSize min(1.0_f / (VolumePrecision * Density), SceneRadius / 2.0_f)

		auto randomFloat = writer.implementFunction<Float>(
		    "randomFloat",
		    [&](Float& seed) {
			    LocaleFloat(writer, res) = fract(sin(seed) * 43758.5453_f);
			    seed = seed + 1.0_f;
			    writer.returnStmt(res);
		    },
		    InOutFloat{ writer, "seed" });

		auto rotationMatrix = writer.implementFunction<Mat3>(
		    "rotationMatrix",
		    [&](const Vec3& rotEuler) {
			    LocaleFloat(writer, c) = cos(rotEuler.x());
			    LocaleFloat(writer, s) = sin(rotEuler.x());
			    LocaleMat3(writer, rx) =
			        mat3(vec3(1.0_f, 0.0_f, 0.0_f), vec3(0.0_f, c, -s),
			             vec3(0.0_f, s, c));
			    c = cos(rotEuler.y());
			    s = sin(rotEuler.y());
			    LocaleMat3(writer, ry) =
			        mat3(vec3(c, 0.0_f, -s), vec3(0.0_f, 1.0_f, 0.0_f),
			             vec3(s, 0.0_f, c));
			    c = cos(rotEuler.z());
			    s = sin(rotEuler.z());
			    LocaleMat3(writer, rz) =
			        mat3(vec3(c, -s, 0.0_f), vec3(s, c, 0.0_f),
			             vec3(0.0_f, 0.0_f, 1.0_f));
			    writer.returnStmt(rz * rx * ry);
		    },
		    InVec3{ writer, "rotEuler" });

		auto maxV = writer.implementFunction<Float>(
		    "maxV",
		    [&](const Vec3& v) {
			    writer.returnStmt(writer.ternary(
			        v.x() > v.y(), writer.ternary(v.x() > v.z(), v.x(), v.z()),
			        writer.ternary(v.y() > v.z(), v.y(), v.z())));
		    },
		    InVec3{ writer, "v" });

		auto randomDir = writer.implementFunction<Vec3>(
		    "randomDir",
		    [&](Float& seed) {
			    writer.returnStmt(
			        vec3(1.0_f, 0.0_f, 0.0_f) *
			        rotationMatrix(vec3(randomFloat(seed) * 2.0_f * Pi, 0.0_f,
			                            randomFloat(seed) * 2.0_f * Pi)));
		    },
		    InOutFloat{ writer, "seed" });

		auto backgroundColor = writer.implementFunction<Vec3>(
		    "backgroundColor",
		    [&](const Vec3&) { writer.returnStmt(vec3(0.0_f)); },
		    InVec3{ writer, "unused" });

		auto distanceEstimation = writer.implementFunction<Float>(
		    "distanceEstimation",
		    [&](const Vec3& pos_arg, Vec3& volumeColor, Vec3& emissionColor) {
			    // Vec3 pos = writer.declLocale<Vec3>writer, ("pos_local");
			    LocaleVec3(writer, pos) = pos_arg;
			    LocaleVec3(writer, basePos) = vec3(0.0_f);
			    LocaleFloat(writer, scale) = 1.0_f;

			    // pos = pos_arg;
			    // Vec3 basePos = writer.declLocale<Vec3>writer, ("basePos");
			    //   basePos = vec3(0.0_f);
			    //   Float scale = 1.0_f;

			    //   pos /= scale;
			    //   pos += basePos;

			    volumeColor = vec3(0.0_f);
			    emissionColor = vec3(0.0_f);

			    pos.yz() = vec2(pos.z(), pos.y());

			    LocaleFloat(writer, r) = length(pos);
			    LocaleVec3(writer, z) = pos;
			    LocaleVec3(writer, c) = pos;
			    LocaleFloat(writer, dr) = 1.0_f;
			    LocaleFloat(writer, theta) = 0.0_f;
			    LocaleFloat(writer, phi) = 0.0_f;
			    LocaleVec3(writer, orbitTrap) = vec3(1.0_f);

			    // Float r = length(pos);
			    // Vec3 z = pos;
			    // Vec3 c = pos;
			    // Float dr = 1.0_f, theta = 0.0_f, phi = 0.0_f;
			    // Vec3 orbitTrap = vec3(1.0_f);

			    FOR(writer, Int, i, 0_i, i < 8_i, ++i)
			    {
				    r = length(z);
				    IF(writer, r > SceneRadius) { writer.loopBreakStmt(); }
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

			    LocaleFloat(writer, dist) = 0.5_f * log(r) * r / dr * scale;

			    volumeColor = (1.0_f - orbitTrap) * 0.98_f;

			    emissionColor = vec3(
			        writer.ternary(orbitTrap.z() < 0.0001_f, 20.0_f, 0.0_f));

			    writer.returnStmt(dist);
		    },
		    InVec3{ writer, "pos_arg" }, OutVec3{ writer, "volumeColor" },
		    OutVec3{ writer, "emissionColor" });

		auto directLight = writer.implementFunction<Vec3>(
		    "directLight",
		    [&](const Vec3& pos_arg, Float& seed) {
			    LocaleVec3(writer, pos) = pos_arg;

			    LocaleVec3(writer, absorption) = vec3(1.0_f);
			    LocaleVec3(writer, volumeColor) = vec3(0.0_f);
			    LocaleVec3(writer, emissionColor) = vec3(0.0_f);

			    FOR(writer, Int, i, 0_i, i < MaxSteps, ++i)
			    {
				    LocaleFloat(writer, dist) =
				        distanceEstimation(pos, volumeColor, emissionColor);
				    pos -= LightDir * max(dist, StepSize);

				    IF(writer, dist < StepSize)
				    {
					    LocaleFloat(writer, abStep) =
					        StepSize * randomFloat(seed);
					    pos -= LightDir * (abStep - StepSize);
					    IF(writer, dist < 0.0_f)
					    {
						    LocaleFloat(writer, absorbance) =
						        exp(-Density * abStep);
						    absorption *= absorbance;
						    IF(writer,
						       maxV(absorption) < 1.0_f - MaxShadowAbso)
						    {
							    writer.loopBreakStmt();
						    }
						    FI;
					    }
					    FI;
				    }
				    FI;

				    IF(writer, length(pos) > SceneRadius)
				    {
					    writer.loopBreakStmt();
				    }
				    FI;
			    }
			    ROF;

			    writer.returnStmt(
			        LightColor *
			        max((absorption + MaxShadowAbso - 1.0_f) / MaxShadowAbso,
			            vec3(0.0_f)));
		    },
		    InVec3{ writer, "pos_arg" }, InOutFloat{ writer, "seed" });

		auto pathTrace = writer.implementFunction<Vec3>(
		    "pathTrace",
		    [&](const Vec3& rayPos_arg, const Vec3& rayDir_arg, Float& seed) {
			    LocaleVec3(writer, rayPos) = rayPos_arg;
			    LocaleVec3(writer, rayDir) = rayDir_arg;

			    rayPos += rayDir * max(length(rayPos) - SceneRadius, 0.0_f);

			    LocaleVec3(writer, outColor) = vec3(0.0_f);
			    LocaleVec3(writer, absorption) = vec3(1.0_f);

			    LocaleVec3(writer, volumeColor) = vec3(0.0_f);
			    LocaleVec3(writer, emissionColor) = vec3(0.0_f);

			    FOR(writer, Int, i, 0_i, i < MaxSteps, ++i)
			    {
				    LocaleFloat(writer, dist) =
				        distanceEstimation(rayPos, volumeColor, emissionColor);
				    rayPos += rayDir * max(dist, StepSize);
				    IF(writer, dist < StepSize && length(rayPos) < SceneRadius)
				    {
					    LocaleFloat(writer, abStep) =
					        StepSize * randomFloat(seed);
					    rayPos += rayDir * (abStep - StepSize);
					    IF(writer, dist < 0.0_f)
					    {
						    LocaleFloat(writer, absorbance) =
						        exp(-Density * abStep);
						    LocaleFloat(writer, transmittance) =
						        1.0_f - absorbance;

						    // surface glow for a nice additional effect
						    // if(dist > -.0001) outColor += absorption *
						    // vec3(.2, .2, .2);

						    // if(randomFloat() < ShadowRaysPerStep)
						    // emissionColor
						    // += 1.0/ShadowRaysPerStep * volumeColor *
						    // directLight(rayPos);

						    LocaleFloat(writer, i_f) = writer.cast<Float>(i);

						    IF(writer,
						       mod(i_f, Float(StepsSkipShadow)) == 0.0_f)
						    {
							    emissionColor += Float(StepsSkipShadow) *
							                     volumeColor *
							                     directLight(rayPos, seed);
						    }
						    FI;

						    outColor +=
						        absorption * transmittance * emissionColor;

						    IF(writer, maxV(absorption) < 1.0_f - MaxAbso)
						    {
							    writer.loopBreakStmt();
						    }
						    FI;

						    IF(writer, randomFloat(seed) > absorbance)
						    {
							    rayDir = randomDir(seed);
							    absorption *= volumeColor;
						    }
						    FI;
					    }
					    FI;
				    }
				    FI;

				    IF(writer, length(rayPos) > SceneRadius &&
				                   dot(rayDir, rayPos) > 0.0_f)
				    {
					    writer.returnStmt(outColor + backgroundColor(rayDir) *
					                                     absorption);
				    }
				    FI;
			    }
			    ROF;

			    writer.returnStmt(outColor);
		    },
		    InVec3{ writer, "rayPos_arg" }, InVec3{ writer, "rayDir_arg" },
		    InOutFloat{ writer, "seed" });

		// n-blade aperture
		auto sampleAperture = writer.implementFunction<Vec2>(
		    "sampleAperture",
		    [&](const Int& nbBlades, const Float& rotation, Float& seed) {
			    LocaleFloat(writer, alpha) =
			        2.0_f * Pi / writer.cast<Float>(nbBlades);
			    LocaleFloat(writer, side) = sin(alpha / 2.0_f);

			    LocaleInt(writer, blade) = writer.cast<Int>(
			        randomFloat(seed) * writer.cast<Float>(nbBlades));

			    LocaleVec2(writer, tri) =
			        vec2(randomFloat(seed), -randomFloat(seed));
			    IF(writer, tri.x() + tri.y() > 0.0_f)
			    {
				    tri = vec2(tri.x() - 1.0_f, -1.0_f - tri.y());
			    }
			    FI;
			    tri.x() *= side;
			    tri.y() *= sqrt(1.0_f - side * side);

			    LocaleFloat(writer, angle) =
			        rotation + writer.cast<Float>(blade) /
			                       writer.cast<Float>(nbBlades) * 2.0_f * Pi;

			    writer.returnStmt(
			        vec2(tri.x() * cos(angle) + tri.y() * sin(angle),
			             tri.y() * cos(angle) - tri.x() * sin(angle)));
		    },
		    InInt{ writer, "nbBlades" }, InFloat{ writer, "rotation" },
		    InOutFloat{ writer, "seed" });

		//// used to store values in the unused alpha channel of the buffer
		// auto setVector = writer.implementFunction<Void>(
		//    "setVector",
		//    [&](const Int& index, const Vec4& v, const Vec2& fragCoord_arg,
		//        Vec4& fragColor) {
		//	    LocaleVec2(writer, fragCoord) = fragCoord_arg;
		//	    fragCoord -= vec2(0.5_f);
		//	    IF(writer, fragCoord.y() == writer.cast<Float>(index))
		//	    {
		//		    IF(writer, fragCoord.x() == 0.0_f)
		//		    {
		//			    fragColor.a() = v.x();
		//		    }
		//		    FI;
		//		    IF(writer, fragCoord.x() == 1.0_f)
		//		    {
		//			    fragColor.a() = v.y();
		//		    }
		//		    FI;
		//		    IF(writer, fragCoord.x() == 2.0_f)
		//		    {
		//			    fragColor.a() = v.z();
		//		    }
		//		    FI;
		//		    IF(writer, fragCoord.x() == 3.0_f)
		//		    {
		//			    fragColor.a() = v.a();
		//		    }
		//		    FI;
		//	    }
		//	    FI;
		//    },
		//    InInt{ writer, "index" }, InVec4{ writer, "v" },
		//    InVec2{ writer, "fragCoord_arg" },
		//    InOutVec4{ writer, "fragColor" });

		//*/

		// auto getVector = writer.implementFunction<Vec4>(
		//    "getVector",
		//    [&](Int index) {
		//	    writer.returnStmt(
		//	        vec4(texelFetch(iChannel0, ivec2(0, index), 0).a,
		//	             texelFetch(iChannel0, ivec2(1, index), 0).a,
		//	             texelFetch(iChannel0, ivec2(2, index), 0).a,
		//	             texelFetch(iChannel0, ivec2(3, index), 0).a));
		//    },
		//    InInt{ writer, "index" });

		// writer.implementFunction<Void>("main", [&]() {
		writer.implementMain([&]() {
			//*
			LocaleFloat(writer, seed) =
			    cos(in.fragCoord.x()) + sin(in.fragCoord.y());

			// LocaleFloat(writer, width) = 1280.0_f * 4.0_f;
			// LocaleFloat(writer, height) = 720.0_f * 4.0_f;

			// outColor = vec4(0.0_f, 0.0_f, 0.0_f, 0.0_f);
			// outColor = in.fragCoord / vec4(1280.0_f, 720.0_f, 1.0_f, 1.0_f);
			// LocaleVec4(writer, uv) = in.fragCoord / vec4(1280.0_f,
			// 720.0_f, 1.0_f, 1.0_f);
			// LocaleVec2(writer, uv) = (in.fragCoord.xy() / vec2(1280.0_f,
			// 720.0_f) -
			//                  vec2(0.5_f));  // / vec2(720.0_f);
			LocaleVec2(writer, uv) =
			    (in.fragCoord.xy() +
			     vec2(randomFloat(seed) / Width, randomFloat(seed) / Height) -
			     vec2(Width, Height) / 2.0_f) /
			    vec2(Height);
			// LocaleVec2(writer, uv) = (in.fragCoord.xy() - vec2(1280.0_f,
			// 720.0_f) / 2.0_f) / vec2(720.0_f);

			// LocaleVec2(writer, uv) = (in.fragCoord + vec2(randomFloat(),
			// randomFloat()) - iResolution.xy / 2.0) / iResolution.y;

			// float samples = texelFetch(iChannel0, ivec2(0, 0), 0).a;
			// if (iFrame > 0)
			// CamRot = getVector(1).xyz;
			// vec4 prevMouse = getVector(2);

			// fragColor = texelFetch(iChannel0, ivec2(fragCoord), 0);

			// bool mouseDragged =
			// iMouse.z >= 0.0 && prevMouse.z >= 0.0 && iMouse != prevMouse;

			// if (mouseDragged)
			// CamRot.yx += (prevMouse.xy - iMouse.xy) / iResolution.y * 2.0;

			// if (iFrame == 0 || mouseDragged)
			//{
			//    fragColor = vec4(0.0);
			//    samples = 0.0;
			//}

			// setVector(1, vec4(CamRot, 0), fragCoord, fragColor);
			// setVector(2, iMouse, fragCoord, fragColor);

			//   IF (writer, in.fragCoord - vec2(0.5_f) == vec2(0.0_f))
			//{
			//	fragColor.a = samples + 1.0;
			//}
			// FI

			LocaleVec3(writer, focalPoint) =
			    vec3(uv * CamFocalDistance / CamFocalLength, CamFocalDistance);
			LocaleVec3(writer, aperture) =
			    CamAperture * vec3(sampleAperture(6_i, 0.0_f, seed), 0.0_f);
			LocaleVec3(writer, rayDir) = normalize(focalPoint - aperture);

			LocaleVec3(writer, CamPos) = vec3(0.0_f, 0.0_f, -15.0_f);

			LocaleMat3(writer, CamMatrix) = rotationMatrix(CamRot);
			CamPos = CamMatrix * CamPos;
			rayDir = CamMatrix * rayDir;
			aperture = CamMatrix * aperture;

			outColorHDR.rgb() =
			    pathTrace(vec3(0.0_f, 0.0_f, 0.0_f) + CamPos + aperture,
			              rayDir, seed) /
			    15.0_f;
			outColorHDR.a() = 0.5_f;

			outColor = outColorHDR;
			//*/

			// mix(outColor.rgb,
			//    pathTrace(vec3(0.0_f, 0.0_f, 0.0_f) + CamPos + aperture,
			//    rayDir), 1.0_f / (samples + 1.0_f));

			// outColor = in.fragCoord / vec4(1280.0_f, 720.0_f, 1.0_f, 1.0_f);
		});
		// writer.implementFunction<Void>("main", [&]() { outColor =
		// vec4(in.pointCoord, 0.0_f, 1.0_f); });
		//[&]() { outColor = inColor; });

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

		// writer.implementFunction<void>("main",
		//                               //[&]() { outColor = in.fragCoord; });
		//                               [&]() { outColor = inColor; });
		//[&]() { outColor = vec4(0.0_f, 0.0_f, 1.0_f, 1.0_f); });

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

		ComputeWriter writer;

		auto in = writer.getIn();

		writer.inputLayout(8, 8);

		auto samplesPC = Pcb(writer, "pc");
		samplesPC.declMember<Float>("samples");
		samplesPC.end();

		// auto inPosition = writer.declInput<Vec4>("inPosition", 0u);
		// auto outColorHDR = writer.declOutput<Vec4>("outColorHDR", 1u);
		// auto outColor = writer.declOutput<Vec4>("outColor", 0u);

		//*

#define Pi 3.14159265359_f

#define VolumePrecision 0.35_f
#define SceneRadius 1.5_f
#define StepsSkipShadow 4.0_f
#define MaxSteps 500_i
#define MaxAbso 0.7_f
#define MaxShadowAbso 0.7_f

		auto Width = writer.declConstant<Float>("Width", Float(float(width)));
		auto Height =
		    writer.declConstant<Float>("Height", Float(float(height)));

		auto CamRot =
		    writer.declConstant<Vec3>("CamRot", vec3(0.7_f, -2.4_f, 0.0_f));

#define CamFocalLength 5.0_f
#define CamFocalDistance 14.2_f
#define CamAperture 0.1_f

		auto LightColor = writer.declConstant<Vec3>("LightColor", vec3(3.0_f));
		auto LightDir =
		    writer.declConstant<Vec3>("LightDir", vec3(-1.0_f, -2.0_f, 0.0_f));

#define Power 8.0_f

#define Density 500.0_f

#define StepSize min(1.0_f / (VolumePrecision * Density), SceneRadius / 2.0_f)

		auto randomFloat = writer.implementFunction<Float>(
		    "randomFloat",
		    [&](Float& seed) {
			    LocaleFloat(writer, res) = fract(sin(seed) * 43758.5453_f);
			    seed = seed + 1.0_f;
			    writer.returnStmt(res);
		    },
		    InOutFloat{ writer, "seed" });

		auto rotationMatrix = writer.implementFunction<Mat3>(
		    "rotationMatrix",
		    [&](const Vec3& rotEuler) {
			    LocaleFloat(writer, c) = cos(rotEuler.x());
			    LocaleFloat(writer, s) = sin(rotEuler.x());
			    LocaleMat3(writer, rx) =
			        mat3(vec3(1.0_f, 0.0_f, 0.0_f), vec3(0.0_f, c, -s),
			             vec3(0.0_f, s, c));
			    c = cos(rotEuler.y());
			    s = sin(rotEuler.y());
			    LocaleMat3(writer, ry) =
			        mat3(vec3(c, 0.0_f, -s), vec3(0.0_f, 1.0_f, 0.0_f),
			             vec3(s, 0.0_f, c));
			    c = cos(rotEuler.z());
			    s = sin(rotEuler.z());
			    LocaleMat3(writer, rz) =
			        mat3(vec3(c, -s, 0.0_f), vec3(s, c, 0.0_f),
			             vec3(0.0_f, 0.0_f, 1.0_f));
			    writer.returnStmt(rz * rx * ry);
		    },
		    InVec3{ writer, "rotEuler" });

		auto maxV = writer.implementFunction<Float>(
		    "maxV",
		    [&](const Vec3& v) {
			    writer.returnStmt(writer.ternary(
			        v.x() > v.y(), writer.ternary(v.x() > v.z(), v.x(), v.z()),
			        writer.ternary(v.y() > v.z(), v.y(), v.z())));
		    },
		    InVec3{ writer, "v" });

		auto randomDir = writer.implementFunction<Vec3>(
		    "randomDir",
		    [&](Float& seed) {
			    writer.returnStmt(
			        vec3(1.0_f, 0.0_f, 0.0_f) *
			        rotationMatrix(vec3(randomFloat(seed) * 2.0_f * Pi, 0.0_f,
			                            randomFloat(seed) * 2.0_f * Pi)));
		    },
		    InOutFloat{ writer, "seed" });

		auto backgroundColor = writer.implementFunction<Vec3>(
		    "backgroundColor",
		    [&](const Vec3&) { writer.returnStmt(vec3(0.0_f)); },
		    InVec3{ writer, "unused" });

		auto distanceEstimation = writer.implementFunction<Float>(
		    "distanceEstimation",
		    [&](const Vec3& pos_arg, Vec3& volumeColor, Vec3& emissionColor) {
			    // Vec3 pos = writer.declLocale<Vec3>writer, ("pos_local");
			    LocaleVec3(writer, pos) = pos_arg;
			    LocaleVec3(writer, basePos) = vec3(0.0_f);
			    LocaleFloat(writer, scale) = 1.0_f;

			    // pos = pos_arg;
			    // Vec3 basePos = writer.declLocale<Vec3>writer, ("basePos");
			    //   basePos = vec3(0.0_f);
			    //   Float scale = 1.0_f;

			    //   pos /= scale;
			    //   pos += basePos;

			    volumeColor = vec3(0.0_f);
			    emissionColor = vec3(0.0_f);

			    pos.yz() = vec2(pos.z(), pos.y());

			    LocaleFloat(writer, r) = length(pos);
			    LocaleVec3(writer, z) = pos;
			    LocaleVec3(writer, c) = pos;
			    LocaleFloat(writer, dr) = 1.0_f;
			    LocaleFloat(writer, theta) = 0.0_f;
			    LocaleFloat(writer, phi) = 0.0_f;
			    LocaleVec3(writer, orbitTrap) = vec3(1.0_f);

			    FOR(writer, Int, i, 0_i, i < 8_i, ++i)
			    {
				    r = length(z);
				    IF(writer, r > SceneRadius) { writer.loopBreakStmt(); }
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

			    LocaleFloat(writer, dist) = 0.5_f * log(r) * r / dr * scale;

			    volumeColor = (1.0_f - orbitTrap) * 0.98_f;

			    emissionColor = vec3(
			        writer.ternary(orbitTrap.z() < 0.0001_f, 20.0_f, 0.0_f));

			    writer.returnStmt(dist);
		    },
		    InVec3{ writer, "pos_arg" }, OutVec3{ writer, "volumeColor" },
		    OutVec3{ writer, "emissionColor" });

		auto directLight = writer.implementFunction<Vec3>(
		    "directLight",
		    [&](const Vec3& pos_arg, Float& seed) {
			    LocaleVec3(writer, pos) = pos_arg;

			    LocaleVec3(writer, absorption) = vec3(1.0_f);
			    LocaleVec3(writer, volumeColor) = vec3(0.0_f);
			    LocaleVec3(writer, emissionColor) = vec3(0.0_f);

			    FOR(writer, Int, i, 0_i, i < MaxSteps, ++i)
			    {
				    LocaleFloat(writer, dist) =
				        distanceEstimation(pos, volumeColor, emissionColor);
				    pos -= LightDir * max(dist, StepSize);

				    IF(writer, dist < StepSize)
				    {
					    LocaleFloat(writer, abStep) =
					        StepSize * randomFloat(seed);
					    pos -= LightDir * (abStep - StepSize);
					    IF(writer, dist < 0.0_f)
					    {
						    LocaleFloat(writer, absorbance) =
						        exp(-Density * abStep);
						    absorption *= absorbance;
						    IF(writer,
						       maxV(absorption) < 1.0_f - MaxShadowAbso)
						    {
							    writer.loopBreakStmt();
						    }
						    FI;
					    }
					    FI;
				    }
				    FI;

				    IF(writer, length(pos) > SceneRadius)
				    {
					    writer.loopBreakStmt();
				    }
				    FI;
			    }
			    ROF;

			    writer.returnStmt(
			        LightColor *
			        max((absorption + MaxShadowAbso - 1.0_f) / MaxShadowAbso,
			            vec3(0.0_f)));
		    },
		    InVec3{ writer, "pos_arg" }, InOutFloat{ writer, "seed" });

		auto pathTrace = writer.implementFunction<Vec3>(
		    "pathTrace",
		    [&](const Vec3& rayPos_arg, const Vec3& rayDir_arg, Float& seed) {
			    LocaleVec3(writer, rayPos) = rayPos_arg;
			    LocaleVec3(writer, rayDir) = rayDir_arg;

			    rayPos += rayDir * max(length(rayPos) - SceneRadius, 0.0_f);

			    LocaleVec3(writer, outColor) = vec3(0.0_f);
			    LocaleVec3(writer, absorption) = vec3(1.0_f);

			    LocaleVec3(writer, volumeColor) = vec3(0.0_f);
			    LocaleVec3(writer, emissionColor) = vec3(0.0_f);

			    FOR(writer, Int, i, 0_i, i < MaxSteps, ++i)
			    {
				    LocaleFloat(writer, dist) =
				        distanceEstimation(rayPos, volumeColor, emissionColor);
				    rayPos += rayDir * max(dist, StepSize);
				    IF(writer, dist < StepSize && length(rayPos) < SceneRadius)
				    {
					    LocaleFloat(writer, abStep) =
					        StepSize * randomFloat(seed);
					    rayPos += rayDir * (abStep - StepSize);
					    IF(writer, dist < 0.0_f)
					    {
						    LocaleFloat(writer, absorbance) =
						        exp(-Density * abStep);
						    LocaleFloat(writer, transmittance) =
						        1.0_f - absorbance;

						    // surface glow for a nice additional effect
						    // if(dist > -.0001) outColor += absorption *
						    // vec3(.2, .2, .2);

						    // if(randomFloat() < ShadowRaysPerStep)
						    // emissionColor
						    // += 1.0/ShadowRaysPerStep * volumeColor *
						    // directLight(rayPos);

						    LocaleFloat(writer, i_f) = writer.cast<Float>(i);

						    IF(writer,
						       mod(i_f, Float(StepsSkipShadow)) == 0.0_f)
						    {
							    emissionColor += Float(StepsSkipShadow) *
							                     volumeColor *
							                     directLight(rayPos, seed);
						    }
						    FI;

						    outColor +=
						        absorption * transmittance * emissionColor;

						    IF(writer, maxV(absorption) < 1.0_f - MaxAbso)
						    {
							    writer.loopBreakStmt();
						    }
						    FI;

						    IF(writer, randomFloat(seed) > absorbance)
						    {
							    rayDir = randomDir(seed);
							    absorption *= volumeColor;
						    }
						    FI;
					    }
					    FI;
				    }
				    FI;

				    IF(writer, length(rayPos) > SceneRadius &&
				                   dot(rayDir, rayPos) > 0.0_f)
				    {
					    writer.returnStmt(outColor + backgroundColor(rayDir) *
					                                     absorption);
				    }
				    FI;
			    }
			    ROF;

			    writer.returnStmt(outColor);
		    },
		    InVec3{ writer, "rayPos_arg" }, InVec3{ writer, "rayDir_arg" },
		    InOutFloat{ writer, "seed" });

		// n-blade aperture
		auto sampleAperture = writer.implementFunction<Vec2>(
		    "sampleAperture",
		    [&](const Int& nbBlades, const Float& rotation, Float& seed) {
			    LocaleFloat(writer, alpha) =
			        2.0_f * Pi / writer.cast<Float>(nbBlades);
			    LocaleFloat(writer, side) = sin(alpha / 2.0_f);

			    LocaleInt(writer, blade) = writer.cast<Int>(
			        randomFloat(seed) * writer.cast<Float>(nbBlades));

			    LocaleVec2(writer, tri) =
			        vec2(randomFloat(seed), -randomFloat(seed));
			    IF(writer, tri.x() + tri.y() > 0.0_f)
			    {
				    tri = vec2(tri.x() - 1.0_f, -1.0_f - tri.y());
			    }
			    FI;
			    tri.x() *= side;
			    tri.y() *= sqrt(1.0_f - side * side);

			    LocaleFloat(writer, angle) =
			        rotation + writer.cast<Float>(blade) /
			                       writer.cast<Float>(nbBlades) * 2.0_f * Pi;

			    writer.returnStmt(
			        vec2(tri.x() * cos(angle) + tri.y() * sin(angle),
			             tri.y() * cos(angle) - tri.x() * sin(angle)));
		    },
		    InInt{ writer, "nbBlades" }, InFloat{ writer, "rotation" },
		    InOutFloat{ writer, "seed" });

		//// used to store values in the unused alpha channel of the buffer
		auto setVector = writer.implementFunction<Void>(
		    "setVector",
		    [&](const Int& index, const Vec4& v, const Vec2& fragCoord_arg,
		        Vec4& fragColor) {
			    LocaleVec2(writer, fragCoord) = fragCoord_arg;
			    fragCoord -= vec2(0.5_f);
			    IF(writer, fragCoord.y() == writer.cast<Float>(index))
			    {
				    IF(writer, fragCoord.x() == 0.0_f)
				    {
					    fragColor.a() = v.x();
				    }
				    FI;
				    IF(writer, fragCoord.x() == 1.0_f)
				    {
					    fragColor.a() = v.y();
				    }
				    FI;
				    IF(writer, fragCoord.x() == 2.0_f)
				    {
					    fragColor.a() = v.z();
				    }
				    FI;
				    IF(writer, fragCoord.x() == 3.0_f)
				    {
					    fragColor.a() = v.a();
				    }
				    FI;
			    }
			    FI;
		    },
		    InInt{ writer, "index" }, InVec4{ writer, "v" },
		    InVec2{ writer, "fragCoord_arg" },
		    InOutVec4{ writer, "fragColor" });

		//*/

		auto kernelImage =
		    writer.declImage<RWFImg2DRgba32>("kernelImage", 0, 0);
		    //writer.declImage<ast::type::ImageFormat::eRgba32f,
		    //                 ast::type::ImageDim::e2D, false, false, false>(
		    //    "kernelImage", 0, 0);

		/*
		 auto getVector = writer.implementFunction<Vec4>(
		    "getVector",
		    [&](Int index) {
		        writer.returnStmt(
		            vec4(imageLoad(kernelImage, ivec2(0_i, index)).a(),
		                 imageLoad(kernelImage, ivec2(1_i, index)).a(),
		                 imageLoad(kernelImage, ivec2(2_i, index)).a(),
		                 imageLoad(kernelImage, ivec2(3_i, index)).a()));
		    },
		    InInt{ writer, "index" });
		 //*/

		// sdw::ImageT<ast::type::ImageFormat::eRgba32f,
		// ast::type::ImageDim::e2D, false, false, false>

		// writer.implementFunction<Void>("main", [&]() {
		writer.implementMain([&]() {
			//*

			LocaleFloat(writer, x) =
			    writer.cast<Float>(in.globalInvocationID.x());
			LocaleFloat(writer, y) =
			    writer.cast<Float>(in.globalInvocationID.y());

			LocaleFloat(writer, samples) =
			    samplesPC.getMember<Float>("samples");

			LocaleVec2(writer, xy) =
			    vec2(x, y) * vec2(samples, samples * samples);

			LocaleFloat(writer, seed) =
			    // cos(in.globalInvocationID.x()) +
			    // sin(in.globalInvocationID().y());
			    cos(x) + sin(y);

			// LocaleFloat(writer, width) = 1280.0_f * 4.0_f;
			// LocaleFloat(writer, height) = 720.0_f * 4.0_f;

			// outColor = vec4(0.0_f, 0.0_f, 0.0_f, 0.0_f);
			// outColor = in.fragCoord / vec4(1280.0_f, 720.0_f, 1.0_f, 1.0_f);
			// LocaleVec4(writer, uv) = in.fragCoord / vec4(1280.0_f,
			// 720.0_f, 1.0_f, 1.0_f);
			// LocaleVec2(writer, uv) = (in.fragCoord.xy() / vec2(1280.0_f,
			// 720.0_f) -
			//                  vec2(0.5_f));  // / vec2(720.0_f);
			LocaleVec2(writer, uv) =
			    (xy +
			     vec2(randomFloat(seed) / Width, randomFloat(seed) / Height) -
			     vec2(Width, Height) / 2.0_f) /
			    vec2(Height);

			LocaleIVec2(writer, iuv);
			iuv.x() = writer.cast<Int>(uv.x());
			iuv.y() = writer.cast<Int>(uv.y());

			// LocaleVec2(writer, uv) = (in.fragCoord.xy() - vec2(1280.0_f,
			// 720.0_f) / 2.0_f) / vec2(720.0_f);

			// LocaleVec2(writer, uv) = (in.fragCoord + vec2(randomFloat(),
			// randomFloat()) - iResolution.xy / 2.0) / iResolution.y;

			// float samples = texelFetch(iChannel0, ivec2(0, 0), 0).a;
			// if (iFrame > 0)
			// CamRot = getVector(1).xyz;
			// vec4 prevMouse = getVector(2);

			// fragColor = texelFetch(iChannel0, ivec2(fragCoord), 0);

			// bool mouseDragged =
			// iMouse.z >= 0.0 && prevMouse.z >= 0.0 && iMouse != prevMouse;

			// if (mouseDragged)
			// CamRot.yx += (prevMouse.xy - iMouse.xy) / iResolution.y * 2.0;

			// if (iFrame == 0 || mouseDragged)
			//{
			//    fragColor = vec4(0.0);
			//    samples = 0.0;
			//}

			// setVector(1, vec4(CamRot, 0), fragCoord, fragColor);
			// setVector(2, iMouse, fragCoord, fragColor);

			//   IF (writer, in.fragCoord - vec2(0.5_f) == vec2(0.0_f))
			//{
			//	fragColor.a = samples + 1.0;
			//}
			// FI

			LocaleVec3(writer, focalPoint) =
			    vec3(uv * CamFocalDistance / CamFocalLength, CamFocalDistance);
			LocaleVec3(writer, aperture) =
			    CamAperture * vec3(sampleAperture(6_i, 0.0_f, seed), 0.0_f);
			LocaleVec3(writer, rayDir) = normalize(focalPoint - aperture);

			LocaleVec3(writer, CamPos) = vec3(0.0_f, 0.0_f, -15.0_f);

			LocaleMat3(writer, CamMatrix) = rotationMatrix(CamRot);
			CamPos = CamMatrix * CamPos;
			rayDir = CamMatrix * rayDir;
			aperture = CamMatrix * aperture;

			// outColorHDR.rgb() =
			//    pathTrace(vec3(0.0_f, 0.0_f, 0.0_f) + CamPos + aperture,
			//              rayDir, seed) /
			//    15.0_f;
			// outColorHDR.a() = samples + 1.0;
			//*/

			// clang-format off
			LocaleVec3(writer, previousColor) = imageLoad(kernelImage, iuv).rgb();
			LocaleVec3(writer, newColor) = pathTrace(CamPos + aperture, rayDir, seed);
			LocaleVec3(writer, mixer) = vec3(1.0_f / (samples + 1.0_f));
			LocaleVec3(writer, mixedColor) = mix(previousColor, newColor, mixer);
			LocaleVec4(writer, mixedColor4) = vec4(mixedColor, samples + 1.0_f);
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
	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSize.descriptorCount = 1;

	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.maxSets = 1;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;

	m_computePool = vk.create(poolInfo);
	if (!m_computePool)
	{
		std::cerr << "error: failed to create compute descriptor pool"
		          << std::endl;
		abort();
	}
#pragma endregion compute descriptor pool

#pragma region compute descriptor set layout
	VkDescriptorSetLayoutBinding layoutBinding;
	layoutBinding.binding = 0;
	layoutBinding.descriptorCount = 1;
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	vk::DescriptorSetLayoutCreateInfo setLayoutInfo;
	setLayoutInfo.bindingCount = 1;
	setLayoutInfo.pBindings = &layoutBinding;

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
	cb.pushConstants(m_computePipelineLayout.get(),
	                 VK_SHADER_STAGE_COMPUTE_BIT, 0, &samples);
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
