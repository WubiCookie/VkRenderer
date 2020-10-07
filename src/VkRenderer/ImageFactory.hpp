#pragma once

#include "MyShaderWriter.hpp"
#include "VertexInputHelper.hpp"
#include "VulkanDevice.hpp"

namespace cdm
{
class ImageFactory
{
	std::reference_wrapper<const VulkanDevice> m_vulkanDevice;

public:
	ImageFactory(const VulkanDevice& vulkanDevice);
	~ImageFactory() = default;

	ImageFactory(const ImageFactory&) = default;
	ImageFactory(ImageFactory&&) = default;

	ImageFactory& operator=(const ImageFactory&) = default;
	ImageFactory& operator=(ImageFactory&&) = default;
};
}  // namespace cdm
