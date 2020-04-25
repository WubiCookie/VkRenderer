/* cdm_vulkan - v0.0 - vulkan helper library - https://github.com/WubiCookie/cdm
   no warranty implied; use at your own risk

LICENSE

	   DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
				   Version 2, December 2004

Copyright (C) 2019 Charles Seizilles de Mazancourt <charles DOT de DOT mazancourt AT hotmail DOT fr>

Everyone is permitted to copy and distribute verbatim or modified
copies of this license document, and changing it is allowed as long
as the name is changed.

		   DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
  TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

 0. You just DO WHAT THE FUCK YOU WANT TO.

CREDITS

Written by Charles Seizilles de Mazancourt
*/

#ifndef CDM_VULKAN_HPP
#define CDM_VULKAN_HPP

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include <cstring>

namespace cdm
{
namespace vk
{
template<typename T, VkStructureType StructureType>
struct CreateInfo : T
{
	using Type = T;
	static constexpr VkStructureType GetStructureType() noexcept { return StructureType; }

	constexpr CreateInfo() noexcept : T() { std::memset(this, 0, sizeof(*this)); this->sType = StructureType; };
	CreateInfo(const CreateInfo&) = default;
	CreateInfo(const T& t) { std::memcpy(this, &t, sizeof(t)); };
	CreateInfo(CreateInfo&&) = default;
	~CreateInfo() = default;

	CreateInfo& operator=(const CreateInfo&) = default;
	CreateInfo& operator=(CreateInfo&&) = default;
};

template<typename T, VkStructureType StructureType>
constexpr bool operator==(const CreateInfo<T, StructureType>& i1, const CreateInfo<T, StructureType>& i2) noexcept
{
	return std::memcmp(&i1, &i2, sizeof(T)) == 0;
}

template<typename T, VkStructureType StructureType>
constexpr bool operator==(const CreateInfo<T, StructureType>& i, const T& t) noexcept
{
	return std::memcmp(&i, &t, sizeof(T)) == 0;
}

template<typename T, VkStructureType StructureType>
constexpr bool operator==(const T& t, const CreateInfo<T, StructureType>& i) noexcept
{
	return std::memcmp(&t, &i, sizeof(T)) == 0;
}

using ApplicationInfo                      = CreateInfo<VkApplicationInfo,                      VK_STRUCTURE_TYPE_APPLICATION_INFO>;
using BufferCreateInfo                     = CreateInfo<VkBufferCreateInfo,                     VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO>;
using BufferViewCreateInfo                 = CreateInfo<VkBufferViewCreateInfo,                 VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO>;
using CommandBufferAllocateInfo            = CreateInfo<VkCommandBufferAllocateInfo,            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO>;
using CommandBufferBeginInfo               = CreateInfo<VkCommandBufferBeginInfo,               VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO>;
using CommandPoolCreateInfo                = CreateInfo<VkCommandPoolCreateInfo,                VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO>;
using ComputePipelineCreateInfo            = CreateInfo<VkComputePipelineCreateInfo,            VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO>;
using DebugReportCallbackCreateInfoEXT     = CreateInfo<VkDebugReportCallbackCreateInfoEXT,     VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT>;
using DebugUtilsMessengerCreateInfoEXT     = CreateInfo<VkDebugUtilsMessengerCreateInfoEXT,     VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT>;
using DescriptorPoolCreateInfo             = CreateInfo<VkDescriptorPoolCreateInfo,             VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO>;
using DescriptorSetAllocateInfo            = CreateInfo<VkDescriptorSetAllocateInfo,            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO>;
using DescriptorSetLayoutCreateInfo        = CreateInfo<VkDescriptorSetLayoutCreateInfo,        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO>;
using DescriptorUpdateTemplateCreateInfo   = CreateInfo<VkDescriptorUpdateTemplateCreateInfo,   VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO>;
using DeviceCreateInfo                     = CreateInfo<VkDeviceCreateInfo,                     VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO>;
using DeviceQueueCreateInfo                = CreateInfo<VkDeviceQueueCreateInfo,                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO>;
using EventCreateInfo                      = CreateInfo<VkEventCreateInfo,                      VK_STRUCTURE_TYPE_EVENT_CREATE_INFO>;
using FenceCreateInfo                      = CreateInfo<VkFenceCreateInfo,                      VK_STRUCTURE_TYPE_FENCE_CREATE_INFO>;
using FormatProperties2                    = CreateInfo<VkFormatProperties2,                    VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2>;
using FramebufferCreateInfo                = CreateInfo<VkFramebufferCreateInfo,                VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO>;
using GraphicsPipelineCreateInfo           = CreateInfo<VkGraphicsPipelineCreateInfo,           VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO>;
using ImageCreateInfo                      = CreateInfo<VkImageCreateInfo,                      VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO>;
using ImageMemoryBarrier                   = CreateInfo<VkImageMemoryBarrier,                   VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER>;
using ImageViewCreateInfo                  = CreateInfo<VkImageViewCreateInfo,                  VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO>;
using InstanceCreateInfo                   = CreateInfo<VkInstanceCreateInfo,                   VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO>;
using MemoryAllocateInfo                   = CreateInfo<VkMemoryAllocateInfo,                   VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO>;
using PhysicalDeviceFeatures2              = CreateInfo<VkPhysicalDeviceFeatures2,              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2>;
using PhysicalDeviceProperties2            = CreateInfo<VkPhysicalDeviceProperties2,            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2>;
using PhysicalDeviceSurfaceInfo2KHR        = CreateInfo<VkPhysicalDeviceSurfaceInfo2KHR,        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR>;
using PipelineCacheCreateInfo              = CreateInfo<VkPipelineCacheCreateInfo,              VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO>;
using PipelineColorBlendStateCreateInfo    = CreateInfo<VkPipelineColorBlendStateCreateInfo,    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO>;
using PipelineDepthStencilStateCreateInfo  = CreateInfo<VkPipelineDepthStencilStateCreateInfo,  VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO>;
using PipelineDynamicStateCreateInfo       = CreateInfo<VkPipelineDynamicStateCreateInfo,       VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO>;
using PipelineInputAssemblyStateCreateInfo = CreateInfo<VkPipelineInputAssemblyStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO>;
using PipelineLayoutCreateInfo             = CreateInfo<VkPipelineLayoutCreateInfo,             VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO>;
using PipelineMultisampleStateCreateInfo   = CreateInfo<VkPipelineMultisampleStateCreateInfo,   VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO>;
using PipelineRasterizationStateCreateInfo = CreateInfo<VkPipelineRasterizationStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO>;
using PipelineShaderStageCreateInfo        = CreateInfo<VkPipelineShaderStageCreateInfo,        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO>;
using PipelineVertexInputStateCreateInfo   = CreateInfo<VkPipelineVertexInputStateCreateInfo,   VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO>;
using PipelineViewportStateCreateInfo      = CreateInfo<VkPipelineViewportStateCreateInfo,      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO>;
using PresentInfoKHR                       = CreateInfo<VkPresentInfoKHR,                       VK_STRUCTURE_TYPE_PRESENT_INFO_KHR>;
using QueryPoolCreateInfo                  = CreateInfo<VkQueryPoolCreateInfo,                  VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO>;
using RenderPassBeginInfo                  = CreateInfo<VkRenderPassBeginInfo,                  VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO>;
using RenderPassCreateInfo                 = CreateInfo<VkRenderPassCreateInfo,                 VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO>;
using RenderPassCreateInfo2                = CreateInfo<VkRenderPassCreateInfo2,                VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2>;
using SamplerCreateInfo                    = CreateInfo<VkSamplerCreateInfo,                    VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO>;
using SamplerYcbcrConversionCreateInfo     = CreateInfo<VkSamplerYcbcrConversionCreateInfo,     VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO>;
using SemaphoreCreateInfo                  = CreateInfo<VkSemaphoreCreateInfo,                  VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO>;
using SemaphoreWaitInfo                    = CreateInfo<VkSemaphoreWaitInfo,                    VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO>;
using ShaderModuleCreateInfo               = CreateInfo<VkShaderModuleCreateInfo,               VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO>;
using SubmitInfo                           = CreateInfo<VkSubmitInfo,                           VK_STRUCTURE_TYPE_SUBMIT_INFO>;
using SurfaceCapabilities2KHR              = CreateInfo<VkSurfaceCapabilities2KHR,              VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR>;
using SwapchainCreateInfoKHR               = CreateInfo<VkSwapchainCreateInfoKHR,               VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR>;
using Win32SurfaceCreateInfoKHR            = CreateInfo<VkWin32SurfaceCreateInfoKHR,            VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR>;
using SubpassBeginInfo                     = CreateInfo<VkSubpassBeginInfo,                     VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO>;
using SubpassEndInfo                       = CreateInfo<VkSubpassEndInfo,                       VK_STRUCTURE_TYPE_SUBPASS_END_INFO>;
using DebugMarkerObjectTagInfoEXT          = CreateInfo<VkDebugMarkerObjectTagInfoEXT,          VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT>;
using DebugMarkerObjectNameInfoEXT         = CreateInfo<VkDebugMarkerObjectNameInfoEXT,         VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT>;
using DebugMarkerMarkerInfoEXT             = CreateInfo<VkDebugMarkerMarkerInfoEXT,             VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT>;

} // namespace vk

template<typename T, auto DefaultValue = nullptr>
class Moveable
{
    T m_obj = DefaultValue;

public:
    Moveable() = default;
    Moveable(const Moveable&) = default;
    Moveable(Moveable&& obj) noexcept : m_obj(std::exchange(obj.m_obj, DefaultValue)) {}
    Moveable(T obj) noexcept : m_obj(obj) {}

    Moveable& operator=(const Moveable&) = default;
    Moveable& operator=(Moveable&& ptr) noexcept
    {
        m_obj = std::exchange(ptr.m_obj, DefaultValue);
        return *this;
    }
    Moveable& operator=(T ptr) noexcept
    {
        m_obj = ptr;
        return *this;
    }

    T& get() noexcept { return m_obj; }
    const T& get() const noexcept { return m_obj; }

    //operator T&() noexcept { return m_obj; }
    //const operator T&() const noexcept { return m_obj; }

    //T* operator->() noexcept { return &m_obj; }
    //const T* operator->() const noexcept { return &m_obj; }

    //T* operator&() noexcept { return &m_obj; }
    //const T* operator&() const noexcept { return &m_obj; }
};

} // namespace cdm

#endif // CDM_VULKAN_HPP
