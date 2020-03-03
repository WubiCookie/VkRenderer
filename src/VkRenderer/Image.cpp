#include "Image.hpp"

#include <cassert>
#include <stdexcept>

namespace cdm
{
Image::Image(const VulkanDevice& device_, VkFormat format)
    : AbstractImage(device_, format)
{
}

Image::~Image() { device().destroy(m_image.get()); }

VkImage Image::image()
{
	if (outdated())
		recreate();

	return m_image.get();
}

bool Image::outdated() const
{
	assert(false && "not implemented yet");
	return true;
}

void Image::recreate()
{
	assert(false && "not implemented yet");

	setCreationTime();
}

SwapchainImage::SwapchainImage(const VulkanDevice& device_, VkImage image_,
                               VkFormat format)
    : Image(device_, format),
      m_image(image_)
{
	setCreationTime();
}

SwapchainImage::~SwapchainImage()
{
	// The parent class can not destroy the image if there's no image
	m_image = nullptr;
}

void SwapchainImage::setImage(VkImage image_)
{
	m_image = image_;

	setCreationTime();
}

void SwapchainImage::setFormat(VkFormat format_)
{
	m_format = format_;

	setCreationTime();
}
}  // namespace cdm
