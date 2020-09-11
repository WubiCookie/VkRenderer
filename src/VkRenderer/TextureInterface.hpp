#pragma once

#include "VulkanDevice.hpp"

namespace cdm
{
class RenderWindow;

class TextureInterface
{
public:
	virtual uint32_t width() const = 0;
	virtual uint32_t height() const = 0;
	virtual uint32_t depth() const = 0;
	virtual VkExtent2D extent2D() const = 0;
	virtual VkExtent3D extent3D() const = 0;
	virtual VkDeviceSize size() const = 0;
	virtual VkDeviceSize offset() const = 0;
	virtual VkFormat format() const = 0;
	virtual uint32_t mipLevels() const = 0;
	virtual VkSampleCountFlagBits samples() const = 0;
	virtual VkDeviceMemory deviceMemory() const = 0;
	virtual VkImage& get() = 0;
	virtual const VkImage& get() const = 0;
	const VkImage& image() const { return get(); }
	operator VkImage&() { return get(); }
	operator const VkImage&() const { return get(); }
	virtual const VkImageView& view() const = 0;
	virtual const VkSampler& sampler() const = 0;
	virtual const VmaAllocation& allocation() const = 0;

	virtual void transitionLayoutImmediate(VkImageLayout initialLayout,
	                                       VkImageLayout finalLayout) = 0;
};
}  // namespace cdm
