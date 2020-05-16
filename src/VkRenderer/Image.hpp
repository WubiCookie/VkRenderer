#pragma once

#include "VulkanDevice.hpp"

#include <vector>

namespace cdm
{
class AbstractImage : public VulkanDeviceObject
{
protected:
	VkFormat m_format;

public:
	AbstractImage(const VulkanDevice& device_, VkFormat format)
	    : VulkanDeviceObject(device_),
	      m_format(format)
	{
	}

	virtual VkImage image() = 0;
	virtual operator VkImage() = 0;

	virtual VkFormat format() { return m_format; }
};

class Image : public AbstractImage
{
	cdm::Movable<VkImage> m_image;

public:
	Image(const VulkanDevice& device_, VkFormat format);
	~Image();

	VkImage image() override;
	operator VkImage() override { return image(); }

private:
	bool outdated() const;
	void recreate();
};

class SwapchainImage final : public Image
{
	Movable<VkImage> m_image = nullptr;

public:
	SwapchainImage(const VulkanDevice& device_, VkImage image_,
	               VkFormat format);
	~SwapchainImage();

	void setImage(VkImage image_);
	void setFormat(VkFormat format_);

	VkImage image() override { return m_image.get(); }
	operator VkImage() override { return image(); }
};
}  // namespace cdm
