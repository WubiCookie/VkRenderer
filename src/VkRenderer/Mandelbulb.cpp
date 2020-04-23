#define VMA_IMPLEMENTATION
//#include "vk_mem_alloc.h"

#include "MandelBulb.hpp"

#include "CompilerSpirV/compileSpirV.hpp"
#include "ShaderWriter/Intrinsics/Intrinsics.hpp"
#include "ShaderWriter/Source.hpp"

#include <array>
#include <iostream>
#include <random>
#include <stdexcept>

constexpr size_t POINT_COUNT = 100000;

namespace cdm
{
Mandelbulb::Mandelbulb(RenderWindow& renderWindow) : rw(renderWindow)
{
	uint32_t width = 1280;
	uint32_t height = 720;
	float widthf = 1280.0f;
	float heightf = 720.0f;

	auto& vk = rw.get().device();

#pragma region renderPass
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = VkFormat::VK_FORMAT_B8G8R8A8_UNORM;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
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
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vk.create(renderPassInfo, m_renderPass.get()) != VK_SUCCESS)
	{
		throw std::runtime_error("error: failed to create render pass");
	}
#pragma endregion renderPass

#pragma region shaders
	{
		using namespace sdw;

		VertexWriter writer;

		auto inPosition = writer.declInput<Vec2>("inPosition", 0);
		auto outColor = writer.declOutput<Vec4>("fragColor", 0u);

		auto out = writer.getOut();

		writer.implementFunction<void>("main", [&]() {
			out.vtx.position = vec4(inPosition, 0.0_f, 1.0_f);
			outColor = vec4(inPosition / 2.0_f + 0.5_f, 0.0_f, 1.0_f);
		});

		std::vector<uint32_t> bytecode =
		    spirv::serialiseSpirv(writer.getShader());

		vk::ShaderModuleCreateInfo createInfo;
		createInfo.codeSize = bytecode.size() * sizeof(*bytecode.data());
		createInfo.pCode = bytecode.data();

		VkShaderModule shaderModule = nullptr;
		if (vk.create(createInfo, shaderModule) != VK_SUCCESS)
		{
			std::cerr << "error: failed to create vertex shader module"
			          << std::endl;
			abort();
		}

		vk.destroy(m_vertexModule.get());
		m_vertexModule = shaderModule;
	}

	{
		using namespace sdw;

		FragmentWriter writer;

		auto in = writer.getIn();

		auto inPosition = writer.declInput<Vec4>("inPosition", 0u);
		auto outColor = writer.declOutput<Vec4>("outColor", 0u);

		//*

#define Pi 3.14159265359_f

#define VolumePrecision 0.35_f
#define SceneRadius 1.5_f
#define StepsSkipShadow 4.0_f
#define MaxSteps 500_i
#define MaxAbso 0.7_f
#define MaxShadowAbso 0.7_f

		auto CamPos =
		    writer.declConstant<Vec3>("CamPos", vec3(0.0_f, 0.0_f, -15.0_f));
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

		Float StepSize = 0.0_f;

		Float seed = 0.0_f;

		auto randomFloat =
		    writer.implementFunction<Float>("randomFloat", [&]() {
			    Float res = fract(sin(seed) * 43758.5453_f);
			    seed = seed + 1.0_f;  /// TODO: missing post increment on Float
			    writer.returnStmt(res);
		    });

		auto rotationMatrix = writer.implementFunction<Mat3>(
		    "rotationMatrix",
		    [&](const Vec3& rotEuler) {
			    Float c = cos(rotEuler.x());
			    Float s = sin(rotEuler.x());
			    Mat3 rx = mat3(1.0_f, 0.0_f, 0.0_f, 0.0_f, c, -s, 0.0_f, s, c);
			    c = cos(rotEuler.y());
			    s = sin(rotEuler.y());
			    Mat3 ry = mat3(c, 0.0_f, -s, 0.0_f, 1.0_f, 0.0_f, s, 0.0_f, c);
			    c = cos(rotEuler.z());
			    s = sin(rotEuler.z());
			    Mat3 rz = mat3(c, -s, 0.0_f, s, c, 0.0_f, 0.0_f, 0.0_f, 1.0_f);
			    writer.returnStmt(rz * rx * ry);
		    },
		    InVec3{ writer, "rotEuler" });

		auto maxV = writer.implementFunction<Float>(
		    "maxV",
		    [&](const Vec3& v) {
			    writer.returnStmt(v.x() > v.y()
			                          ? v.x() > v.z() ? v.x() : v.z()
			                          : v.y() > v.z() ? v.y() : v.z());
		    },
		    InVec3{ writer, "v" });

		auto randomDir = writer.implementFunction<Vec3>("randomDir", [&]() {
			writer.returnStmt(
			    vec3(1.0_f, 0.0_f, 0.0_f) *
			    rotationMatrix(vec3(randomFloat() * 2.0_f * Pi, 0.0_f,
			                        randomFloat() * 2.0_f * Pi)));
		});

		auto backgroundColor = writer.implementFunction<Vec3>(
		    "backgroundColor",
		    [&](const Vec3&) { writer.returnStmt(vec3(0.0_f)); },
		    InVec3{ writer, "unused" });

		auto distanceEstimation = writer.implementFunction<Float>(
		    "distanceEstimation",
		    [&](const Vec3& pos_arg, Vec3& volumeColor, Vec3& emissionColor) {
			    Vec3 pos = pos_arg;
			    Vec3 basePos = vec3(0.0_f);
			    Float scale = 1.0_f;

			    pos /= scale;
			    pos += basePos;

			    volumeColor = vec3(0.0_f);
			    emissionColor = vec3(0.0_f);

			    pos.yz() = vec2(pos.z(), pos.y());

			    Float r = length(pos);
			    Vec3 z = pos;
			    Vec3 c = pos;
			    Float dr = 1.0_f, theta = 0.0_f, phi = 0.0_f;
			    Vec3 orbitTrap = vec3(1.0_f);

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

			    Float dist = 0.5_f * log(r) * r / dr * scale;

			    volumeColor = (1.0_f - orbitTrap) * 0.98_f;

			    emissionColor = vec3(
			        writer.ternary(orbitTrap.z() < 0.0001_f, 20.0_f, 0.0_f));

			    writer.returnStmt(dist);
		    },
		    InVec3{ writer, "pos" }, OutVec3{ writer, "volumeColor" },
		    OutVec3{ writer, "emissionColor" });

		auto directLight = writer.implementFunction<Vec3>(
		    "directLight",
		    [&](const Vec3& pos_arg) {
			    Vec3 pos = pos_arg;

			    Vec3 absorption = vec3(1.0_f);
			    Vec3 volumeColor = vec3(0.0_f);
			    Vec3 emissionColor = vec3(0.0_f);

			    FOR(writer, Int, i, 0_i, i < MaxSteps, ++i)
			    {
				    Float dist =
				        distanceEstimation(pos, volumeColor, emissionColor);
				    pos -= LightDir * max(dist, StepSize);

				    IF(writer, dist < StepSize)
				    {
					    Float abStep = StepSize * randomFloat();
					    pos -= LightDir * (abStep - StepSize);
					    IF(writer, dist < 0.0_f)
					    {
						    Float absorbance = exp(-Density * abStep);
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
		    InVec3{ writer, "pos" });

		auto pathTrace = writer.implementFunction<Vec3>(
		    "pathTrace",
		    [&](const Vec3& rayPos_arg, const Vec3& rayDir_arg) {
			    Vec3 rayPos = rayPos_arg;
			    Vec3 rayDir = rayDir_arg;

			    rayPos += rayDir * max(length(rayPos) - SceneRadius, 0.0_f);

			    Vec3 outColor = vec3(0.0_f);
			    Vec3 absorption = vec3(1.0_f);

			    Vec3 volumeColor = vec3(0.0_f);
			    Vec3 emissionColor = vec3(0.0_f);

			    FOR(writer, Int, i, 0_i, i < MaxSteps, ++i)
			    {
				    Float dist =
				        distanceEstimation(rayPos, volumeColor, emissionColor);
				    rayPos += rayDir * max(dist, StepSize);
				    IF(writer, dist < StepSize && length(rayPos) < SceneRadius)
				    {
					    Float abStep = StepSize * randomFloat();
					    rayPos += rayDir * (abStep - StepSize);
					    IF(writer, dist < 0.0_f)
					    {
						    Float absorbance = exp(-Density * abStep);
						    Float transmittance = 1.0_f - absorbance;

						    // surface glow for a nice additional effect
						    // if(dist > -.0001) outColor += absorption *
						    // vec3(.2, .2, .2);

						    // if(randomFloat() < ShadowRaysPerStep)
						    // emissionColor
						    // += 1.0/ShadowRaysPerStep * volumeColor *
						    // directLight(rayPos);

						    Float i_f = writer.cast<Float>(i);

						    IF(writer,
						       mod(i_f, Float(StepsSkipShadow)) == 0.0_f)
						    {
							    emissionColor += Float(StepsSkipShadow) *
							                     volumeColor *
							                     directLight(rayPos);
						    }
						    FI;

						    outColor +=
						        absorption * transmittance * emissionColor;

						    IF(writer, maxV(absorption) < 1.0_f - MaxAbso)
						    {
							    writer.loopBreakStmt();
						    }
						    FI;

						    IF(writer, randomFloat() > absorbance)
						    {
							    rayDir = randomDir();
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
		    InVec3{ writer, "rayPos" }, InVec3{ writer, "rayDir" });

		// n-blade aperture
		auto sampleAperture = writer.implementFunction<Vec2>(
		    "sampleAperture",
		    [&](const Int& nbBlades, const Float& rotation) {
			    Float alpha = 2.0_f * Pi / writer.cast<Float>(nbBlades);
			    Float side = sin(alpha / 2.0_f);

			    Int blade = writer.cast<Int>(randomFloat() *
			                                 writer.cast<Float>(nbBlades));

			    Vec2 tri = vec2(randomFloat(), -randomFloat());
			    IF(writer, tri.x() + tri.y() > 0.0_f)
			    {
				    tri = vec2(tri.x() - 1.0_f, -1.0_f - tri.y());
			    }
			    FI;
			    tri.x() *= side;
			    tri.y() *= sqrt(1.0_f - side * side);

			    Float angle = rotation + writer.cast<Float>(blade) /
			                                 writer.cast<Float>(nbBlades) *
			                                 2.0_f * Pi;

			    writer.returnStmt(
			        vec2(tri.x() * cos(angle) + tri.y() * sin(angle),
			             tri.y() * cos(angle) - tri.x() * sin(angle)));
		    },
		    InInt{ writer, "nbBlades" }, InFloat{ writer, "rotation" });

		// used to store values in the unused alpha channel of the buffer
		auto setVector = writer.implementFunction<Void>(
		    "setVector",
		    [&](const Int& index, const Vec4& v, const Vec2& fragCoord_arg,
		        Vec4& fragColor) {
			    Vec2 fragCoord = fragCoord_arg;
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
		    InVec2{ writer, "fragCoord" }, InOutVec4{ writer, "fragColor" });

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

		writer.implementFunction<Void>("main", [&]() {
			outColor = in.fragCoord / vec4(1280.0_f, 720.0_f, 1.0_f, 1.0_f);
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

		VkShaderModule shaderModule = nullptr;
		if (vk.create(createInfo, shaderModule) != VK_SUCCESS)
		{
			std::cerr << "error: failed to create fragment shader module"
			          << std::endl;
			abort();
		}

		vk.destroy(m_fragmentModule.get());
		m_fragmentModule = shaderModule;
	}
#pragma endregion shaders

#pragma region pipelineLayout
	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vk.create(pipelineLayoutInfo, m_pipelineLayout.get()) != VK_SUCCESS)
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
	// VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; colorBlendAttachment.colorBlendOp =
	// VK_BLEND_OP_ADD; colorBlendAttachment.srcAlphaBlendFactor =
	// VK_BLEND_FACTOR_ONE; colorBlendAttachment.dstAlphaBlendFactor =
	// VK_BLEND_FACTOR_ZERO; colorBlendAttachment.alphaBlendOp =
	// VK_BLEND_OP_ADD;

	vk::PipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
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

	VkResult res = vk.create(pipelineInfo, m_pipeline.get());
	if (res != VK_SUCCESS)
	{
		std::cerr << "error: failed to create graphics pipeline" << std::endl;
		abort();
	}
#pragma endregion pipeline

#pragma region allocator
	VmaVulkanFunctions vulkanFunction = {};
	vulkanFunction.vkGetPhysicalDeviceProperties =
	    vk.GetPhysicalDeviceProperties;
	vulkanFunction.vkGetPhysicalDeviceMemoryProperties =
	    vk.GetPhysicalDeviceMemoryProperties;
	vulkanFunction.vkAllocateMemory = vk.AllocateMemory;
	vulkanFunction.vkFreeMemory = vk.FreeMemory;
	vulkanFunction.vkMapMemory = vk.MapMemory;
	vulkanFunction.vkUnmapMemory = vk.UnmapMemory;
	vulkanFunction.vkFlushMappedMemoryRanges = vk.FlushMappedMemoryRanges;
	vulkanFunction.vkInvalidateMappedMemoryRanges =
	    vk.InvalidateMappedMemoryRanges;
	vulkanFunction.vkBindBufferMemory = vk.BindBufferMemory;
	vulkanFunction.vkBindImageMemory = vk.BindImageMemory;
	vulkanFunction.vkGetBufferMemoryRequirements =
	    vk.GetBufferMemoryRequirements;
	vulkanFunction.vkGetImageMemoryRequirements =
	    vk.GetImageMemoryRequirements;
	vulkanFunction.vkCreateBuffer = vk.CreateBuffer;
	vulkanFunction.vkDestroyBuffer = vk.DestroyBuffer;
	vulkanFunction.vkCreateImage = vk.CreateImage;
	vulkanFunction.vkDestroyImage = vk.DestroyImage;
	vulkanFunction.vkCmdCopyBuffer = vk.CmdCopyBuffer;
	vulkanFunction.vkGetBufferMemoryRequirements2KHR =
	    vk.GetBufferMemoryRequirements2KHR;
	vulkanFunction.vkGetImageMemoryRequirements2KHR =
	    vk.GetImageMemoryRequirements2KHR;
	vulkanFunction.vkBindBufferMemory2KHR = vk.BindBufferMemory2KHR;
	vulkanFunction.vkBindImageMemory2KHR = vk.BindImageMemory2KHR;
	vulkanFunction.vkGetPhysicalDeviceMemoryProperties2KHR =
	    vk.GetPhysicalDeviceMemoryProperties2KHR;

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
	allocatorInfo.instance = vk.instance();
	allocatorInfo.physicalDevice = vk.physicalDevice();
	allocatorInfo.device = vk.vkDevice();
	allocatorInfo.pVulkanFunctions = &vulkanFunction;

	vmaCreateAllocator(&allocatorInfo, &m_allocator.get());
#pragma endregion allocator

#pragma region vertexBuffer
	using Vertex = std::array<float, 2>;
	std::array<Vertex, POINT_COUNT> vertices{
		// Vertex{ -0.5f, -0.5f },
		// Vertex{  0.5f, -0.5f },
		// Vertex{ -0.5f,  0.5f },
		// Vertex{  0.5f,  0.5f },
		// Vertex{  0.0f,  0.0f },
	};

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

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
	vbAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	vbAllocCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

	VmaAllocationInfo stagingVertexBufferAllocInfo = {};
	vmaCreateBuffer(m_allocator.get(), &vbInfo, &vbAllocCreateInfo,
	                &m_vertexBuffer.get(), &m_vertexBufferAllocation.get(),
	                &stagingVertexBufferAllocInfo);

	std::copy(vertices.begin(), vertices.end(),
	          static_cast<Vertex*>(stagingVertexBufferAllocInfo.pMappedData));
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
	imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
	imageAllocCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

	vmaCreateImage(m_allocator.get(), &imageInfo, &imageAllocCreateInfo,
	               &m_outputImage.get(), &m_outputImageAllocation.get(),
	               nullptr);

	CommandBuffer transitionCB(vk, rw.get().commandPool());

	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	transitionCB.begin(beginInfo);
	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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

	if (vk.create(viewInfo, m_outputImageView.get()) != VK_SUCCESS)
	{
		std::cerr << "error: failed to create image views" << std::endl;
		abort();
	}
#pragma endregion outputImage

#pragma region framebuffer
	vk::FramebufferCreateInfo framebufferInfo;
	framebufferInfo.renderPass = m_renderPass.get();
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.pAttachments = &m_outputImageView.get();
	framebufferInfo.width = width;
	framebufferInfo.height = height;
	framebufferInfo.layers = 1;

	if (vk.create(framebufferInfo, m_framebuffer.get()) != VK_SUCCESS)
	{
		std::cerr << "error: failed to create framebuffer" << std::endl;
		abort();
	}
#pragma endregion framebuffer
}

Mandelbulb::~Mandelbulb()
{
	auto& vk = rw.get().device();

	vk.destroy(m_outputImageView.get());
	vmaDestroyImage(m_allocator.get(), m_outputImage.get(),
	                m_outputImageAllocation.get());
	vmaDestroyBuffer(m_allocator.get(), m_vertexBuffer.get(),
	                 m_vertexBufferAllocation.get());
	vmaDestroyAllocator(m_allocator.get());
	vk.destroy(m_pipeline.get());
	vk.destroy(m_pipelineLayout.get());
	vk.destroy(m_vertexModule.get());
	vk.destroy(m_fragmentModule.get());
	vk.destroy(m_framebuffer.get());
	vk.destroy(m_renderPass.get());
}

void Mandelbulb::render(CommandBuffer& cb)
{
	uint32_t width = 1280;
	uint32_t height = 720;

	VkClearValue clearValues = {};

	vk::RenderPassBeginInfo rpInfo;
	rpInfo.framebuffer = m_framebuffer.get();
	rpInfo.renderPass = m_renderPass.get();
	rpInfo.renderArea.extent.width = width;
	rpInfo.renderArea.extent.height = height;
	rpInfo.clearValueCount = 1;
	rpInfo.pClearValues = &clearValues;

	vk::SubpassBeginInfo subpassBeginInfo;
	subpassBeginInfo.contents = VK_SUBPASS_CONTENTS_INLINE;

	vk::SubpassEndInfo subpassEndInfo;

	cb.beginRenderPass2(rpInfo, subpassBeginInfo);

	cb.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.get());
	cb.bindVertexBuffer(m_vertexBuffer.get());
	cb.draw(POINT_COUNT);

	cb.endRenderPass2(subpassEndInfo);
}
}  // namespace cdm
