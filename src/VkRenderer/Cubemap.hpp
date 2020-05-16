#pragma once

#include "TextureInterface.hpp"

namespace cdm
{
class RenderWindow;

class Cubemap final : public TextureInterface
{
	Movable<RenderWindow*> rw;

	Movable<VmaAllocation> m_allocation;
	Movable<VkImage> m_image;
	UniqueImageView m_imageView;
	UniqueImageView m_imageViewFace0;
	UniqueImageView m_imageViewFace1;
	UniqueImageView m_imageViewFace2;
	UniqueImageView m_imageViewFace3;
	UniqueImageView m_imageViewFace4;
	UniqueImageView m_imageViewFace5;

	UniqueSampler m_sampler;

	Movable<VkDeviceMemory> m_deviceMemory;
	VkDeviceSize m_offset = 0;
	VkDeviceSize m_size = 0;

	uint32_t m_width = 0;
	uint32_t m_height = 0;
	VkFormat m_format = VK_FORMAT_UNDEFINED;
	uint32_t m_mipLevels = 0;

public:
	Cubemap() = default;
	Cubemap(RenderWindow& renderWindow, uint32_t imageWidth,
	        uint32_t imageHeight, VkFormat imageFormat,
	        VkImageTiling imageTiling, VkImageUsageFlags usage,
	        VmaMemoryUsage memoryUsage, VkMemoryPropertyFlags requiredFlags,
	        uint32_t mipLevels = 1);
	Cubemap(const Cubemap&) = delete;
	Cubemap(Cubemap&& texture) = default;
	~Cubemap();

	Cubemap& operator=(const Cubemap&) = delete;
	Cubemap& operator=(Cubemap&& texture) = default;

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
	VkImage& get() override { return m_image.get(); }
	const VkImage& get() const override { return m_image.get(); }
	const VkImageView& view() const override { return m_imageView.get(); }
	const VkImageView& viewFace0() const { return m_imageViewFace0.get(); }
	const VkImageView& viewFace1() const { return m_imageViewFace1.get(); }
	const VkImageView& viewFace2() const { return m_imageViewFace2.get(); }
	const VkImageView& viewFace3() const { return m_imageViewFace3.get(); }
	const VkImageView& viewFace4() const { return m_imageViewFace4.get(); }
	const VkImageView& viewFace5() const { return m_imageViewFace5.get(); }
	UniqueImageView createView(
	    const VkImageSubresourceRange& subresourceRange) const;
	const VkSampler& sampler() const override { return m_sampler.get(); }
	const VmaAllocation& allocation() const override
	{
		return m_allocation.get();
	}

	void transitionLayoutImmediate(VkImageLayout initialLayout,
	                               VkImageLayout finalLayout) override;
	void generateMipmapsImmediate(VkImageLayout currentLayout);
	void generateMipmapsImmediate(VkImageLayout initialLayout,
	                              VkImageLayout finalLayout);

	void uploadDataImmediate(const void* texels,
	                         const VkBufferImageCopy& region, uint32_t layer,
	                         VkImageLayout currentLayout);
	void uploadDataImmediate(const void* texels,
	                         const VkBufferImageCopy& region, uint32_t layer,
	                         VkImageLayout initialLayout,
	                         VkImageLayout finalLayout);
};
}  // namespace cdm
