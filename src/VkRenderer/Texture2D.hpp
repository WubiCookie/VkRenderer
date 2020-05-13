#pragma once

#include "TextureInterface.hpp"

namespace cdm
{
class RenderWindow;

class Texture2D final : TextureInterface
{
	Moveable<RenderWindow*> rw;

	Moveable<VmaAllocation> m_allocation;
	Moveable<VkImage> m_image;
	UniqueImageView m_imageView;

	UniqueSampler m_sampler;

	Moveable<VkDeviceMemory> m_deviceMemory;
	VkDeviceSize m_offset = 0;
	VkDeviceSize m_size = 0;

	uint32_t m_width = 0;
	uint32_t m_height = 0;
	VkFormat m_format = VK_FORMAT_UNDEFINED;
	uint32_t m_mipLevels = 0;

public:
	Texture2D() = default;
	Texture2D(RenderWindow& renderWindow, uint32_t imageWidth,
	          uint32_t imageHeight, VkFormat imageFormat,
	          VkImageTiling imageTiling, VkImageUsageFlags usage,
	          VmaMemoryUsage memoryUsage, VkMemoryPropertyFlags requiredFlags, uint32_t mipLevels = 1);
	Texture2D(const Texture2D&) = delete;
	Texture2D(Texture2D&& texture) noexcept;
	~Texture2D();

	Texture2D& operator=(const Texture2D&) = delete;
	Texture2D& operator=(Texture2D&& texture) noexcept;

	uint32_t width() const override { return m_width; }
	uint32_t height() const override { return m_height; }
	uint32_t depth() const override { return 1; }
	VkDeviceSize size() const override { return m_size; }
	VkDeviceSize offset() const override { return m_offset; }
	VkFormat format() const override { return m_format; }
	uint32_t mipLevels() const override { return m_mipLevels; }
	VkDeviceMemory deviceMemory() const override
	{
		return m_deviceMemory.get();
	}
	const VkImage& get() const override { return m_image.get(); }
	const VkImageView& view() const override { return m_imageView.get(); }
	const VkSampler& sampler() const override { return m_sampler.get(); }
	const VmaAllocation& allocation() const override
	{
		return m_allocation.get();
	}

	void transitionLayoutImmediate(VkImageLayout initialLayout,
	                              VkImageLayout finalLayout) override;
	void generateMipmapsImmediate(VkImageLayout currentLayout) override;
};
}  // namespace cdm
