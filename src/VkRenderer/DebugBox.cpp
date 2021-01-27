#include "DebugBox.hpp"
#include "Camera.hpp"
#include "CommandBuffer.hpp"
#include "MyShaderWriter.hpp"
#include "PipelineFactory.hpp"

#include <iostream>

namespace cdm
{
struct PushConstantStruct
{
	matrix4 mvp;
	vector4 color;
};

DebugBox::DebugBox(const VulkanDevice& device, VkRenderPass rp,
                   const vk::Viewport& viewport, const vk::Rect2D& scissor)
    : m_vk(device)
{
	auto& vk = m_vk.get();

#pragma region pipeline layout
	VkPushConstantRange pcRange{};
	pcRange.size = sizeof(PushConstantStruct);
	pcRange.stageFlags =
	    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pcRange;

	m_pipelineLayout = vk.create(pipelineLayoutInfo);
	if (!m_pipelineLayout)
	{
		std::cerr << "error: failed to create pipeline layout" << std::endl;
		abort();
	}
#pragma endregion

	UniqueShaderModule vertexModule;
#pragma region vertexShader
	{
		using namespace sdw;

		cdm::VertexWriter writer;

		auto positions = writer.declConstantArray<Vec4>(
		    "positions", { vec4(0.0_f, 0.0_f, 0.0_f, 1.0_f),
		                   vec4(1.0_f, 0.0_f, 0.0_f, 1.0_f),
		                   vec4(0.0_f, 1.0_f, 0.0_f, 1.0_f),
		                   vec4(1.0_f, 1.0_f, 0.0_f, 1.0_f),

		                   vec4(0.0_f, 0.0_f, 1.0_f, 1.0_f),
		                   vec4(1.0_f, 0.0_f, 1.0_f, 1.0_f),
		                   vec4(0.0_f, 1.0_f, 1.0_f, 1.0_f),
		                   vec4(1.0_f, 1.0_f, 1.0_f, 1.0_f) });

		auto indices = writer.declConstantArray<UInt>(
		    "indices", { 0_u, 1_u, 0_u, 2_u, 3_u, 1_u, 3_u, 2_u,

		                 4_u, 5_u, 4_u, 6_u, 7_u, 5_u, 7_u, 6_u,

		                 0_u, 4_u, 1_u, 5_u, 2_u, 6_u, 3_u, 7_u });

		auto in = writer.getIn();
		auto out = writer.getOut();

		auto pcb = writer.declPushConstantsBuffer("pcb");
		pcb.declMember<Mat4>("mvp");
		pcb.declMember<Vec4>("color");
		pcb.end();

		writer.implementMain([&]() {
			out.vtx.position = pcb.getMember<Mat4>("mvp") *
			                   positions[indices[in.vertexIndex]];
		});

		vertexModule = writer.createShaderModule(vk);
		if (!vertexModule)
		{
			std::cerr << "error: failed to create vertex shader module"
			          << std::endl;
			abort();
		}
	}
#pragma endregion

	UniqueShaderModule fragmentModule;
#pragma region fragmentShader
	{
		using namespace sdw;
		cdm::FragmentWriter writer;

		auto in = writer.getIn();

		auto fragColor = writer.declOutput<Vec4>("fragColor", 0u);

		auto pcb = writer.declPushConstantsBuffer("pcb");
		pcb.declMember<Mat4>("mvp");
		pcb.declMember<Vec4>("color");
		pcb.end();

		writer.implementMain(
		    [&]() { fragColor = pcb.getMember<Vec4>("color"); });

		fragmentModule = writer.createShaderModule(vk);
		if (!fragmentModule)
		{
			std::cerr << "error: failed to create fragment shader module"
			          << std::endl;
			abort();
		}
	}
#pragma endregion

	GraphicsPipelineFactory f(vk);
#pragma region pipeline
	f.setRenderPass(rp);
	f.setLayout(m_pipelineLayout);
	f.setPolygonMode(VK_POLYGON_MODE_LINE);
	f.setTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
	f.addColorBlendAttachmentState({});
	f.setShaderModules(vertexModule, fragmentModule);
	f.addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
	f.addDynamicState(VK_DYNAMIC_STATE_SCISSOR);
	f.setViewport(viewport);
	f.setScissor(scissor);
	f.setDepthTestEnable(true);
	f.setDepthWriteEnable(true);

	m_pipeline = f.createPipeline();
	if (!m_pipeline)
	{
		std::cerr << "error: failed to create graphics pipeline" << std::endl;
		abort();
	}
#pragma endregion
}

void DebugBox::draw(CommandBuffer& cb, vector3 min, vector3 max, vector3 color,
                    const Camera& camera)
{
	cb.bindPipeline(m_pipeline);

	PushConstantStruct pcb;
	pcb.mvp = camera.viewproj() * matrix4::translation(min) *
	          matrix4::scale(max - min);
	pcb.mvp.transpose();
	pcb.color = vector4(color, 1.0f);

	cb.pushConstants(m_pipelineLayout,
	                 VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	                 0, &pcb);

	cb.draw(24);
}
}  // namespace cdm
