#include "DepthTexture.hpp"

#include "CommandBuffer.hpp"
#include "RenderWindow.hpp"

#include <stdexcept>

namespace cdm
{
DepthTexture::DepthTexture(RenderWindow& renderWindow, VkImageUsageFlags usage,
                           VmaMemoryUsage memoryUsage,
                           VkMemoryPropertyFlags requiredFlags,
                           uint32_t mipLevels, VkSampleCountFlagBits samples)
    : DepthTexture(renderWindow, renderWindow.swapchainExtent().width,
                   renderWindow.swapchainExtent().height,
                   renderWindow.depthImageFormat(), VK_IMAGE_TILING_OPTIMAL,
                   usage | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                   memoryUsage, requiredFlags, mipLevels, samples)
{
}

DepthTexture::DepthTexture(RenderWindow& renderWindow, uint32_t imageWidth,
                           uint32_t imageHeight, VkFormat imageFormat,
                           VkImageTiling imageTiling, VkImageUsageFlags usage,
                           VmaMemoryUsage memoryUsage,
                           VkMemoryPropertyFlags requiredFlags,
                           uint32_t mipLevels, VkSampleCountFlagBits samples)
    : rw(&renderWindow)
{
	auto& vk = rw.get()->device();

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
	info.samples = samples;
	info.flags = 0;

	VmaAllocationCreateInfo imageAllocCreateInfo = {};
	imageAllocCreateInfo.usage = memoryUsage;
	imageAllocCreateInfo.requiredFlags = requiredFlags;

	VmaAllocationInfo allocInfo{};

	vmaCreateImage(vk.allocator(), &info, &imageAllocCreateInfo,
	               &m_image.get(), &m_allocation.get(), &allocInfo);

	if (m_image == false)
		throw std::runtime_error("could not create image");

	m_width = imageWidth;
	m_height = imageHeight;
	m_deviceMemory = allocInfo.deviceMemory;
	m_offset = allocInfo.offset;
	m_size = allocInfo.size;
	m_mipLevels = mipLevels;
	m_samples = samples;
	m_format = imageFormat;

	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = m_image.get();
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = imageFormat;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	m_imageView = vk.create(viewInfo);
	if (!m_imageView)
		throw std::runtime_error("could not create image view");

	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
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
}

DepthTexture::~DepthTexture()
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

void DepthTexture::transitionLayoutImmediate(VkImageLayout oldLayout,
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
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
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

void DepthTexture::generateMipmapsImmediate(VkImageLayout currentLayout)
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
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
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
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
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
}  // namespace cdm
