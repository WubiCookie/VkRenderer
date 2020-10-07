#include "ImageFactory.hpp"

#include <iostream>

namespace cdm
{
ImageFactory::ImageFactory(const VulkanDevice& vulkanDevice)
    : m_vulkanDevice(vulkanDevice)
{
}
}  // namespace cdm
