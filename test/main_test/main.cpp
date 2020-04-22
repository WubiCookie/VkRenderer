#include "CommandBuffer.hpp"
#include "Framebuffer.hpp"
#include "Mandelbulb.hpp"
#include "Material.hpp"
#include "RenderPass.hpp"
#include "RenderWindow.hpp"
#include "Renderer.hpp"

#include <fstream>
#include <iostream>
#include <vector>

int main()
{
	using namespace cdm;

	RenderWindow rw(1280, 720, true);

	rw.prerender();
	rw.present();

	auto& vk = rw.device();
	// auto device = vk.vkDevice();

	Renderer ren(rw);

	Mandelbulb mandelbulb(rw);

	// CommandBuffer transitionCB(vk, rw.commandPool());

	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	// transitionCB.begin(beginInfo);
	// vk::ImageMemoryBarrier barrier;
	// barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	// barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	// barrier.image = mandelbulb.outputImage();
	// barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	// barrier.subresourceRange.baseMipLevel = 0;
	// barrier.subresourceRange.levelCount = 1;
	// barrier.subresourceRange.baseArrayLayer = 0;
	// barrier.subresourceRange.layerCount = 1;
	// barrier.srcAccessMask = 0;
	// barrier.dstAccessMask = 0;
	// transitionCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	// VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_DEPENDENCY_DEVICE_GROUP_BIT,
	// barrier); if (transitionCB.end() != VK_SUCCESS)
	//{
	//	std::cerr << "error: failed to record command buffer" << std::endl;
	//	abort();
	//}

	vk::SubmitInfo submitInfo;

	//// VkSemaphore waitSemaphores[] = {
	////	rw.get().currentImageAvailableSemaphore()
	////};
	//// VkPipelineStageFlags waitStages[] = {
	////	VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	////};
	//// submitInfo.waitSemaphoreCount = 1;
	//// submitInfo.pWaitSemaphores = waitSemaphores;
	//// submitInfo.pWaitDstStageMask = waitStages;

	//// auto cb = m_commandBuffers[rw.get().imageIndex()].commandBuffer();
	// auto cb = transitionCB.commandBuffer();
	submitInfo.commandBufferCount = 1;
	// submitInfo.pCommandBuffers = &cb;
	//// VkSemaphore signalSemaphores[] = {
	////	m_renderFinishedSemaphores[rw.get().currentFrame()]
	////};
	//// submitInfo.signalSemaphoreCount = 1;
	//// submitInfo.pSignalSemaphores = signalSemaphores;

	//// VkFence inFlightFence = rw.get().currentInFlightFences();

	//// vk.ResetFences(vk.vkDevice(), 1, &inFlightFence);

	// if (vk.queueSubmit(vk.graphicsQueue(), submitInfo) != VK_SUCCESS)
	//{
	//	std::cerr << "error: failed to submit draw command buffer"
	//	          << std::endl;
	//	abort();
	//}

	CommandBuffer renderCB(vk, rw.commandPool());

	renderCB.begin(beginInfo);
	mandelbulb.render(renderCB);
	if (renderCB.end() != VK_SUCCESS)
	{
		std::cerr << "error: failed to record command buffer" << std::endl;
		abort();
	}

	vk.wait(vk.graphicsQueue());

	auto cb = renderCB.commandBuffer();
	submitInfo.pCommandBuffers = &cb;

	if (vk.queueSubmit(vk.graphicsQueue(), submitInfo) != VK_SUCCESS)
	{
		std::cerr << "error: failed to submit draw command buffer"
		          << std::endl;
		abort();
	}

	auto swapImages = rw.swapchainImages();

	CommandBuffer copyCB(vk, rw.commandPool());

	copyCB.begin(beginInfo);
	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = mandelbulb.outputImage();
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;
	copyCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, barrier);

	for (auto swapImage : swapImages)
	{
		vk::ImageMemoryBarrier swapBarrier = barrier;
		swapBarrier.image = swapImage;
		swapBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		swapBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		copyCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
		                       swapBarrier);

		VkImageCopy copy{};
		copy.extent.width = 1280;
		copy.extent.height = 720;
		copy.extent.depth = 1;
		copy.srcSubresource.aspectMask =
		    VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		copy.srcSubresource.baseArrayLayer = 0;
		copy.srcSubresource.layerCount = 1;
		copy.srcSubresource.mipLevel = 0;
		copy.dstSubresource.aspectMask =
		    VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		copy.dstSubresource.baseArrayLayer = 0;
		copy.dstSubresource.layerCount = 1;
		copy.dstSubresource.mipLevel = 0;

		copyCB.copyImage(mandelbulb.outputImage(),
		                 // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		                 // swapImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, copy);
		                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapImage,
		                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copy);

		//VkImageBlit blit{};
		//blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		//blit.srcSubresource.baseArrayLayer = 0;
		//blit.srcSubresource.layerCount = 1;
		//blit.srcSubresource.mipLevel = 0;
		//blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		//blit.dstSubresource.baseArrayLayer = 0;
		//blit.dstSubresource.layerCount = 1;
		//blit.dstSubresource.mipLevel = 0;

		//copyCB.blitImage(mandelbulb.outputImage(),
		//	// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		//	// swapImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, copy);
		//	VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapImage,
		//	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, blit, VK_FILTER_NEAREST);

		swapBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		swapBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		copyCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
		                       swapBarrier);
	}

	vk::ImageMemoryBarrier barrier2 = barrier;
	barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier2.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	copyCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, barrier2);

	if (copyCB.end() != VK_SUCCESS)
	{
		std::cerr << "error: failed to record command buffer" << std::endl;
		abort();
	}

	vk.wait(vk.graphicsQueue());

	cb = copyCB.commandBuffer();
	submitInfo.pCommandBuffers = &cb;

	if (vk.queueSubmit(vk.graphicsQueue(), submitInfo) != VK_SUCCESS)
	{
		std::cerr << "error: failed to submit draw command buffer"
		          << std::endl;
		abort();
	}

	vk.wait(vk.graphicsQueue());

	while (!rw.shouldClose())
	{
		// ren.render();
		rw.prerender();
		rw.present();
		rw.pollEvents();
	}
}
