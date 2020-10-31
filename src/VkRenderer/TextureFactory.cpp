#include "TextureFactory.hpp"

#include "CommandBuffer.hpp"
#include "StagingBuffer.hpp"

#include <iostream>

namespace cdm
{
TextureFactory::TextureFactory(const VulkanDevice& vulkanDevice)
    : m_vulkanDevice(vulkanDevice)
{
	m_imageInfo.imageType = VK_IMAGE_TYPE_2D;
	m_imageInfo.extent.width = 1;
	m_imageInfo.extent.height = 1;
	m_imageInfo.extent.depth = 1;
	m_imageInfo.mipLevels = 1;
	m_imageInfo.arrayLayers = 1;
	m_imageInfo.format = VK_FORMAT_UNDEFINED;
	m_imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	m_imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	m_imageInfo.usage = 0;
	m_imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	m_imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	m_imageInfo.flags = 0;

	m_alloceInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	m_alloceInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	m_viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	m_viewInfo.format = VK_FORMAT_UNDEFINED;
	m_viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	m_viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	m_viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	m_viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	m_viewInfo.subresourceRange.aspectMask = 0;
	m_viewInfo.subresourceRange.baseMipLevel = 0;
	m_viewInfo.subresourceRange.levelCount = 1;
	m_viewInfo.subresourceRange.baseArrayLayer = 0;
	m_viewInfo.subresourceRange.layerCount = 1;

	m_samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	m_samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	m_samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	m_samplerInfo.minFilter = VK_FILTER_LINEAR;
	m_samplerInfo.magFilter = VK_FILTER_LINEAR;
	m_samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	m_samplerInfo.mipLodBias = 0.0f;
	m_samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
	m_samplerInfo.minLod = 0.0f;
	m_samplerInfo.maxLod = 1.0f;
}

void TextureFactory::setFormat(VkFormat format)
{
	m_imageInfo.format = format;
	m_viewInfo.format = format;
}

void TextureFactory::setWidth(uint32_t width)
{
	m_imageInfo.extent.width = width;
}

void TextureFactory::setHeight(uint32_t height)
{
	m_imageInfo.extent.height = height;
}

void TextureFactory::setDepth(uint32_t depth)
{
	m_imageInfo.extent.depth = depth;
}

void TextureFactory::setExtent(const VkExtent2D& extent)
{
	m_imageInfo.extent.width = extent.width;
	m_imageInfo.extent.height = extent.height;
	m_imageInfo.extent.depth = 1;
}

void TextureFactory::setExtent(const VkExtent3D& extent)
{
	m_imageInfo.extent = extent;
}

void TextureFactory::setTiling(VkImageTiling tiling)
{
	m_imageInfo.tiling = tiling;
}

void TextureFactory::setUsage(VkImageUsageFlags usage)
{
	m_imageInfo.usage = usage;
}

void TextureFactory::setMemoryUsage(VmaMemoryUsage memoryUsage)
{
	m_alloceInfo.usage = memoryUsage;
}

void TextureFactory::setRequieredMemoryProperties(
    VkMemoryPropertyFlags requiredFlags)
{
	m_alloceInfo.requiredFlags = requiredFlags;
}

void TextureFactory::setMipLevels(uint32_t mipLevels)
{
	m_imageInfo.mipLevels = mipLevels;
}

void TextureFactory::setSamples(VkSampleCountFlagBits samples)
{
	m_imageInfo.samples = samples;
}

void TextureFactory::setSharingMode(VkSharingMode sharingMode)
{
	m_imageInfo.sharingMode = sharingMode;
}

void TextureFactory::setViewComponents(const VkComponentMapping& components)
{
	m_viewInfo.components = components;
}

void TextureFactory::setViewSubresourceRange(
    const VkImageSubresourceRange& range)
{
	m_viewInfo.subresourceRange = range;
}

void TextureFactory::setMinFilter(VkFilter filter)
{
	m_samplerInfo.minFilter = filter;
}

void TextureFactory::setMagFilter(VkFilter filter)
{
	m_samplerInfo.magFilter = filter;
}

void TextureFactory::setFilters(VkFilter minFilter, VkFilter magFilter)
{
	setMinFilter(minFilter);
	setMagFilter(magFilter);
}

void TextureFactory::setAddressModeU(VkSamplerAddressMode addressMode)
{
	m_samplerInfo.addressModeU = addressMode;
}

void TextureFactory::setAddressModeV(VkSamplerAddressMode addressMode)
{
	m_samplerInfo.addressModeV = addressMode;
}

void TextureFactory::setAddressModeW(VkSamplerAddressMode addressMode)
{
	m_samplerInfo.addressModeW = addressMode;
}

void TextureFactory::setAddressModes(VkSamplerAddressMode addressModeU,
                                     VkSamplerAddressMode addressModeV,
                                     VkSamplerAddressMode addressModeW)
{
	setAddressModeU(addressModeU);
	setAddressModeV(addressModeV);
	setAddressModeW(addressModeW);
}

void TextureFactory::setMipmapMode(VkSamplerMipmapMode mipmapMode)
{
	m_samplerInfo.mipmapMode = mipmapMode;
}

void TextureFactory::setMipLodBias(float lodBias)
{
	m_samplerInfo.mipLodBias = lodBias;
}

void TextureFactory::setSamplerCompareEnable(bool enable)
{
	m_samplerInfo.compareEnable = enable;
}

void TextureFactory::setSamplerCompareOp(VkCompareOp compareOp)
{
	m_samplerInfo.compareOp = compareOp;
}

void TextureFactory::setMinLod(float minLod) { m_samplerInfo.minLod = minLod; }

void TextureFactory::setMaxLod(float maxLod) { m_samplerInfo.maxLod = maxLod; }

void TextureFactory::setLod(float minLod, float maxLod)
{
	setMinLod(minLod);
	setMaxLod(maxLod);
}

void TextureFactory::setAnisotropyEnable(bool enable)
{
	m_samplerInfo.anisotropyEnable = enable;
}

void TextureFactory::setMaxAnisotropy(float maxAnisotropy)
{
	m_samplerInfo.maxAnisotropy = maxAnisotropy;
}

void TextureFactory::setBorderColor(const VkBorderColor& borderColor)
{
	m_samplerInfo.borderColor = borderColor;
}

void TextureFactory::setUnormalizedCoordinated(bool unnormalized)
{
	m_samplerInfo.unnormalizedCoordinates = unnormalized;
}

//Texture1D TextureFactory::createTexture1D() { return Texture1D(); }

Texture2D TextureFactory::createTexture2D()
{
	return Texture2D(m_vulkanDevice, m_imageInfo, m_alloceInfo, m_viewInfo,
	                 m_samplerInfo);
}

//Cubemap TextureFactory::createCubemap() { return Cubemap(); }

//DepthTexture TextureFactory::createDepthTexture() { return DepthTexture(); }
}  // namespace cdm
