#pragma once

#include "VulkanDevice.hpp"

#include <vector>

namespace cdm
{
class Image;

class ImageView final : public VulkanDeviceObject
{
	std::reference_wrapper<Image> m_image;

	Movable<VkImageView> m_imageView = nullptr;

	VkFormat m_format;

public:
	ImageView(const VulkanDevice& device_, Image& image_);
	ImageView(const VulkanDevice& device_, Image& image_, VkFormat format);
	~ImageView();

	void setImage(Image& image_);
	void setImage(Image& image_, VkFormat format);

	VkImageView imageView();
	operator VkImageView() { return imageView(); }

private:
	bool outdated() const;
	void recreate();
};
}  // namespace cdm
