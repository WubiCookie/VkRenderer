#include "CommandBuffer.hpp"
#include "Framebuffer.hpp"
#include "Material.hpp"
#include "RenderPass.hpp"
#include "RenderWindow.hpp"

#include <fstream>
#include <iostream>
#include <vector>

#include "CompilerSpirV/compileSpirV.hpp"
#include "ShaderWriter/Intrinsics/Intrinsics.hpp"
#include "ShaderWriter/Source.hpp"

static std::vector<uint32_t> readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

	file.close();

	return buffer;
}

int main()
{
	using namespace cdm;

	RenderWindow rw(800, 600, true);

	auto& vk = rw.device();
	auto device = vk.vkDevice();

	RenderPass rp(vk, rw.swapchainImageFormat());

	Material mtl(rw, rp);

	/*
	{
	    using namespace sdw;

	    VertexWriter writer;

	    // Shader constants
	    auto positions = writer.declConstantArray<Vec4>(
	        "positions", std::vector<Vec4>{ vec4(-1.0_f, -1.0f, 0.0f, 1.0f),
	                                        vec4(1.0_f, -1.0f, 0.0f, 1.0f),
	                                        vec4(-1.0_f, 1.0f, 0.0f, 1.0f),
	                                        vec4(-1.0_f, 1.0f, 0.0f, 1.0f),
	                                        vec4(1.0_f, -1.0f, 0.0f, 1.0f),
	                                        vec4(1.0_f, 1.0f, 0.0f, 1.0f) });

	    //auto colors = writer.declConstantArray<Vec4>(
	    //    "colors", std::vector<Vec4>{ vec4(0.0_f, 0.0f, 0.0f, 1.0f),
	    //                                 vec4(1.0_f, 1.0f, 1.0f, 1.0f),
	    //                                 vec4(0.0_f, 0.0f, 0.0f, 1.0f),
	    //                                 vec4(0.0_f, 0.0f, 0.0f, 1.0f),
	    //                                 vec4(1.0_f, 1.0f, 1.0f, 1.0f),
	    //                                 vec4(1.0_f, 1.0f, 1.0f, 1.0f) });

	    // Shader inputs
	    auto in = writer.getIn();

	    // Shader outputs
	    auto outColor = writer.declOutput<Vec4>("outColor", 0u);
	    auto out = writer.getOut();

	    writer.implementFunction<void>("main", [&]() {
			outColor = vec4(1.0_f);// colors[in.gl_VertexID];
	        out.gl_out.gl_Position = positions[in.gl_VertexID];
	        // outColor = vec4(1.0_f, 0.0_f, 0.0_f, 1.0_f);
	        // out.gl_out.gl_Position = vec4(0.0_f, 0.0_f, 0.0_f, 1.0_f);
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

	mtl.setVertexShaderBytecode(readFile("vert.spv"));
	mtl.setFragmentShaderBytecode(readFile("frag.spv"));

	if (mtl.buildPipeline() == false)
		return 1;

	std::vector<Framebuffer> framebuffers;
	std::vector<CommandBuffer> commandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight(rw.swapchainImageViews().size(),
	                                    nullptr);

	for (ImageView& view : rw.swapchainImageViews())
	{
		framebuffers.push_back(Framebuffer(rw, rp, { view }));
		commandBuffers.push_back(CommandBuffer(vk, rw.commandPool()));

		VkSemaphore imageAvailableSemaphore;
		VkSemaphore renderFinishedSemaphore;
		VkFence inFlightFence;
		vk::SemaphoreCreateInfo semaphoreInfo;
		vk::FenceCreateInfo fenceInfo;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		if (vk.create(semaphoreInfo, imageAvailableSemaphore) != VK_SUCCESS ||
		    vk.create(semaphoreInfo, renderFinishedSemaphore) != VK_SUCCESS ||
		    vk.create(fenceInfo, inFlightFence) != VK_SUCCESS)
		{
			throw std::runtime_error(
			    "error: failed to create semaphores or fence");
		}

		imageAvailableSemaphores.push_back(imageAvailableSemaphore);
		renderFinishedSemaphores.push_back(renderFinishedSemaphore);
		inFlightFences.push_back(inFlightFence);
	}

	for (size_t i = 0; i < commandBuffers.size(); i++)
	{
		auto& cb = commandBuffers[i];
		auto& fb = framebuffers[i];

		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.flags = 0;                   // Optional
		beginInfo.pInheritanceInfo = nullptr;  // Optional

		if (cb.begin(beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error(
			    "error: failed to begin recording command buffer");
		}

		vk::RenderPassBeginInfo renderPassInfo;
		renderPassInfo.renderPass = rp;
		renderPassInfo.framebuffer = fb;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = rw.swapchainExtent();

		VkClearValue clearColor = { 0x27 / 255.0f, 0x28 / 255.0f,
			                        0x22 / 255.0f, 1.0f };
		// VkClearValue clearColor = {0};
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		cb.beginRenderPass(renderPassInfo);

		cb.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, mtl.pipeline());
		cb.draw(12);

		cb.endRenderPass();

		if (cb.end() != VK_SUCCESS)
		{
			throw std::runtime_error("error: failed to record command buffer");
		}
	}

	size_t currentFrame = 0;
	uint32_t imageIndex;
	while (!rw.shouldClose())
	{
		vk.wait(inFlightFences[currentFrame]);

		VkResult result = vk.AcquireNextImageKHR(
		    device, rw.swapchain(), UINT64_MAX,
		    imageAvailableSemaphores[currentFrame], nullptr, &imageIndex);

		// Check if a previous frame is using this image (i.e. there is its
		// fence to wait on)
		if (imagesInFlight[imageIndex] != nullptr)
		{
			vk.wait(imagesInFlight[imageIndex]);
		}
		// Mark the image as now being in use by this frame
		imagesInFlight[imageIndex] = inFlightFences[currentFrame];

		vk::SubmitInfo submitInfo;

		VkSemaphore waitSemaphores[] = {
			imageAvailableSemaphores[currentFrame]
		};
		VkPipelineStageFlags waitStages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		auto cb = commandBuffers[imageIndex].commandBuffer();
		submitInfo.pCommandBuffers = &cb;
		VkSemaphore signalSemaphores[] = {
			renderFinishedSemaphores[currentFrame]
		};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vk.ResetFences(device, 1, &inFlightFences[currentFrame]);

		if (vk.QueueSubmit(vk.graphicsQueue(), 1, &submitInfo,
		                   inFlightFences[currentFrame]) != VK_SUCCESS)
		{
			throw std::runtime_error(
			    "error failed to submit draw command buffer");
		}

		vk::PresentInfoKHR presentInfo;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { rw.swapchain() };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;  // Optional

		vk.QueuePresentKHR(vk.presentQueue(), &presentInfo);
		// vk.QueueWaitIdle(ctx.presentQueue());

		currentFrame = (currentFrame + 1) % 2;

		rw.pollEvents();
	}

	vk.wait();

	for (auto imageAvailableSemaphore : imageAvailableSemaphores)
	{
		vk.destroy(imageAvailableSemaphore);
	}

	for (auto renderFinishedSemaphore : renderFinishedSemaphores)
	{
		vk.destroy(renderFinishedSemaphore);
	}

	for (auto inFlightFence : inFlightFences)
	{
		vk.destroy(inFlightFence);
	}
}
