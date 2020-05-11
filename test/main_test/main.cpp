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

#define STB_IMAGE_WRITE_IMPLEMENTATION 1
#include "stb_image_write.h"

int main()
{
	using namespace cdm;

	RenderWindow rw(1280, 720, true);

	auto& vk = rw.device();
	vk.setLogActive();

	cdm::UniqueFence fence = vk.createFence();

	rw.acquireNextImage(fence.get());
	rw.present();

	Mandelbulb mandelbulb(rw);

	float sample = 0.0f;

	CommandBuffer computeCB(vk, rw.oneTimeCommandPool());
	CommandBuffer imguiCB(vk, rw.oneTimeCommandPool());
	CommandBuffer copyHDRCB(vk, rw.oneTimeCommandPool());
	CommandBuffer cb(vk, rw.oneTimeCommandPool());
	while (!rw.shouldClose())
	{
		rw.pollEvents();

		vk.wait(fence.get());
		vk.resetFence(fence.get());

		if (mandelbulb.mustClear)
		{
			mandelbulb.mustClear = false;
			sample = -1.0f;

			// CommandBuffer cb(vk, rw.oneTimeCommandPool());
			cb.reset();
			cb.begin();

			vk::ImageMemoryBarrier barrier;
			barrier.image = mandelbulb.outputImage();
			barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount =
			    mandelbulb.outputTexture().mipLevels();
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			cb.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
			                   VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);

			VkClearColorValue clearColor{};
			VkImageSubresourceRange range{};
			range.aspectMask =
			    VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
			range.layerCount = 1;
			range.levelCount = 1;
			cb.clearColorImage(mandelbulb.outputImage(),
			                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			                   &clearColor, range);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
			cb.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
			                   VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);

			cb.end();

			if (vk.queueSubmit(vk.graphicsQueue(), imguiCB.get()) !=
			    VK_SUCCESS)
			{
				std::cerr << "error: failed to submit clear command buffer"
				          << std::endl;
				abort();
			}

			vk.wait(vk.graphicsQueue());
			sample += 1.0f;
		}

		mandelbulb.setSampleAndRandomize(sample);

		computeCB.reset();
		computeCB.begin();
		computeCB.debugMarkerBegin("compute", 1.0f, 0.2f, 0.2f);
		mandelbulb.compute(computeCB);
		computeCB.debugMarkerEnd();
		computeCB.end();
		if (vk.queueSubmit(vk.graphicsQueue(), computeCB.get()) != VK_SUCCESS)
		{
			std::cerr << "error: failed to submit imgui command buffer"
			          << std::endl;
			abort();
		}
		vk.wait(vk.graphicsQueue());

		mandelbulb.outputTextureHDR().generateMipmapsImmediate(
		    VK_IMAGE_LAYOUT_GENERAL);

		imguiCB.reset();
		imguiCB.begin();
		imguiCB.debugMarkerBegin("render", 0.2f, 0.2f, 1.0f);
		// mandelbulb.compute(imguiCB);
		mandelbulb.render(imguiCB);
		mandelbulb.imgui(imguiCB);
		imguiCB.debugMarkerEnd();
		imguiCB.end();
		if (vk.queueSubmit(vk.graphicsQueue(), imguiCB.get()) != VK_SUCCESS)
		{
			std::cerr << "error: failed to submit imgui command buffer"
			          << std::endl;
			abort();
		}
		vk.wait(vk.graphicsQueue());

		sample += 1.0f;

		auto swapImages = rw.swapchainImages();

		vk.debugMarkerSetObjectName(
		    copyHDRCB.get(), VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
		    "copyHDRCB");

		copyHDRCB.reset();
		copyHDRCB.begin();

		vk::ImageMemoryBarrier barrier;
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = mandelbulb.outputImage();
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

			copyHDRCB.blitImage(
			    mandelbulb.outputImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			    swapImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, blit,
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

		rw.acquireNextImage(fence.get());
		rw.present();
	}

	vk.wait(fence.get());
	vk.wait(vk.graphicsQueue());
	vk.wait();
}
