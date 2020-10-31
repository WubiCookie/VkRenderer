#pragma once

#include "MyShaderWriter.hpp"
#include "VertexInputHelper.hpp"
#include "VulkanDevice.hpp"
#include "Texture1D.hpp"
#include "Texture2D.hpp"
#include "Cubemap.hpp"
#include "DepthTexture.hpp"

namespace cdm
{
class TextureFactory
{
	std::reference_wrapper<const VulkanDevice> m_vulkanDevice;

	vk::ImageCreateInfo m_imageInfo;
	VmaAllocationCreateInfo m_alloceInfo = {};
	vk::ImageViewCreateInfo m_viewInfo;
	vk::SamplerCreateInfo m_samplerInfo;

public:
	TextureFactory(const VulkanDevice& vulkanDevice);
	~TextureFactory() = default;

	TextureFactory(const TextureFactory&) = default;
	TextureFactory(TextureFactory&&) = default;

	TextureFactory& operator=(const TextureFactory&) = default;
	TextureFactory& operator=(TextureFactory&&) = default;

	void setFormat(VkFormat format);
	void setWidth(uint32_t width);
	void setHeight(uint32_t height);
	void setDepth(uint32_t depth);
	void setExtent(const VkExtent2D& extent);
	void setExtent(const VkExtent3D& extent);
	void setTiling(VkImageTiling tiling);
	void setUsage(VkImageUsageFlags usage);
	void setMemoryUsage(VmaMemoryUsage memoryUsage);
	void setRequieredMemoryProperties(VkMemoryPropertyFlags requiredFlags);
	void setMipLevels(uint32_t mipLevels);
	void setSamples(VkSampleCountFlagBits samples);
	void setSharingMode(VkSharingMode sharingMode);

	void setViewComponents(const VkComponentMapping& components);
	void setViewSubresourceRange(const VkImageSubresourceRange& range);

	void setMinFilter(VkFilter filter);
	void setMagFilter(VkFilter filter);
	void setFilters(VkFilter minFilter, VkFilter magFilter);
	void setAddressModeU(VkSamplerAddressMode addressMode);
	void setAddressModeV(VkSamplerAddressMode addressMode);
	void setAddressModeW(VkSamplerAddressMode addressMode);
	void setAddressModes(VkSamplerAddressMode addressModeU,
	                     VkSamplerAddressMode addressModeV,
	                     VkSamplerAddressMode addressModeW);
	void setMipmapMode(VkSamplerMipmapMode mipmapMode);
	void setMipLodBias(float lodBias);
	void setSamplerCompareEnable(bool enable);
	void setSamplerCompareOp(VkCompareOp compareOp);
	void setMinLod(float minLod);
	void setMaxLod(float maxLod);
	void setLod(float minLod, float maxLod);
	void setAnisotropyEnable(bool enable);
	void setMaxAnisotropy(float maxAnisotropy);
	void setBorderColor(const VkBorderColor& borderColor);
	void setUnormalizedCoordinated(bool unnormalized);

	//Texture1D createTexture1D();
	Texture2D createTexture2D();
	//Cubemap createCubemap();
	//DepthTexture createDepthTexture();
};
}  // namespace cdm
