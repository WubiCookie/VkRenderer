#include "Texture2D.hpp"

#include "CommandBuffer.hpp"
#include "RenderWindow.hpp"
#include "StagingBuffer.hpp"

#include <stdexcept>

namespace cdm
{
Texture2D::Texture2D(RenderWindow& renderWindow, uint32_t imageWidth,
                     uint32_t imageHeight, VkFormat imageFormat,
                     VkImageTiling imageTiling, VkImageUsageFlags usage,
                     VmaMemoryUsage memoryUsage,
                     VkMemoryPropertyFlags requiredFlags, uint32_t mipLevels)
    : rw(&renderWindow)
{
	auto& vk = rw.get()->device();

#pragma region image
	mipLevels = std::min(mipLevels, static_cast<uint32_t>(std::floor(std::log2(
	                                    std::max(imageWidth, imageHeight)))) +
	                                    1);

	vk::ImageCreateInfo info;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.extent.width = imageWidth;
	info.extent.height = imageHeight;
	info.extent.depth = 1;
	info.mipLevels = mipLevels;
	info.arrayLayers = 1;
	info.format = imageFormat;
	info.tiling = imageTiling;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.usage = usage;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.flags = 0;

	VmaAllocationCreateInfo imageAllocCreateInfo = {};
	imageAllocCreateInfo.usage = memoryUsage;
	imageAllocCreateInfo.requiredFlags = requiredFlags;

	VmaAllocationInfo allocInfo{};

	VkResult res =
	    vmaCreateImage(vk.allocator(), &info, &imageAllocCreateInfo,
	                   &m_image.get(), &m_allocation.get(), &allocInfo);

	if (m_image == false)
		throw std::runtime_error("could not create image");
#pragma endregion

	m_width = imageWidth;
	m_height = imageHeight;
	m_deviceMemory = allocInfo.deviceMemory;
	m_offset = allocInfo.offset;
	m_size = allocInfo.size;
	m_mipLevels = mipLevels;
	m_format = imageFormat;

#pragma region view
	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = m_image.get();
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = imageFormat;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	m_imageView = vk.create(viewInfo);
	if (!m_imageView)
		throw std::runtime_error("could not create image view");
#pragma endregion

#pragma region sampler
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = (float)info.mipLevels;

	m_sampler = vk.create(samplerInfo);
	if (!m_sampler)
		throw std::runtime_error("could not create sampler");
#pragma endregion
}

Texture2D::~Texture2D()
{
	if (rw.get())
	{
		auto& vk = rw.get()->device();

		if (m_imageView)
			m_imageView.reset();
		if (m_image)
			vmaDestroyImage(vk.allocator(), m_image.get(), m_allocation.get());
	}
}

void Texture2D::transitionLayoutImmediate(VkImageLayout oldLayout,
                                          VkImageLayout newLayout)
{
	if (rw.get() == nullptr)
		return;

	auto& vk = rw.get()->device();

	CommandBuffer transitionCB(vk, rw.get()->commandPool());

	transitionCB.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	vk::ImageMemoryBarrier barrier;
	barrier.image = image();
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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

	if (vk.queueSubmit(vk.graphicsQueue(), transitionCB.get()) != VK_SUCCESS)
		throw std::runtime_error("failed to submit transition command buffer");

	vk.wait(vk.graphicsQueue());
}

void Texture2D::generateMipmapsImmediate(VkImageLayout currentLayout)
{
	if (rw.get() == nullptr)
		return;
	if (m_mipLevels <= 1)
		return;

	auto& vk = rw.get()->device();

	CommandBuffer mipmapCB(vk, rw.get()->commandPool());

	mipmapCB.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	mipmapCB.debugMarkerBegin("mipmap generation");
	vk::ImageMemoryBarrier barrier;
	barrier.image = image();
	barrier.oldLayout = currentLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = 0;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;

		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1,
			                   mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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

	if (vk.queueSubmit(vk.graphicsQueue(), mipmapCB.get()) != VK_SUCCESS)
		throw std::runtime_error("failed to submit mipmap command buffer");

	vk.wait(vk.graphicsQueue());
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
	if (rw.get() == nullptr)
		return;

	auto& vk = rw.get()->device();

	StagingBuffer stagingBuffer(*(rw.get()), texels, size);

	CommandBuffer cb(vk, rw.get()->commandPool());

	cb.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = initialLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image();
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels();
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
	cb.pipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	                   VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, barrier);

	cb.copyBufferToImage(stagingBuffer, image(),
	                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, region);

	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = finalLayout;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = 0;
	cb.pipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	                   VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, barrier);

	if (cb.end() != VK_SUCCESS)
		throw std::runtime_error("failed to record upload command buffer");

	if (vk.queueSubmit(vk.graphicsQueue(), cb.get()) != VK_SUCCESS)
		throw std::runtime_error("failed to submit upload command buffer");

	vk.wait(vk.graphicsQueue());
}

Buffer Texture2D::downloadDataToBufferImmediate(VkImageLayout currentLayout)
{
	return downloadDataToBufferImmediate(currentLayout, currentLayout);
}

Buffer Texture2D::downloadDataToBufferImmediate(VkImageLayout initialLayout,
                                                VkImageLayout finalLayout)
{
	if (rw.get() == nullptr || m_image.get() == nullptr)
		return {};

	auto& vk = rw.get()->device();

	VkDeviceSize dataSize = size();

	Buffer tmpBuffer(*(rw.get()), dataSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                 VMA_MEMORY_USAGE_GPU_TO_CPU,
	                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	VkBufferImageCopy copy{};
	copy.bufferRowLength = width();
	copy.bufferImageHeight = height();
	copy.imageExtent = extent3D();
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.imageSubresource.layerCount = 1;

	CommandBuffer cb(vk, rw.get()->commandPool());

	cb.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = initialLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image();
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels();
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	cb.pipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	                   VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, barrier);

	cb.copyImageToBuffer(image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	                     tmpBuffer, copy);

	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout = finalLayout;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	barrier.dstAccessMask = 0;
	cb.pipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	                   VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, barrier);

	if (cb.end() != VK_SUCCESS)
		throw std::runtime_error("failed to record upload command buffer");

	if (vk.queueSubmit(vk.graphicsQueue(), cb.get()) != VK_SUCCESS)
		throw std::runtime_error("failed to submit upload command buffer");

	vk.wait(vk.graphicsQueue());

	// return tmpBuffer.download();
	return tmpBuffer;
}
}  // namespace cdm
