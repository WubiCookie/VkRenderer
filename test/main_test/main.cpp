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

	rw.prerender();
	rw.present();

	auto& vk = rw.device();
	// auto device = vk.vkDevice();

	Renderer ren(rw);

	Mandelbulb mandelbulb(rw);
	mandelbulb.outputTextureHDR().transitionLayoutImediate(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

	// CommandBuffer transitionCB(vk, rw.commandPool());

	vk::CommandBufferBeginInfo beginInfo;
	// beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	//*
	CommandBuffer computeCB(vk, rw.commandPool());

	computeCB.begin(beginInfo);

	//vk::ImageMemoryBarrier computeBarrier;
	//computeBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	//computeBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	//computeBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//computeBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//computeBarrier.image = mandelbulb.outputImageHDR();
	//computeBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	//computeBarrier.subresourceRange.baseMipLevel = 0;
	//computeBarrier.subresourceRange.levelCount = 1;
	//computeBarrier.subresourceRange.baseArrayLayer = 0;
	//computeBarrier.subresourceRange.layerCount = 1;
	//computeBarrier.srcAccessMask = 0;
	//computeBarrier.dstAccessMask = 0;
	//computeCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	//                          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
	//                          computeBarrier);

	mandelbulb.compute(computeCB);
	computeCB.end();


	mandelbulb.setSampleAndRandomize(1.0f);
	if (vk.queueSubmit(vk.graphicsQueue(), computeCB.get()) != VK_SUCCESS)
	{
		std::cerr << "error: failed to submit compute command buffer"
		          << std::endl;
		abort();
	}
	vk.wait(vk.graphicsQueue());

	mandelbulb.setSampleAndRandomize(2.0f);
	if (vk.queueSubmit(vk.graphicsQueue(), computeCB.get()) != VK_SUCCESS)
	{
		std::cerr << "error: failed to submit compute command buffer"
			        << std::endl;
		abort();
	}
	vk.wait(vk.graphicsQueue());

	mandelbulb.setSampleAndRandomize(3.0f);
	if (vk.queueSubmit(vk.graphicsQueue(), computeCB.get()) != VK_SUCCESS)
	{
		std::cerr << "error: failed to submit compute command buffer"
			        << std::endl;
		abort();
	}
	vk.wait(vk.graphicsQueue());

	rw.prerender();
	rw.present();
	rw.pollEvents();
	return 0;

	/*
	{
		vk.wait(vk.graphicsQueue());

		CommandBuffer copyCB(vk, rw.commandPool());

		copyCB.begin(beginInfo);

		vk::ImageMemoryBarrier barrier;
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = mandelbulb.outputImageHDR();
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;
		copyCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
		                       barrier);

		VmaAllocation HDRBufferAllocation;
		VkBuffer HDRBuffer;

		vk::BufferCreateInfo vbInfo;
		vbInfo.size = sizeof(float) * 4 * 1280 * 720;
		vbInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		vbInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo vbAllocCreateInfo = {};
		vbAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
		// vbAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		vbAllocCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		VmaAllocationInfo HDRBufferAllocInfo = {};
		vmaCreateBuffer(vk.allocator(), &vbInfo, &vbAllocCreateInfo,
		                &HDRBuffer, &HDRBufferAllocation, &HDRBufferAllocInfo);

		VkBufferImageCopy copy{};
		copy.bufferRowLength = 1280;
		copy.bufferImageHeight = 720;
		copy.imageExtent.width = 1280;
		copy.imageExtent.height = 720;
		copy.imageExtent.depth = 1;
		copy.imageSubresource.aspectMask =
		    VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		copy.imageSubresource.baseArrayLayer = 0;
		copy.imageSubresource.layerCount = 1;
		copy.imageSubresource.mipLevel = 0;

		copyCB.copyImageToBuffer(mandelbulb.outputImageHDR(),
		                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		                         HDRBuffer, copy);

		if (copyCB.end() != VK_SUCCESS)
		{
			std::cerr << "error: failed to record command buffer" << std::endl;
			abort();
		}

		if (vk.queueSubmit(vk.graphicsQueue(), copyCB.get()) != VK_SUCCESS)
		{
			std::cerr << "error: failed to submit copy command buffer"
			          << std::endl;
			abort();
		}

		vk.wait(vk.graphicsQueue());

		void* data;

		vmaMapMemory(vk.allocator(), HDRBufferAllocation, &data);

		float* dataf = static_cast<float*>(data);
		// dataf[1 + 1280 * 1] = 1.0f;

		stbi_write_hdr("D:\\Projects\\out.hdr", 1280, 720, 4, dataf);

		vmaUnmapMemory(vk.allocator(), HDRBufferAllocation);

		// copy and write image to disk

		vmaDestroyBuffer(vk.allocator(), HDRBuffer, HDRBufferAllocation);

		mandelbulb.setSampleAndRandomize(2.0f);
		if (vk.queueSubmit(vk.graphicsQueue(), computeCB.get()) != VK_SUCCESS)
		{
			std::cerr << "error: failed to submit compute command buffer"
			          << std::endl;
			abort();
		}
		vk.wait(vk.graphicsQueue());

		mandelbulb.setSampleAndRandomize(3.0f);
		if (vk.queueSubmit(vk.graphicsQueue(), computeCB.get()) != VK_SUCCESS)
		{
			std::cerr << "error: failed to submit compute command buffer"
			          << std::endl;
			abort();
		}
		vk.wait(vk.graphicsQueue());

		rw.prerender();
		rw.present();
		rw.pollEvents();
		return 0;

		CommandBuffer copyHDRCB(vk, rw.commandPool());
		while (!rw.shouldClose())
		{
			auto swapImages = rw.swapchainImages();

			vk.debugMarkerSetObjectName(
			    copyHDRCB.get(),
			    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, "copyHDRCB");

			copyHDRCB.begin(beginInfo);

			// copyHDRCB.debugMarkerBegin("mandelbulb color to transfer src", {
			// 0.4f, 0.4f, 1.0f, 1.0f });
			vk::ImageMemoryBarrier barrier;
			barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = mandelbulb.outputImageHDR();
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = 0;
			// copyHDRCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			//                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
			//                       barrier);

			for (auto swapImage : swapImages)
			{
				copyHDRCB.debugMarkerInsert(
				    "swapImage present src to transfer dst", 0.4f, 0.4f, 1.0f);

				vk::ImageMemoryBarrier swapBarrier = barrier;
				swapBarrier.image = swapImage;
				swapBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				swapBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				copyHDRCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				                          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				                          0, swapBarrier);

				// VkImageCopy copy{};
				// copy.extent.width = 1280;
				// copy.extent.height = 720;
				// copy.extent.depth = 1;
				// copy.srcSubresource.aspectMask =
				// VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
				// copy.srcSubresource.baseArrayLayer = 0;
				// copy.srcSubresource.layerCount = 1;
				// copy.srcSubresource.mipLevel = 0;
				// copy.dstSubresource.aspectMask =
				// VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
				// copy.dstSubresource.baseArrayLayer = 0;
				// copy.dstSubresource.layerCount = 1;
				// copy.dstSubresource.mipLevel = 0;

				copyHDRCB.debugMarkerInsert("copy", 0.4f, 1.0f, 0.4f);

				// copyHDRCB.copyImage(mandelbulb.outputImage(),
				//                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				//                 swapImage,
				//                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copy);

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

				copyHDRCB.blitImage(mandelbulb.outputImage(),
				                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				                    swapImage,
				                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, blit,
				                    VkFilter::VK_FILTER_LINEAR);

				swapBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				swapBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				swapBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
				// swapBarrier.dstAccessMask = VK_ACCESS_;

				copyHDRCB.debugMarkerInsert(
				    "swapImage transfer dst to present src", 1, 1, 0.4f);

				copyHDRCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				                          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				                          0, swapBarrier);
			}

			copyHDRCB.debugMarkerEnd();

			if (copyHDRCB.end() != VK_SUCCESS)
			{
				std::cerr << "error: failed to record command buffer"
				          << std::endl;
				abort();
			}

			vk.wait(vk.graphicsQueue());

			if (vk.queueSubmit(vk.graphicsQueue(), copyHDRCB.get()) !=
			    VK_SUCCESS)
			{
				std::cerr << "error: failed to submit draw command buffer"
				          << std::endl;
				abort();
			}

			vk.wait(vk.graphicsQueue());

			rw.prerender();
			rw.present();
			rw.pollEvents();
		}

		return 0;
	}

	return 0;
	//*/

	CommandBuffer renderCB(vk, rw.commandPool());
	vk.debugMarkerSetObjectName(renderCB.get(),
	                            VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
	                            "renderCB");

	renderCB.begin(beginInfo);
	renderCB.debugMarkerBegin("render", 1.0f, 0.0f, 0.0f);
	// renderCB.debugMarkerInsert("render!", { 1.0f, 0.4f, 0.4f, 1.0f });
	mandelbulb.render(renderCB);

	renderCB.debugMarkerEnd();

	if (renderCB.end() != VK_SUCCESS)
	{
		std::cerr << "error: failed to record command buffer" << std::endl;
		abort();
	}

	vk.wait(vk.graphicsQueue());

	if (vk.queueSubmit(vk.graphicsQueue(), renderCB.get()) != VK_SUCCESS)
	{
		std::cerr << "error: failed to submit draw command buffer"
		          << std::endl;
		abort();
	}

	for (int i = 0; i < 0; i++)
	{
		std::cout << i << std::endl;

		vk.wait(vk.graphicsQueue());
		mandelbulb.randomizePoints();
		if (vk.queueSubmit(vk.graphicsQueue(), renderCB.get()) != VK_SUCCESS)
		{
			std::cerr << "error: failed to submit draw command buffer"
			          << std::endl;
			abort();
		}
	}

	/*
	{
	    vk.wait(vk.graphicsQueue());

	    CommandBuffer copyCB(vk, rw.commandPool());

	    copyCB.begin(beginInfo);
	    // vk::ImageMemoryBarrier barrier;
	    // barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	    // barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
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
	    // copyCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	    //                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
	    //                       barrier);

	    VmaAllocation HDRBufferAllocation;
	    VkBuffer HDRBuffer;

	    vk::BufferCreateInfo vbInfo;
	    vbInfo.size = sizeof(float) * 4 * 1280 * 720;
	    vbInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	    vbInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	    VmaAllocationCreateInfo vbAllocCreateInfo = {};
	    vbAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
	    // vbAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	    vbAllocCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	    VmaAllocationInfo HDRBufferAllocInfo = {};
	    vmaCreateBuffer(vk.allocator(), &vbInfo, &vbAllocCreateInfo,
	                    &HDRBuffer, &HDRBufferAllocation, &HDRBufferAllocInfo);

	    VkBufferImageCopy copy{};
	    copy.bufferRowLength = 1280;
	    copy.bufferImageHeight = 720;
	    copy.imageExtent.width = 1280;
	    copy.imageExtent.height = 720;
	    copy.imageExtent.depth = 1;
	    copy.imageSubresource.aspectMask =
	VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	    copy.imageSubresource.baseArrayLayer = 0;
	    copy.imageSubresource.layerCount = 1;
	    copy.imageSubresource.mipLevel = 0;

	    copyCB.copyImageToBuffer(
	        mandelbulb.outputImageHDR(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	        HDRBuffer, copy);

	    if (copyCB.end() != VK_SUCCESS)
	    {
	        std::cerr << "error: failed to record command buffer" << std::endl;
	        abort();
	    }

	    if (vk.queueSubmit(vk.graphicsQueue(), copyCB.get()) != VK_SUCCESS)
	    {
	        std::cerr << "error: failed to submit copy command buffer"
	                  << std::endl;
	        abort();
	    }

	    vk.wait(vk.graphicsQueue());

	    void* data;

	    vmaMapMemory(vk.allocator(), HDRBufferAllocation, &data);

	    float* dataf = static_cast<float*>(data);
	    //dataf[1 + 1280 * 1] = 1.0f;

	    stbi_write_hdr("D:\\Projects\\out.hdr", 1280, 720, 4, dataf);

	    vmaUnmapMemory(vk.allocator(), HDRBufferAllocation);

	    // copy and write image to disk

	    vmaDestroyBuffer(vk.allocator(), HDRBuffer, HDRBufferAllocation);

	    rw.prerender();
	    rw.present();
	    rw.pollEvents();

	    return 0;
	}
	//*/

	auto swapImages = rw.swapchainImages();

	CommandBuffer copyCB(vk, rw.commandPool());
	vk.debugMarkerSetObjectName(copyCB.get(),
	                            VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
	                            "copyCB");

	copyCB.begin(beginInfo);

	// copyCB.debugMarkerBegin("mandelbulb color to transfer src", { 0.4f,
	// 0.4f, 1.0f, 1.0f });
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
	// copyCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	//                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, barrier);

	for (auto swapImage : swapImages)
	{
		copyCB.debugMarkerInsert("swapImage present src to transfer dst", 0.4f,
		                         0.4f, 1.0f);

		vk::ImageMemoryBarrier swapBarrier = barrier;
		swapBarrier.image = swapImage;
		swapBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		swapBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		copyCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
		                       swapBarrier);

		// VkImageCopy copy{};
		// copy.extent.width = 1280;
		// copy.extent.height = 720;
		// copy.extent.depth = 1;
		// copy.srcSubresource.aspectMask =
		// VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		// copy.srcSubresource.baseArrayLayer = 0;
		// copy.srcSubresource.layerCount = 1;
		// copy.srcSubresource.mipLevel = 0;
		// copy.dstSubresource.aspectMask =
		// VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		// copy.dstSubresource.baseArrayLayer = 0;
		// copy.dstSubresource.layerCount = 1;
		// copy.dstSubresource.mipLevel = 0;

		copyCB.debugMarkerInsert("copy", 0.4f, 1.0f, 0.4f);

		// copyCB.copyImage(mandelbulb.outputImage(),
		//                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapImage,
		//                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copy);

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

		copyCB.blitImage(mandelbulb.outputImage(),
		                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapImage,
		                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, blit,
		                 VkFilter::VK_FILTER_LINEAR);

		swapBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		swapBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		swapBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		// swapBarrier.dstAccessMask = VK_ACCESS_;

		copyCB.debugMarkerInsert("swapImage transfer dst to present src", 1, 1,
		                         0.4f);

		copyCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
		                       swapBarrier);
	}

	copyCB.debugMarkerEnd();

	if (copyCB.end() != VK_SUCCESS)
	{
		std::cerr << "error: failed to record command buffer" << std::endl;
		abort();
	}

	vk.wait(vk.graphicsQueue());

	if (vk.queueSubmit(vk.graphicsQueue(), copyCB.get()) != VK_SUCCESS)
	{
		std::cerr << "error: failed to submit draw command buffer"
		          << std::endl;
		abort();
	}

	vk.wait(vk.graphicsQueue());

	bool swapchainRecreated;
	while (!rw.shouldClose())
	{
		if (vk.queueSubmit(vk.graphicsQueue(), renderCB.get()) != VK_SUCCESS)
		{
			std::cerr << "error: failed to submit draw command buffer"
			          << std::endl;
			abort();
		}

		vk.wait(vk.graphicsQueue());
		mandelbulb.randomizePoints();

		if (vk.queueSubmit(vk.graphicsQueue(), copyCB.get()) != VK_SUCCESS)
		{
			std::cerr << "error: failed to submit draw command buffer"
			          << std::endl;
			abort();
		}

		vk.wait(vk.graphicsQueue());

		// ren.render();
		rw.prerender();
		rw.present(swapchainRecreated);

		if (swapchainRecreated)
		{
			vk.wait();

			auto swapImages = rw.swapchainImages();

			copyCB = CommandBuffer(vk, rw.commandPool());
			vk.debugMarkerSetObjectName(
			    copyCB.get(), VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
			    "copyCB");

			copyCB.begin(beginInfo);

			// copyCB.debugMarkerBegin("mandelbulb color to transfer src", {
			// 0.4f, 0.4f, 1.0f, 1.0f });
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
			// copyCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			//                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
			//                       barrier);

			for (auto swapImage : swapImages)
			{
				copyCB.debugMarkerInsert(
				    "swapImage present src to transfer dst", 0.4f, 0.4f, 1.0f);

				vk::ImageMemoryBarrier swapBarrier = barrier;
				swapBarrier.image = swapImage;
				swapBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				swapBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				copyCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
				                       swapBarrier);

				// VkImageCopy copy{};
				// copy.extent.width = 1280;
				// copy.extent.height = 720;
				// copy.extent.depth = 1;
				// copy.srcSubresource.aspectMask =
				// VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
				// copy.srcSubresource.baseArrayLayer = 0;
				// copy.srcSubresource.layerCount = 1;
				// copy.srcSubresource.mipLevel = 0;
				// copy.dstSubresource.aspectMask =
				// VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
				// copy.dstSubresource.baseArrayLayer = 0;
				// copy.dstSubresource.layerCount = 1;
				// copy.dstSubresource.mipLevel = 0;

				copyCB.debugMarkerInsert("copy", 0.4f, 1.0f, 0.4f);

				// copyCB.copyImage(mandelbulb.outputImage(),
				//                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				//                 swapImage,
				//                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copy);

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

				copyCB.blitImage(mandelbulb.outputImage(),
				                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				                 swapImage,
				                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, blit,
				                 VkFilter::VK_FILTER_LINEAR);

				swapBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				swapBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				swapBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
				// swapBarrier.dstAccessMask = VK_ACCESS_;

				copyCB.debugMarkerInsert(
				    "swapImage transfer dst to present src", 1, 1, 0.4f);

				copyCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
				                       swapBarrier);
			}

			// copyCB.debugMarkerBegin("mandelbulb transfer src to color", {
			// 0.4f, 0.4f, 1.0f, 1.0f }); barrier.oldLayout =
			// VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; barrier.newLayout =
			// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; barrier.srcAccessMask
			// = VK_ACCESS_MEMORY_READ_BIT;
			// copyCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			//                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
			//                       barrier);

			copyCB.debugMarkerEnd();

			if (copyCB.end() != VK_SUCCESS)
			{
				std::cerr << "error: failed to record command buffer"
				          << std::endl;
				abort();
			}

			// vk.wait(vk.graphicsQueue());
		}

		rw.pollEvents();
	}
}
