#include "Cubemap.hpp"

#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include "CommandBufferPool.hpp"
#include "RenderWindow.hpp"
#include "StagingBuffer.hpp"

#include <stdexcept>

namespace cdm
{
Cubemap::Cubemap(RenderWindow& renderWindow, uint32_t imageWidth,
                 uint32_t imageHeight, VkFormat imageFormat,
                 VkImageTiling imageTiling, VkImageUsageFlags usage,
                 VmaMemoryUsage memoryUsage,
                 VkMemoryPropertyFlags requiredFlags, uint32_t mipLevels,
                 VkSampleCountFlagBits samples)
    : rw(&renderWindow)
{
	auto& vk = rw.get()->device();

#pragma region image
	mipLevels = std::min(mipLevels, static_cast<uint32_t>(std::floor(std::log2(
	                                    std::max(imageWidth, imageHeight)))) +
	                                    1);

	vk::ImageCreateInfo info;
	info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.extent.width = imageWidth;
	info.extent.height = imageHeight;
	info.extent.depth = 1;
	info.mipLevels = mipLevels;
	info.arrayLayers = 6;
	info.format = imageFormat;
	info.tiling = imageTiling;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.usage = usage;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.samples = VK_SAMPLE_COUNT_1_BIT;

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

#pragma region views
	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = m_image.get();
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	viewInfo.format = imageFormat;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 6;

	m_imageView = vk.create(viewInfo);
	if (!m_imageView)
		throw std::runtime_error("could not create image view");

	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.subresourceRange.layerCount = 1;
	uint32_t baseArrayLayer = 0;
	for (UniqueImageView& view :
	     { std::ref(m_imageViewFace0), std::ref(m_imageViewFace1),
	       std::ref(m_imageViewFace2), std::ref(m_imageViewFace3),
	       std::ref(m_imageViewFace4), std::ref(m_imageViewFace5) })
	{
		viewInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
		view = vk.create(viewInfo);
		if (!view)
			throw std::runtime_error("could not create image view");

		baseArrayLayer++;
	}
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

Cubemap::~Cubemap()
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

UniqueImageView Cubemap::createView(
    const VkImageSubresourceRange& subresourceRange) const
{
	auto& vk = rw.get()->device();

	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = m_image.get();
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	viewInfo.format = m_format;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.subresourceRange = subresourceRange;

	return vk.create(viewInfo);
}

UniqueImageView Cubemap::createView2D(
    const VkImageSubresourceRange& subresourceRange) const
{
	auto& vk = rw.get()->device();

	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = m_image.get();
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = m_format;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.subresourceRange = subresourceRange;

	return vk.create(viewInfo);
}

void Cubemap::transitionLayoutImmediate(VkImageLayout oldLayout,
                                        VkImageLayout newLayout)
{
	if (rw.get() == nullptr)
		return;

	auto& vk = rw.get()->device();

	// CommandBuffer transitionCB(vk, rw.get()->commandPool());
	CommandBufferPool pool(vk, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	auto& frame = pool.getAvailableCommandBuffer();
	frame.reset();
	CommandBuffer& transitionCB = frame.commandBuffer;

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
	barrier.subresourceRange.layerCount = 6;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;
	transitionCB.pipelineBarrier(  // VK_PIPELINE_STAGE_TRANSFER_BIT,
	                               // VK_PIPELINE_STAGE_TRANSFER_BIT,
	    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
	    // VK_DEPENDENCY_DEVICE_GROUP_BIT, barrier);
	    0,
	    // VK_DEPENDENCY_BY_REGION_BIT,
	    barrier);
	if (transitionCB.end() != VK_SUCCESS)
		throw std::runtime_error("failed to record transition command buffer");

	VkResult submitRes =
	    vk.queueSubmit(vk.graphicsQueue(), transitionCB.get(), frame.fence);
	if (submitRes != VK_SUCCESS)
		throw std::runtime_error(
		    std::string("failed to submit transition command buffer ") +
		    std::string(vk::result_to_string(submitRes)));

	// vk.wait(vk.graphicsQueue());
	vk.wait(frame.fence);
	frame.reset();
}

void Cubemap::generateMipmapsImmediate(VkImageLayout currentLayout)
{
	generateMipmapsImmediate(currentLayout, currentLayout);
}

void Cubemap::generateMipmapsImmediate(VkImageLayout initialLayout,
                                       VkImageLayout finalLayout)
{
	if (rw.get() == nullptr)
		return;
	if (m_mipLevels <= 1)
		return;

	auto& vk = rw.get()->device();

	// CommandBuffer mipmapCB(vk, rw.get()->commandPool());
	auto& frame = rw.get()->getAvailableCommandBuffer();
	frame.reset();
	CommandBuffer& mipmapCB = frame.commandBuffer;

	mipmapCB.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	mipmapCB.debugMarkerBegin("mipmap generation");
	vk::ImageMemoryBarrier barrier;
	barrier.image = image();
	barrier.oldLayout = initialLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 6;
	// barrier.srcAccessMask = 0;
	barrier.dstAccessMask =
	    VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
	barrier.srcAccessMask = 0;
	// barrier.dstAccessMask = 0;
	mipmapCB.pipelineBarrier(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
	                         VK_PIPELINE_STAGE_TRANSFER_BIT,
	                         // VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                         0, barrier);

	int32_t mipWidth = int32_t(m_width);
	int32_t mipHeight = int32_t(m_height);

	for (uint32_t i = 1; i < m_mipLevels; i++)
	{
		barrier.subresourceRange.baseMipLevel = i;
		barrier.oldLayout = initialLayout;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcAccessMask =
		    VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask =
		    VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
		mipmapCB.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
		                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { int32_t(m_width), int32_t(m_height), 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = 0;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 6;

		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1,
			                   mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 6;

		mipmapCB.blitImage(image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		                   image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, blit,
		                   VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = finalLayout;
		barrier.srcAccessMask =
		    VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask =
		    VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
		mipmapCB.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
		                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);

		if (mipWidth > 1)
			mipWidth /= 2;
		if (mipHeight > 1)
			mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout = finalLayout;
	barrier.srcAccessMask =
	    VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
	// barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	// barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;
	mipmapCB.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
	                         // VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                         // VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
	                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, barrier);

	mipmapCB.debugMarkerEnd();
	if (mipmapCB.end() != VK_SUCCESS)
		throw std::runtime_error("failed to record mipmap command buffer");

	//if (vk.queueSubmit(vk.graphicsQueue(), mipmapCB.get(), frame.fence) != VK_SUCCESS)
	if (frame.submit(vk.graphicsQueue()) != VK_SUCCESS)
		throw std::runtime_error("failed to submit mipmap command buffer");

	// vk.wait(vk.graphicsQueue());
	//vk.wait(frame.fence);
	//frame.reset();
}

void Cubemap::uploadDataImmediate(const void* texels, size_t size,
                                  const VkBufferImageCopy& region,
                                  uint32_t layer, VkImageLayout currentLayout)
{
	uploadDataImmediate(texels, size, region, layer, currentLayout,
	                    currentLayout);
}

void Cubemap::uploadDataImmediate(const void* texels, size_t size,
                                  const VkBufferImageCopy& region,
                                  uint32_t layer, VkImageLayout initialLayout,
                                  VkImageLayout finalLayout)
{
	if (rw.get() == nullptr)
		return;

	auto& vk = rw.get()->device();

	StagingBuffer stagingBuffer(*(rw.get()), texels, size);

	auto& frame = rw.get()->getAvailableCommandBuffer();
	frame.reset();
	CommandBuffer& cb = frame.commandBuffer;

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
	barrier.subresourceRange.baseArrayLayer = layer;
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

	if (vk.queueSubmit(vk.graphicsQueue(), cb.get(), frame.fence) !=
	    VK_SUCCESS)
		throw std::runtime_error("failed to submit upload command buffer");

	// vk.wait(vk.graphicsQueue());
	vk.wait(frame.fence);
	frame.reset();
}

Buffer Cubemap::downloadDataToBufferImmediate(uint32_t layer,
                                              uint32_t mipLevel,
                                              VkImageLayout currentLayout)
{
	return downloadDataToBufferImmediate(layer, mipLevel, currentLayout,
	                                     currentLayout);
}

Buffer Cubemap::downloadDataToBufferImmediate(uint32_t layer,
                                              uint32_t mipLevel,
                                              VkImageLayout initialLayout,
                                              VkImageLayout finalLayout)
{
	if (rw.get() == nullptr || m_image.get() == nullptr)
		return {};

	auto& vk = rw.get()->device();

	LogRRID log(vk);

	VkDeviceSize dataSize = size();

	Buffer tmpBuffer(*(rw.get()), dataSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                 VMA_MEMORY_USAGE_GPU_TO_CPU,
	                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	VkBufferImageCopy copy{};
	copy.bufferRowLength = width() * std::pow(0.5, mipLevel);
	copy.bufferImageHeight = height() * std::pow(0.5, mipLevel);
	copy.imageExtent = extent3D();
	copy.imageExtent.width *= std::pow(0.5, mipLevel);
	copy.imageExtent.height *= std::pow(0.5, mipLevel);
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.imageSubresource.baseArrayLayer = layer;
	copy.imageSubresource.layerCount = 1;
	copy.imageSubresource.mipLevel = mipLevel;

	// CommandBuffer cb(vk, rw.get()->commandPool());
	auto& frame = rw.get()->getAvailableCommandBuffer();
	frame.reset();
	CommandBuffer& cb = frame.commandBuffer;

	cb.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = initialLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image();
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = mipLevel;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = layer;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	cb.pipelineBarrier(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
	                   VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);

	cb.copyImageToBuffer(image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	                     tmpBuffer, copy);

	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout = finalLayout;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	barrier.dstAccessMask = 0;
	cb.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
	                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, barrier);

	if (cb.end() != VK_SUCCESS)
		throw std::runtime_error("failed to record upload command buffer");

	if (vk.queueSubmit(vk.graphicsQueue(), cb.get(), frame.fence) !=
	    VK_SUCCESS)
		throw std::runtime_error("failed to submit upload command buffer");

	// vk.wait(vk.graphicsQueue());
	vk.wait(frame.fence);
	frame.reset();

	// return tmpBuffer.download();
	return tmpBuffer;
}
}  // namespace cdm
