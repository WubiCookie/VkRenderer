#pragma once

#include "TextureInterface.hpp"

namespace cdm
{
class RenderWindow;

class DepthTexture final : public TextureInterface
{
	Movable<RenderWindow*> rw;

	Movable<VmaAllocation> m_allocation;
	Movable<VkImage> m_image;
	UniqueImageView m_imageView;

	UniqueSampler m_sampler;

	Movable<VkDeviceMemory> m_deviceMemory;
	VkDeviceSize m_offset = 0;
	VkDeviceSize m_size = 0;

	uint32_t m_width = 0;
	uint32_t m_height = 0;
	VkFormat m_format = VK_FORMAT_UNDEFINED;
	uint32_t m_mipLevels = 0;
	VkSampleCountFlagBits m_samples = VK_SAMPLE_COUNT_1_BIT;

public:
	DepthTexture() = default;
	DepthTexture(RenderWindow& renderWindow,
	             VkImageUsageFlags usage = VkImageUsageFlags{},
	             VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
	             VkMemoryPropertyFlags requiredFlags =
	                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	             uint32_t mipLevels = 1,
	             VkSampleCountFlagBits samples =
	                 VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT);
	DepthTexture(RenderWindow& renderWindow, uint32_t imageWidth,
	             uint32_t imageHeight, VkFormat imageFormat,
	             VkImageTiling imageTiling, VkImageUsageFlags usage,
	             VmaMemoryUsage memoryUsage,
	             VkMemoryPropertyFlags requiredFlags, uint32_t mipLevels = 1,
	             VkSampleCountFlagBits samples =
	                 VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT);
	DepthTexture(const DepthTexture&) = delete;
	DepthTexture(DepthTexture&& texture) = default;
	~DepthTexture();

	DepthTexture& operator=(const DepthTexture&) = delete;
	DepthTexture& operator=(DepthTexture&& texture) = default;

	uint32_t width() const override { return m_width; }
	uint32_t height() const override { return m_height; }
	uint32_t depth() const override { return 1; }
	VkExtent2D extent2D() const override
	{
		return VkExtent2D{ width(), height() };
	};
	VkExtent3D extent3D() const override
	{
		return VkExtent3D{ width(), height(), depth() };
	};
	VkDeviceSize size() const override { return m_size; }
	VkDeviceSize offset() const override { return m_offset; }
	VkFormat format() const override { return m_format; }
	uint32_t mipLevels() const override { return m_mipLevels; }
	VkSampleCountFlagBits samples() const override { return m_samples; }
	VkDeviceMemory deviceMemory() const override
	{
		return m_deviceMemory.get();
	}
	VkImage& get() override { return m_image.get(); }
	const VkImage& get() const override { return m_image.get(); }
	const VkImageView& view() const override { return m_imageView.get(); }
	const VkSampler& sampler() const override { return m_sampler.get(); }
	const VmaAllocation& allocation() const override
	{
		return m_allocation.get();
	}

	void transitionLayoutImmediate(VkImageLayout initialLayout,
	                               VkImageLayout finalLayout) override;
	void generateMipmapsImmediate(VkImageLayout currentLayout);
};
}  // namespace cdm
