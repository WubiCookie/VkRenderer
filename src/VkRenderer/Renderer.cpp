#include "Renderer.hpp"
#include "CommandBuffer.hpp"
#include "Framebuffer.hpp"
#include "RenderWindow.hpp"

namespace cdm
{
Renderer::Renderer(RenderWindow& renderWindow)
    : rw(renderWindow),
      m_defaultRenderPass(renderWindow.device(),
                          renderWindow.swapchainImageFormat()),
      m_defaultMaterial(renderWindow, m_defaultRenderPass)
{
	const auto& vk = renderWindow.device();

	/*
	{
	    using namespace sdw;

	    VertexWriter writer;

	    // auto colors = writer.declConstantArray<Vec4>(
	    //    "colors", std::vector<Vec4>{ vec4(0.0_f, 0.0f, 0.0f, 1.0f),
	    //                                 vec4(1.0_f, 1.0f, 1.0f, 1.0f),
	    //                                 vec4(0.0_f, 0.0f, 0.0f, 1.0f),
	    //                                 vec4(0.0_f, 0.0f, 0.0f, 1.0f),
	    //                                 vec4(1.0_f, 1.0f, 1.0f, 1.0f),
	    //                                 vec4(1.0_f, 1.0f, 1.0f, 1.0f) });

	    // auto positions = writer.declLocaleArray(
	    //    "positions", 6u,
	    //    std::vector<Vec4>{ vec4(-1.0_f, -1.0f, 0.0f, 1.0f),
	    //                       vec4(1.0_f, -1.0f, 0.0f, 1.0f),
	    //                       vec4(-1.0_f, 1.0f, 0.0f, 1.0f),
	    //                       vec4(-1.0_f, 1.0f, 0.0f, 1.0f),
	    //                       vec4(1.0_f, -1.0f, 0.0f, 1.0f),
	    //                       vec4(1.0_f, 1.0f, 0.0f, 1.0f) });

	    // Shader inputs
	    auto in = writer.getIn();

	    // Shader outputs
	    auto outColor = writer.declOutput<Vec4>("outColor", 0u);
	    auto out = writer.getOut();

	    writer.implementFunction<void>("main", [&]() {
	        auto positions = writer.declLocaleArray<Vec4>(
	            "positions", 6,
	            std::vector<Vec4>{ vec4(-1.0_f, -1.0f, 0.0f, 1.0f),
	                               vec4(1.0_f, -1.0f, 0.0f, 1.0f),
	                               vec4(-1.0_f, 1.0f, 0.0f, 1.0f),
	                               vec4(-1.0_f, 1.0f, 0.0f, 1.0f),
	                               vec4(1.0_f, -1.0f, 0.0f, 1.0f),
	                               vec4(1.0_f, 1.0f, 0.0f, 1.0f) });

	        outColor = vec4(1.0_f);  // colors[in.gl_VertexID];
	        // out.gl_out.gl_Position = positions[in.gl_VertexID];
	        //// outColor = vec4(1.0_f, 0.0_f, 0.0_f, 1.0_f);
	        //// out.gl_out.gl_Position = vec4(0.0_f, 0.0_f, 0.0_f, 1.0_f);

	        // outColor = colors[in.gl_VertexIndex];
	        out.gl_out.gl_Position = positions[in.gl_VertexID];
	    });

	    // std::cout << spirv::writeSpirv(writer.getShader()) << std::endl;

	    mtl.setVertexShaderBytecode(spirv::serialiseSpirv(writer.getShader()));
	}

	{
	    using namespace sdw;

	    FragmentWriter writer;

	    // Shader inputs
	    auto color = writer.declInput<Vec4>("color", 0u);

	    // Shader outputs
	    auto outColor = writer.declOutput<Vec4>("fragColor", 0u);

	    writer.implementFunction<void>("main", [&]() { outColor = color; });

	    // std::cout << spirv::writeSpirv(writer.getShader()) << std::endl;

	    mtl.setFragmentShaderBytecode(
	        spirv::serialiseSpirv(writer.getShader()));
	}
	//*/

	{
		m_defaultMaterial.setVertexShaderGLSL(R"(
#version 450

layout(location = 0) out vec3 fragColor;

vec2 positions[9] = vec2[](
	vec2(-1.0, -1.0), vec2( 0.0, -1.0), vec2( 1.0, -1.0),
	vec2(-1.0,  0.0), vec2( 0.0,  0.0), vec2( 1.0,  0.0),
	vec2(-1.0,  1.0), vec2( 0.0,  1.0), vec2( 1.0,  1.0)
);

#define miR 0.0
#define maR 1.0
#define miB 0.0
#define maB 1.0

vec3 colors[9] = vec3[](
	vec3(maR, 0.0, 0.0), vec3(1.0, 1.0, 1.0), vec3(0.0, 0.0, maB),
	vec3(maR, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, maB),
	vec3(miR, 0.0, 0.0), vec3(0.0, 0.0, 0.0), vec3(0.0, 0.0, miB)
);

// 0 1 2
// 3 4 5
// 6 7 8

int indices[12] = int[](
	// 0,1,3, 3,1,4, 1,2,5, 1,5,4,

	// 3,4,7, 3,7,6, 4,5,7, 7,5,8

	0,1,6, 6,1,7, 1,2,7, 7,2,8
);

void main()
{
	gl_Position = vec4(positions[indices[gl_VertexIndex]], 0.0, 1.0);
	fragColor = colors[indices[gl_VertexIndex]];
}

)");

		m_defaultMaterial.setFragmentShaderGLSL(R"(
#version 450

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(fragColor, 1.0);
}

)");
	}

	m_defaultMaterial.buildPipeline();

	m_commandBuffers.reserve(renderWindow.swapchainImageViews().size());
	m_framebuffers.reserve(renderWindow.swapchainImageViews().size());
	m_renderFinishedSemaphores.reserve(
	    renderWindow.swapchainImageViews().size());

	for (ImageView& view : renderWindow.swapchainImageViews())
	{
		// m_framebuffers.push_back(
		//    Framebuffer(rw, m_defaultRenderPass, { view }));
		// m_commandBuffers.push_back(
		// CommandBuffer(vk, renderWindow.commandPool()));

		VkSemaphore renderFinishedSemaphore;
		vk::SemaphoreCreateInfo semaphoreInfo;
		if (vk.create(semaphoreInfo, renderFinishedSemaphore) != VK_SUCCESS)
		{
			throw std::runtime_error("error: failed to create semaphores");
		}

		m_renderFinishedSemaphores.push_back(renderFinishedSemaphore);
	}
}

void Renderer::render()
{
	const auto& vk = rw.get().device();

	if (m_framebuffers.empty())
	{
		for (ImageView& view : rw.get().swapchainImageViews())
			m_framebuffers.push_back(
			    Framebuffer(rw, m_defaultRenderPass, { view }));
	}
	if (m_commandBuffers.empty())
	{
		for (size_t i = 0; i < rw.get().swapchainImageViews().size(); i++)
			m_commandBuffers.push_back(
			    CommandBuffer(vk, rw.get().commandPool()));
		for (size_t i = 0; i < m_commandBuffers.size(); i++)
		{
			auto& cb = m_commandBuffers[i];
			auto& fb = m_framebuffers[i];

			vk::CommandBufferBeginInfo beginInfo;
			beginInfo.flags = 0;                   // Optional
			beginInfo.pInheritanceInfo = nullptr;  // Optional

			if (cb.begin(beginInfo) != VK_SUCCESS)
			{
				throw std::runtime_error(
				    "error: failed to begin recording command buffer");
			}

			vk::RenderPassBeginInfo renderPassInfo;
			renderPassInfo.renderPass = m_defaultRenderPass;
			renderPassInfo.framebuffer = fb;
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = rw.get().swapchainExtent();

			VkClearValue clearColor = { 0x27 / 255.0f, 0x28 / 255.0f,
				                        0x22 / 255.0f, 1.0f };
			// VkClearValue clearColor = {0};
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			cb.beginRenderPass(renderPassInfo);

			m_defaultMaterial.bind(cb);
			// cb.draw(12);
			cb.draw(6);

			cb.endRenderPass();

			if (cb.end() != VK_SUCCESS)
			{
				throw std::runtime_error(
				    "error: failed to record command buffer");
			}
		}
	}

	rw.get().prerender();

	vk::SubmitInfo submitInfo;

	VkSemaphore waitSemaphores[] = {
		rw.get().currentImageAvailableSemaphore()
	};
	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	auto cb = m_commandBuffers[rw.get().imageIndex()].commandBuffer();
	submitInfo.pCommandBuffers = &cb;
	VkSemaphore signalSemaphores[] = {
		m_renderFinishedSemaphores[rw.get().currentFrame()]
	};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	VkFence inFlightFence = rw.get().currentInFlightFences();

	vk.ResetFences(vk.vkDevice(), 1, &inFlightFence);

	if (vk.QueueSubmit(vk.graphicsQueue(), 1, &submitInfo, inFlightFence) !=
	    VK_SUCCESS)
	{
		throw std::runtime_error(
		    "error: failed to submit draw command buffer");
	}

	if (rw.get().present() == false)
	{
		vk.wait();
		m_framebuffers.clear();
		m_commandBuffers.clear();
		// m_framebuffers.clear();
		//	for (ImageView& view : renderWindow.swapchainImageViews())
		//{
		//	m_framebuffers.push_back(
		//	    Framebuffer(rw, m_defaultRenderPass, { view }));
		//	m_commandBuffers.push_back(
		//	    CommandBuffer(vk, renderWindow.commandPool()));

		//	VkSemaphore renderFinishedSemaphore;
		//	vk::SemaphoreCreateInfo semaphoreInfo;
		//	if (vk.create(semaphoreInfo, renderFinishedSemaphore) !=
		// VK_SUCCESS)
		//	{
		//		throw std::runtime_error("error: failed to create semaphores");
		//	}

		//	m_renderFinishedSemaphores.push_back(renderFinishedSemaphore);
		//}
	}
}

std::vector<CommandBuffer>& Renderer::commandBuffers()
{
	return m_commandBuffers;
}

std::vector<Framebuffer>& Renderer::framebuffers() { return m_framebuffers; }

RenderPass& Renderer::defaultRenderPass() { return m_defaultRenderPass; }

Material& Renderer::defaultMaterial() { return m_defaultMaterial; }
}  // namespace cdm
