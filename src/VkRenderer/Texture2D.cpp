#include "Texture2D.hpp"

#include "CommandBuffer.hpp"
#include "CommandBufferPool.hpp"
#include "RenderWindow.hpp"
#include "StagingBuffer.hpp"

#include <stdexcept>

#include <iostream>

namespace cdm
{
Texture2D::Texture2D(const VulkanDevice& vulkanDevice,
                     vk::ImageCreateInfo imageInfo,
                     VmaAllocationCreateInfo alloceInfo,
                     vk::ImageViewCreateInfo viewInfo,
                     vk::SamplerCreateInfo samplerInfo)
    : m_vulkanDevice(&vulkanDevice)
{
	auto& vk = *m_vulkanDevice.get();

	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.depth = 1;
	imageInfo.arrayLayers = 1;

	imageInfo.mipLevels =
	    std::min(imageInfo.mipLevels,
	             static_cast<uint32_t>(std::floor(std::log2(std::max(
	                 imageInfo.extent.width, imageInfo.extent.height)))) +
	                 1);

	VmaAllocationInfo allocInfo{};

	vmaCreateImage(vk.allocator(), &imageInfo, &alloceInfo, &m_image.get(),
	               &m_allocation.get(), &allocInfo);

	if (m_image == false)
		throw std::runtime_error("could not create image");

	m_width = imageInfo.extent.width;
	m_height = imageInfo.extent.height;
	m_deviceMemory = allocInfo.deviceMemory;
	m_offset = allocInfo.offset;
	m_size = allocInfo.size;
	m_mipLevels = imageInfo.mipLevels;
	m_samples = imageInfo.samples;
	m_format = imageInfo.format;
	m_aspectMask = viewInfo.subresourceRange.aspectMask;

	viewInfo.image = m_image.get();
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	m_imageView = vk.create(viewInfo);
	if (!m_imageView)
		throw std::runtime_error("could not create image view");

	m_sampler = vk.create(samplerInfo);
	if (!m_sampler)
		throw std::runtime_error("could not create sampler");
}

Texture2D::~Texture2D()
{
	if (m_vulkanDevice)
	{
		auto& vk = *m_vulkanDevice.get();

		if (m_imageView)
			m_imageView.reset();
		if (m_image)
			vmaDestroyImage(vk.allocator(), m_image.get(), m_allocation.get());
	}
}

void Texture2D::transitionLayoutImmediate(VkImageLayout oldLayout,
                                          VkImageLayout newLayout)
{
	auto& vk = *m_vulkanDevice.get();

	CommandBufferPool pool(vk, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	auto& frame = pool.getAvailableCommandBuffer();
	CommandBuffer& transitionCB = frame.commandBuffer;

	transitionCB.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	vk::ImageMemoryBarrier barrier;
	barrier.image = image();
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = m_aspectMask;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = m_mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;
	transitionCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
	                             VK_DEPENDENCY_DEVICE_GROUP_BIT, barrier);
	if (transitionCB.end() != VK_SUCCESS)
		throw std::runtime_error("failed to record transition command buffer");

	if (frame.submit(vk.graphicsQueue()) != VK_SUCCESS)
		throw std::runtime_error("failed to submit transition command buffer");

	pool.waitForAllCommandBuffers();
}

void Texture2D::generateMipmapsImmediate(VkImageLayout currentLayout)
{
	if (m_mipLevels <= 1)
		return;

	auto& vk = *m_vulkanDevice.get();

	CommandBufferPool pool(vk, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	auto& frame = pool.getAvailableCommandBuffer();
	CommandBuffer& mipmapCB = frame.commandBuffer;

	mipmapCB.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	mipmapCB.debugMarkerBegin("mipmap generation");
	vk::ImageMemoryBarrier barrier;
	barrier.image = image();
	barrier.oldLayout = currentLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = m_aspectMask;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	mipmapCB.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
	                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);

	int32_t mipWidth = int32_t(m_width);
	int32_t mipHeight = int32_t(m_height);

	for (uint32_t i = 1; i < m_mipLevels; i++)
	{
		barrier.subresourceRange.baseMipLevel = i;
		barrier.oldLayout = currentLayout;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		mipmapCB.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
		                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { int32_t(m_width), int32_t(m_height), 1 };
		blit.srcSubresource.aspectMask = m_aspectMask;
		blit.srcSubresource.mipLevel = 0;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;

		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1,
			                   mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = m_aspectMask;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		mipmapCB.blitImage(image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		                   image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, blit,
		                   VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = currentLayout;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		mipmapCB.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
		                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		                         barrier);

		if (mipWidth > 1)
			mipWidth /= 2;
		if (mipHeight > 1)
			mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout = currentLayout;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	mipmapCB.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
	                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
	                         barrier);

	mipmapCB.debugMarkerEnd();
	if (mipmapCB.end() != VK_SUCCESS)
		throw std::runtime_error("failed to record mipmap command buffer");

	if (frame.submit(vk.graphicsQueue()) != VK_SUCCESS)
		throw std::runtime_error("failed to submit mipmap command buffer");

	pool.waitForAllCommandBuffers();
}

void Texture2D::uploadDataImmediate(const void* texels, size_t size,
                                    const VkBufferImageCopy& region,
                                    VkImageLayout currentLayout)
{
	uploadDataImmediate(texels, size, region, currentLayout, currentLayout);
}

void Texture2D::uploadDataImmediate(const void* texels, size_t size,
                                    const VkBufferImageCopy& region,
                                    VkImageLayout initialLayout,
                                    VkImageLayout finalLayout)
{
	auto& vk = *m_vulkanDevice.get();

	CommandBufferPool pool(vk, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

	uploadData(texels, size, region, initialLayout, finalLayout, pool);

	pool.waitForAllCommandBuffers();

	/*
	auto& vk = *m_vulkanDevice.get();

	CommandBufferPool pool(vk, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

	StagingBuffer stagingBuffer(vk, texels, size);

	auto& frame = pool.getAvailableCommandBuffer();
	CommandBuffer& cb = frame.commandBuffer;

	cb.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = initialLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image();
	barrier.subresourceRange.aspectMask = m_aspectMask;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels();
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
	cb.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
	                   VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);

	cb.copyBufferToImage(stagingBuffer, image(),
	                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, region);

	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = finalLayout;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = 0;
	cb.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
	                   VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);

	if (cb.end() != VK_SUCCESS)
	    throw std::runtime_error("failed to record upload command buffer");

	if (frame.submit(vk.graphicsQueue()) != VK_SUCCESS)
	    throw std::runtime_error("failed to submit upload command buffer");

	pool.waitForAllCommandBuffers();
	//*/
}

void Texture2D::uploadData(const void* texels, size_t size,
                           const VkBufferImageCopy& region,
                           VkImageLayout currentLayout,
                           CommandBufferPool& pool)
{
	uploadData(texels, size, region, currentLayout, currentLayout, pool);
}

void Texture2D::uploadData(const void* texels, size_t size,
                           const VkBufferImageCopy& region,
                           VkImageLayout initialLayout,
                           VkImageLayout finalLayout, CommandBufferPool& pool)
{
	auto& vk = *m_vulkanDevice.get();

	StagingBuffer stagingBuffer(vk, texels, size);

	auto& frame = pool.getAvailableCommandBuffer();
	CommandBuffer& cb = frame.commandBuffer;

	if (cb.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) != VK_SUCCESS)
		throw std::runtime_error("failed to begin upload command buffer");

	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = initialLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image();
	barrier.subresourceRange.aspectMask = m_aspectMask;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels();
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
	cb.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
	                   VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);

	cb.copyBufferToImage(stagingBuffer, image(),
	                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, region);

	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = finalLayout;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = 0;
	cb.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
	                   VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);

	if (cb.end() != VK_SUCCESS)
		throw std::runtime_error("failed to record upload command buffer");

	if (frame.submit(vk.graphicsQueue()) != VK_SUCCESS)
		throw std::runtime_error("failed to submit upload command buffer");

	pool.registerResource(std::move(stagingBuffer));
}

Buffer Texture2D::downloadDataToBufferImmediate(VkImageLayout currentLayout)
{
	return downloadDataToBufferImmediate(currentLayout, currentLayout);
}

Buffer Texture2D::downloadDataToBufferImmediate(VkImageLayout initialLayout,
                                                VkImageLayout finalLayout)
{
	if (m_image.get() == nullptr)
		return {};

	auto& vk = *m_vulkanDevice.get();

	VkDeviceSize dataSize = size();

	Buffer tmpBuffer(vk, dataSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                 VMA_MEMORY_USAGE_GPU_TO_CPU,
	                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	VkBufferImageCopy copy{};
	copy.bufferRowLength = width();
	copy.bufferImageHeight = height();
	copy.imageExtent = extent3D();
	copy.imageSubresource.aspectMask = m_aspectMask;
	copy.imageSubresource.layerCount = 1;

	CommandBufferPool pool(vk, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	auto& frame = pool.getAvailableCommandBuffer();
	CommandBuffer& cb = frame.commandBuffer;

	cb.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = initialLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image();
	barrier.subresourceRange.aspectMask = m_aspectMask;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels();
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	cb.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
	                   VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);

	cb.copyImageToBuffer(image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	                     tmpBuffer, copy);

	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout = finalLayout;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	barrier.dstAccessMask = 0;
	cb.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
	                   VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);

	if (cb.end() != VK_SUCCESS)
		throw std::runtime_error("failed to record upload command buffer");

	if (frame.submit(vk.graphicsQueue()) != VK_SUCCESS)
		throw std::runtime_error("failed to submit upload command buffer");

	pool.waitForAllCommandBuffers();

	// return tmpBuffer.download();
	return tmpBuffer;
}

void Texture2D::setName(std::string_view name)
{
	if (get())
	{
		auto& vk = *m_vulkanDevice.get();
		vk.debugMarkerSetObjectName(get(), name);
	}
}
}  // namespace cdm
