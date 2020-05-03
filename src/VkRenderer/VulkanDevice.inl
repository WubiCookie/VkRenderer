#pragma once

#include "VulkanDevice.hpp"

namespace cdm
{
template <typename VkHandle>
VkResult VulkanDevice::debugMarkerSetObjectName(
    VkHandle object, VkDebugReportObjectTypeEXT objectType,
    std::string_view objectName) const
{
	vk::DebugMarkerObjectNameInfoEXT nameInfo;
	nameInfo.object = uint64_t(object);
	nameInfo.objectType = objectType;
	nameInfo.pObjectName = objectName.data();

	return debugMarkerSetObjectName(nameInfo);
}

template <typename VkHandle, typename T>
VkResult VulkanDevice::debugMarkerSetObjectTag(
    VkHandle object, VkDebugReportObjectTypeEXT objectType, uint64_t tagName,
    const T& tag) const
{
	vk::DebugMarkerObjectTagInfoEXT tagInfo;
	tagInfo.object = uint64_t(object);
	tagInfo.objectType = objectType;
	tagInfo.tagName = tagName;
	tagInfo.tagSize = sizeof(T);
	tagInfo.pTag = &tag;

	return debugMarkerSetObjectTag(tagInfo);
}

}  // namespace cdm
