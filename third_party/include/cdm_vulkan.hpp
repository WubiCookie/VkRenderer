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

#include <string_view>
#include <sstream>
#include <cstring>
#include <cassert>

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
using BufferMemoryBarrier                  = CreateInfo<VkBufferMemoryBarrier,                  VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER>;
using BufferViewCreateInfo                 = CreateInfo<VkBufferViewCreateInfo,                 VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO>;
using CommandBufferAllocateInfo            = CreateInfo<VkCommandBufferAllocateInfo,            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO>;
using CommandBufferBeginInfo               = CreateInfo<VkCommandBufferBeginInfo,               VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO>;
using CommandPoolCreateInfo                = CreateInfo<VkCommandPoolCreateInfo,                VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO>;
//using ComputePipelineCreateInfo            = CreateInfo<VkComputePipelineCreateInfo,            VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO>;
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
using WriteDescriptorSet                   = CreateInfo<VkWriteDescriptorSet,                   VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET>;
using CopyDescriptorSet                    = CreateInfo<VkCopyDescriptorSet,                    VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET>;

struct ComputePipelineCreateInfo : VkComputePipelineCreateInfo
{
    using Type = VkComputePipelineCreateInfo;
    static constexpr VkStructureType GetStructureType() noexcept { return VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO; }

    constexpr ComputePipelineCreateInfo() noexcept : VkComputePipelineCreateInfo()
    {
        std::memset(this, 0, sizeof(*this));
        this->sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        this->stage = PipelineShaderStageCreateInfo();
    }
    ComputePipelineCreateInfo(const ComputePipelineCreateInfo&) = default;
    ComputePipelineCreateInfo(const VkComputePipelineCreateInfo& t) { std::memcpy(this, &t, sizeof(t)); };
    ComputePipelineCreateInfo(ComputePipelineCreateInfo&&) = default;
    ~ComputePipelineCreateInfo() = default;

    ComputePipelineCreateInfo& operator=(const ComputePipelineCreateInfo&) = default;
    ComputePipelineCreateInfo& operator=(ComputePipelineCreateInfo&&) = default;
};

struct Viewport : VkViewport
{
    // float    x;
    // float    y;
    // float    width;
    // float    height;
    // float    minDepth;
    // float    maxDepth;
    Viewport() : VkViewport{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f } {}
    Viewport(const Viewport&) = default;
    Viewport(Viewport&&) = default;
    Viewport& operator=(const Viewport&) = default;
    Viewport& operator=(Viewport&&) = default;
};

struct Rect2D : VkRect2D
{
    // VkOffset2D    offset;
    // VkExtent2D    extent;
    Rect2D() : VkRect2D{ { 0, 0 }, { 0, 0 } } {}
    Rect2D(const Rect2D&) = default;
    Rect2D(Rect2D&&) = default;
    Rect2D& operator=(const Rect2D&) = default;
    Rect2D& operator=(Rect2D&&) = default;
};

struct AttachmentDescription : VkAttachmentDescription
{
    // VkAttachmentDescriptionFlags    flags;
    // VkFormat                        format;
    // VkSampleCountFlagBits           samples;
    // VkAttachmentLoadOp              loadOp;
    // VkAttachmentStoreOp             storeOp;
    // VkAttachmentLoadOp              stencilLoadOp;
    // VkAttachmentStoreOp             stencilStoreOp;
    // VkImageLayout                   initialLayout;
    // VkImageLayout                   finalLayout;
    AttachmentDescription() : VkAttachmentDescription{
        VkAttachmentDescriptionFlags(),
        VK_FORMAT_UNDEFINED,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_UNDEFINED
    } {}
    AttachmentDescription(const AttachmentDescription&) = default;
    AttachmentDescription(AttachmentDescription&&) = default;
    AttachmentDescription& operator=(const AttachmentDescription&) = default;
    AttachmentDescription& operator=(AttachmentDescription&&) = default;
};

struct PipelineColorBlendAttachmentState : VkPipelineColorBlendAttachmentState
{
    // VkBool32                 blendEnable;
    // VkBlendFactor            srcColorBlendFactor;
    // VkBlendFactor            dstColorBlendFactor;
    // VkBlendOp                colorBlendOp;
    // VkBlendFactor            srcAlphaBlendFactor;
    // VkBlendFactor            dstAlphaBlendFactor;
    // VkBlendOp                alphaBlendOp;
    // VkColorComponentFlags    colorWriteMask;
    PipelineColorBlendAttachmentState() : VkPipelineColorBlendAttachmentState{
        false,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_OP_ADD,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_OP_ADD,
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
	    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    } {}

    PipelineColorBlendAttachmentState(bool defaultBlendEnabled) : PipelineColorBlendAttachmentState()
    {
        if (defaultBlendEnabled)
        {
            blendEnable = true;
		    srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		    dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        }
    }
    PipelineColorBlendAttachmentState(const PipelineColorBlendAttachmentState&) = default;
    PipelineColorBlendAttachmentState(PipelineColorBlendAttachmentState&&) = default;
    PipelineColorBlendAttachmentState& operator=(const PipelineColorBlendAttachmentState&) = default;
    PipelineColorBlendAttachmentState& operator=(PipelineColorBlendAttachmentState&&) = default;
};

struct SubpassDescription : VkSubpassDescription
{
    // VkSubpassDescriptionFlags       flags;
    // VkPipelineBindPoint             pipelineBindPoint;
    // uint32_t                        inputAttachmentCount;
    // const VkAttachmentReference*    pInputAttachments;
    // uint32_t                        colorAttachmentCount;
    // const VkAttachmentReference*    pColorAttachments;
    // const VkAttachmentReference*    pResolveAttachments;
    // const VkAttachmentReference*    pDepthStencilAttachment;
    // uint32_t                        preserveAttachmentCount;
    // const uint32_t*                 pPreserveAttachments;
    SubpassDescription() : VkSubpassDescription{
        VkSubpassDescriptionFlags(),
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        0,
        nullptr,
        0,
        nullptr,
        nullptr,
        nullptr,
        0,
        nullptr
    } {}
    SubpassDescription(const SubpassDescription&) = default;
    SubpassDescription(SubpassDescription&&) = default;
    SubpassDescription& operator=(const SubpassDescription&) = default;
    SubpassDescription& operator=(SubpassDescription&&) = default;
};

constexpr std::string_view result_to_string(VkResult result)
{
    switch (result)
    {
        case VK_SUCCESS: return "VK_SUCCESS";
        case VK_NOT_READY: return "VK_NOT_READY";
        case VK_TIMEOUT: return "VK_TIMEOUT";
        case VK_EVENT_SET: return "VK_EVENT_SET";
        case VK_EVENT_RESET: return "VK_EVENT_RESET";
        case VK_INCOMPLETE: return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
    }

    return "UNKNOWN";
}

inline std::string feature_to_string(VkFormatFeatureFlags features)
{
    std::stringstream ss;

    if ((VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT & features) == VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
        ss << "VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | ";
    if ((VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT & features) == VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)
        ss << "VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | ";
    if ((VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT & features) == VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT)
        ss << "VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT | ";
    if ((VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT & features) == VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT)
        ss << "VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT | ";
    if ((VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT & features) == VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT)
        ss << "VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT | ";
    if ((VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT & features) == VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT)
        ss << "VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT | ";
    if ((VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT & features) == VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT)
        ss << "VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT | ";
    if ((VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT & features) == VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
        ss << "VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | ";
    if ((VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT & features) == VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT)
        ss << "VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT | ";
    if ((VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT & features) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        ss << "VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | ";
    if ((VK_FORMAT_FEATURE_BLIT_SRC_BIT & features) == VK_FORMAT_FEATURE_BLIT_SRC_BIT)
        ss << "VK_FORMAT_FEATURE_BLIT_SRC_BIT | ";
    if ((VK_FORMAT_FEATURE_BLIT_DST_BIT & features) == VK_FORMAT_FEATURE_BLIT_DST_BIT)
        ss << "VK_FORMAT_FEATURE_BLIT_DST_BIT | ";
    if ((VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT & features) == VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
        ss << "VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT | ";
    if ((VK_FORMAT_FEATURE_TRANSFER_SRC_BIT & features) == VK_FORMAT_FEATURE_TRANSFER_SRC_BIT)
        ss << "VK_FORMAT_FEATURE_TRANSFER_SRC_BIT | ";
    if ((VK_FORMAT_FEATURE_TRANSFER_DST_BIT & features) == VK_FORMAT_FEATURE_TRANSFER_DST_BIT)
        ss << "VK_FORMAT_FEATURE_TRANSFER_DST_BIT | ";
    if ((VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT & features) == VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT)
        ss << "VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT | ";
    if ((VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT & features) == VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT)
        ss << "VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT | ";
    if ((VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_SEPARATE_RECONSTRUCTION_FILTER_BIT & features) == VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_SEPARATE_RECONSTRUCTION_FILTER_BIT)
        ss << "VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_SEPARATE_RECONSTRUCTION_FILTER_BIT | ";
    if ((VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_BIT & features) == VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_BIT)
        ss << "VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_BIT | ";
    if ((VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_FORCEABLE_BIT & features) == VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_FORCEABLE_BIT)
        ss << "VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_FORCEABLE_BIT | ";
    if ((VK_FORMAT_FEATURE_DISJOINT_BIT & features) == VK_FORMAT_FEATURE_DISJOINT_BIT)
        ss << "VK_FORMAT_FEATURE_DISJOINT_BIT | ";
    if ((VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT & features) == VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT)
        ss << "VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT | ";
    if ((VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_MINMAX_BIT & features) == VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_MINMAX_BIT)
        ss << "VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_MINMAX_BIT | ";
    if ((VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG & features) == VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG)
        ss << "VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG | ";
    if ((VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR & features) == VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR)
        ss << "VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR | ";
    if ((VK_FORMAT_FEATURE_FRAGMENT_DENSITY_MAP_BIT_EXT & features) == VK_FORMAT_FEATURE_FRAGMENT_DENSITY_MAP_BIT_EXT)
        ss << "VK_FORMAT_FEATURE_FRAGMENT_DENSITY_MAP_BIT_EXT | ";

    return ss.str();
}

inline std::string usage_to_string(VkImageUsageFlags usage)
{
    std::stringstream ss;

    if ((VK_IMAGE_USAGE_TRANSFER_SRC_BIT & usage) == VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
        ss << "VK_IMAGE_USAGE_TRANSFER_SRC_BIT | ";
    if ((VK_IMAGE_USAGE_TRANSFER_DST_BIT & usage) == VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        ss << "VK_IMAGE_USAGE_TRANSFER_DST_BIT | ";
    if ((VK_IMAGE_USAGE_SAMPLED_BIT & usage) == VK_IMAGE_USAGE_SAMPLED_BIT)
        ss << "VK_IMAGE_USAGE_SAMPLED_BIT | ";
    if ((VK_IMAGE_USAGE_STORAGE_BIT & usage) == VK_IMAGE_USAGE_STORAGE_BIT)
        ss << "VK_IMAGE_USAGE_STORAGE_BIT | ";
    if ((VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT & usage) == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        ss << "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | ";
    if ((VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT & usage) == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
        ss << "VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | ";
    if ((VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT & usage) == VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
        ss << "VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | ";
    if ((VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT & usage) == VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
        ss << "VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | ";
    if ((VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV & usage) == VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV)
        ss << "VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV | ";
    if ((VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT & usage) == VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT)
        ss << "VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT | ";

    return ss.str();
}

} // namespace vk

template<typename T, auto DefaultValue = nullptr>
class Movable
{
    T m_obj = DefaultValue;

public:
    Movable() = default;
    Movable(const Movable&) = default;
    Movable(Movable&& obj) noexcept : m_obj(std::exchange(obj.m_obj, DefaultValue)) {}
    Movable(T obj) noexcept : m_obj(obj) {}

    Movable& operator=(const Movable&) = default;
    Movable& operator=(Movable&& ptr) noexcept
    {
        std::swap(m_obj, ptr.m_obj);
        return *this;
    }

    void reset(T obj = DefaultValue) { m_obj = obj; }

    T& get() noexcept { return m_obj; }
    const T& get() const noexcept { return m_obj; }

    operator T& () noexcept { return get(); }
    operator const T& () const noexcept { return get(); }

    operator bool() const { return !!m_obj; }
};

template<typename T, auto DefaultValue = nullptr>
bool operator==(const Movable<T, DefaultValue>& lhs, bool rhs)
{
    return bool(lhs) == rhs;
}

template<typename T, auto DefaultValue = nullptr>
bool operator==(bool lhs, const Movable<T, DefaultValue>& rhs)
{
    return lhs == bool(rhs);
}

template<typename T, auto DefaultValue = nullptr>
bool operator!=(const Movable<T, DefaultValue>& lhs, bool rhs)
{
    return !(lhs == rhs);
}

template<typename T, auto DefaultValue = nullptr>
bool operator!=(bool lhs, const Movable<T, DefaultValue>& rhs)
{
    return !(lhs == rhs);
}

template<typename T, typename Owner, auto Deleter>
class Unique
{
    T m_obj = nullptr;
    const Owner* m_owner = nullptr;

public:
    Unique() = default;
    Unique(const Unique&) = delete;
    Unique(Unique&& obj) noexcept :
        m_obj(std::exchange(obj.m_obj, nullptr)),
        m_owner(std::exchange(obj.m_owner, nullptr))
    {}
    Unique(T obj, const Owner* owner) noexcept : m_obj(obj), m_owner(owner)
    {
        assert(m_obj);
        assert(m_owner);
    }
    Unique(T obj, const Owner& owner) noexcept : m_obj(obj), m_owner(&owner)
    {
        assert(m_obj);
    }
    ~Unique() { reset(); }

    Unique& operator=(const Unique&) = delete;
    Unique& operator=(Unique&& ptr) noexcept
    {
        std::swap(m_obj, ptr.m_obj);
        std::swap(m_owner, ptr.m_owner);
        return *this;
    }

    void reset(nullptr_t = nullptr)
    {
        reset(nullptr, nullptr);
    }

    void reset(T obj, const Owner* owner)
    {
        if (m_owner && m_obj)
            (m_owner->*Deleter)(m_obj);

        m_obj = obj;
        m_owner = owner;
    }

    void reset(T obj, const Owner& owner)
    {
        if (m_owner && m_obj)
            (m_owner->*Deleter)(m_obj);

        m_obj = obj;
        m_owner = &owner;
    }

    T release()
    {
        T res = m_obj;
        m_obj = nullptr;
        m_owner = nullptr;
        return res;
    }

    T& get() noexcept { return m_obj; }
    const T& get() const noexcept { return m_obj; }

    operator T& () noexcept { return get(); }
    operator const T& () const noexcept { return get(); }

    const Owner* owner() const { return m_owner; }

    operator bool() const { return !!m_obj; }
};

} // namespace cdm

#endif // CDM_VULKAN_HPP
