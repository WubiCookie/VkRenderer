#include "ImageView.hpp"
#include "Image.hpp"

#include <stdexcept>

namespace cdm
{
ImageView::ImageView(const VulkanDevice& device_, Image& image_)
    : VulkanDeviceObject(device_),
      m_image(image_),
      m_format(m_image.get().format())
{
}

ImageView::ImageView(const VulkanDevice& device_, Image& image_,
                     VkFormat format)
    : VulkanDeviceObject(device_),
      m_image(image_),
      m_format(format)
{
}

ImageView::~ImageView() { device().destroy(m_imageView.get()); }

void ImageView::setImage(Image& image_)
{
	m_image = image_;
	m_format = image_.format();

	setCreationTime();
}

void ImageView::setImage(Image& image_, VkFormat format)
{
	m_image = image_;
	m_format = format;

	setCreationTime();
}

VkImageView ImageView::imageView()
{
	if (outdated())
		recreate();

	return m_imageView.get();
}

bool ImageView::outdated() const
{
	return m_image.get().creationTime() > creationTime();
}

void ImageView::recreate()
{
	auto& vk = device();

	vk.destroy(m_imageView);

	vk::ImageViewCreateInfo createInfo;
	createInfo.image = m_image.get().image();
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = m_format;
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	if (vk.create(createInfo, m_imageView.get()) != VK_SUCCESS)
	{
		throw std::runtime_error("error: failed to create image views");
	}

	setCreationTime();
}
}  // namespace cdm
