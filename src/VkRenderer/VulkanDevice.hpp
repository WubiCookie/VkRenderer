#pragma once

//#include "VulkanFunctions.hpp"

#define NOCOMM
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <libloaderapi.h>

#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include "cdm_vulkan.hpp"
#include "vk_mem_alloc.h"

#include <initializer_list>
#include <optional>
#include <string_view>
#include <utility>

#define VULKAN_BASE_FUNCTIONS_VISIBILITY protected
#define VULKAN_FUNCTIONS_VISIBILITY private

namespace cdm
{
struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

class VulkanDeviceBase
{
	VkInstance m_instance = nullptr;
	VkDebugReportCallbackEXT m_callback = nullptr;
	VkDebugUtilsMessengerEXT m_messenger = nullptr;

protected:
	// VulkanFunctions m_vk;
	bool m_layers;

	inline static size_t LogActive = 0;

public:
	inline static void setLogActive() { LogActive++; }
	inline static void setLogInactive()
	{
		if (LogActive != 0)
			LogActive--;
	}

	VulkanDeviceBase(bool layers = false) noexcept;
	virtual ~VulkanDeviceBase();

	VkInstance instance() const { return m_instance; }

	HMODULE VulkanLibrary;

	// clang-format off
	PFN_vkGetInstanceProcAddr GetInstanceProcAddr;

	// Global functions
	PFN_vkCreateInstance CreateInstance;
	PFN_vkEnumerateInstanceExtensionProperties EnumerateInstanceExtensionProperties;
	PFN_vkEnumerateInstanceLayerProperties EnumerateInstanceLayerProperties;

	// Instance functions
	PFN_vkCreateDevice CreateDevice;
	PFN_vkDestroyInstance DestroyInstance;
	PFN_vkEnumerateDeviceExtensionProperties EnumerateDeviceExtensionProperties;
	PFN_vkEnumerateDeviceLayerProperties EnumerateDeviceLayerProperties;
	PFN_vkEnumeratePhysicalDeviceGroups EnumeratePhysicalDeviceGroups;
	PFN_vkEnumeratePhysicalDevices EnumeratePhysicalDevices;
	PFN_vkGetDeviceProcAddr GetDeviceProcAddr;
	PFN_vkGetPhysicalDeviceExternalBufferProperties GetPhysicalDeviceExternalBufferProperties;
	PFN_vkGetPhysicalDeviceExternalFenceProperties GetPhysicalDeviceExternalFenceProperties;
	PFN_vkGetPhysicalDeviceExternalSemaphoreProperties GetPhysicalDeviceExternalSemaphoreProperties;
	PFN_vkGetPhysicalDeviceFeatures GetPhysicalDeviceFeatures;
	PFN_vkGetPhysicalDeviceFeatures2 GetPhysicalDeviceFeatures2;
	PFN_vkGetPhysicalDeviceFormatProperties GetPhysicalDeviceFormatProperties;
	PFN_vkGetPhysicalDeviceFormatProperties2 GetPhysicalDeviceFormatProperties2;
	PFN_vkGetPhysicalDeviceImageFormatProperties GetPhysicalDeviceImageFormatProperties;
	PFN_vkGetPhysicalDeviceImageFormatProperties2 GetPhysicalDeviceImageFormatProperties2;
	PFN_vkGetPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties;
	PFN_vkGetPhysicalDeviceMemoryProperties2 GetPhysicalDeviceMemoryProperties2;
	PFN_vkGetPhysicalDeviceProperties GetPhysicalDeviceProperties;
	PFN_vkGetPhysicalDeviceProperties2 GetPhysicalDeviceProperties2;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties GetPhysicalDeviceQueueFamilyProperties;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties2 GetPhysicalDeviceQueueFamilyProperties2;
	PFN_vkGetPhysicalDeviceSparseImageFormatProperties GetPhysicalDeviceSparseImageFormatProperties;
	PFN_vkGetPhysicalDeviceSparseImageFormatProperties2 GetPhysicalDeviceSparseImageFormatProperties2;

	// VK_KHR_SURFACE_EXTENSION
	PFN_vkDestroySurfaceKHR DestroySurfaceKHR;
public:
	void destroySurface(VkSurfaceKHR surface) const;
	void destroy(VkSurfaceKHR surface) const;

	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR GetPhysicalDeviceSurfaceCapabilitiesKHR;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR GetPhysicalDeviceSurfaceFormatsKHR;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR GetPhysicalDeviceSurfacePresentModesKHR;
	PFN_vkGetPhysicalDeviceSurfaceSupportKHR GetPhysicalDeviceSurfaceSupportKHR;

	// VK_KHR_WIN32_SURFACE_EXTENSION_NAME
	PFN_vkCreateWin32SurfaceKHR CreateWin32SurfaceKHR;
public:
	VkResult createSurface(const vk::Win32SurfaceCreateInfoKHR& createInfo, VkSurfaceKHR& outSurface) const;
	VkResult create(const vk::Win32SurfaceCreateInfoKHR& createInfo, VkSurfaceKHR& outSurface) const;

	PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR GetPhysicalDeviceWin32PresentationSupportKHR;

	// VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME
	PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR GetPhysicalDeviceSurfaceCapabilities2KHR;
	PFN_vkGetPhysicalDeviceSurfaceFormats2KHR GetPhysicalDeviceSurfaceFormats2KHR;

	// VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
	PFN_vkGetPhysicalDeviceFeatures2KHR GetPhysicalDeviceFeatures2KHR;
	PFN_vkGetPhysicalDeviceFormatProperties2KHR GetPhysicalDeviceFormatProperties2KHR;
	PFN_vkGetPhysicalDeviceImageFormatProperties2KHR GetPhysicalDeviceImageFormatProperties2KHR;
	PFN_vkGetPhysicalDeviceMemoryProperties2KHR GetPhysicalDeviceMemoryProperties2KHR;
	PFN_vkGetPhysicalDeviceProperties2KHR GetPhysicalDeviceProperties2KHR;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR GetPhysicalDeviceQueueFamilyProperties2KHR;
	PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR GetPhysicalDeviceSparseImageFormatProperties2KHR;

	// VK_EXT_DEBUG_REPORT_EXTENSION_NAME
	PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallbackEXT;
	PFN_vkDebugReportMessageEXT DebugReportMessageEXT;
	PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallbackEXT;

	// VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	PFN_vkCreateDebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT;
	PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessengerEXT;
	PFN_vkSubmitDebugUtilsMessageEXT SubmitDebugUtilsMessageEXT;
	// clang-format on

private:
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(
	    VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
	    uint64_t object, size_t location, int32_t messageCode,
	    const char* pLayerPrefix, const char* pMessage, void* pUserData);

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessengerCallback(
	    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	    void* pUserData);
};

class VulkanDeviceDestroyer : public VulkanDeviceBase
{
protected:
	VkPhysicalDevice m_physicalDevice = nullptr;
	VkDevice m_device = nullptr;
	VkQueue m_graphicsQueue = nullptr;
	VkQueue m_presentQueue = nullptr;
	QueueFamilyIndices m_queueFamilyIndices;

	Movable<VmaAllocator> m_allocator = nullptr;

public:
	VulkanDeviceDestroyer(bool layers = false) noexcept;
	~VulkanDeviceDestroyer() override;

	void createDevice(VkSurfaceKHR surface,
	                  QueueFamilyIndices queueFamilyIndices);

	VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
	VkDevice vkDevice() const { return m_device; }
	VkQueue graphicsQueue() const { return m_graphicsQueue; }
	VkQueue presentQueue() const { return m_presentQueue; }
	QueueFamilyIndices queueFamilyIndices() const
	{
		return m_queueFamilyIndices;
	}
	VmaAllocator allocator() const { return m_allocator.get(); }

	using VulkanDeviceBase::create;
	using VulkanDeviceBase::createSurface;
	using VulkanDeviceBase::destroy;
	using VulkanDeviceBase::destroySurface;

	// clang-format off
	VkFormatProperties getPhysicalDeviceFormatProperties(VkFormat format) const;

	PFN_vkCreateHeadlessSurfaceEXT CreateHeadlessSurfaceEXT;
	PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT GetPhysicalDeviceCalibrateableTimeDomainsEXT;
	PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT GetPhysicalDeviceMultisamplePropertiesEXT;
	PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT GetPhysicalDeviceSurfaceCapabilities2EXT;
	PFN_vkGetPhysicalDeviceToolPropertiesEXT GetPhysicalDeviceToolPropertiesEXT;
	PFN_vkReleaseDisplayEXT ReleaseDisplayEXT;

	PFN_vkCreateDisplayModeKHR CreateDisplayModeKHR;
	PFN_vkCreateDisplayPlaneSurfaceKHR CreateDisplayPlaneSurfaceKHR;
	PFN_vkEnumeratePhysicalDeviceGroupsKHR EnumeratePhysicalDeviceGroupsKHR;
	PFN_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR EnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR;
	PFN_vkGetDisplayModeProperties2KHR GetDisplayModeProperties2KHR;
	PFN_vkGetDisplayModePropertiesKHR GetDisplayModePropertiesKHR;
	PFN_vkGetDisplayPlaneCapabilities2KHR GetDisplayPlaneCapabilities2KHR;
	PFN_vkGetDisplayPlaneCapabilitiesKHR GetDisplayPlaneCapabilitiesKHR;
	PFN_vkGetDisplayPlaneSupportedDisplaysKHR GetDisplayPlaneSupportedDisplaysKHR;
	PFN_vkGetPhysicalDeviceDisplayPlaneProperties2KHR GetPhysicalDeviceDisplayPlaneProperties2KHR;
	PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR GetPhysicalDeviceDisplayPlanePropertiesKHR;
	PFN_vkGetPhysicalDeviceDisplayProperties2KHR GetPhysicalDeviceDisplayProperties2KHR;
	PFN_vkGetPhysicalDeviceDisplayPropertiesKHR GetPhysicalDeviceDisplayPropertiesKHR;
	PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR GetPhysicalDeviceExternalBufferPropertiesKHR;
	PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR GetPhysicalDeviceExternalFencePropertiesKHR;
	PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR GetPhysicalDeviceExternalSemaphorePropertiesKHR;
	PFN_vkGetPhysicalDevicePresentRectanglesKHR GetPhysicalDevicePresentRectanglesKHR;
	PFN_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR GetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR;

	PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV GetPhysicalDeviceCooperativeMatrixPropertiesNV;
	PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV GetPhysicalDeviceExternalImageFormatPropertiesNV;
	PFN_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV GetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV;
	
	PFN_vkCmdSetCheckpointNV CmdSetCheckpointNV;
	PFN_vkGetQueueCheckpointDataNV GetQueueCheckpointDataNV;

	// PFN_vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX
	// GetPhysicalDeviceGeneratedCommandsPropertiesNVX;

	// Device functions

	PFN_vkAllocateCommandBuffers AllocateCommandBuffers;
public:
	VkResult allocateCommandBuffers(const vk::CommandBufferAllocateInfo& allocateInfo, VkCommandBuffer* pCommandBuffers) const;
	VkResult allocate(const vk::CommandBufferAllocateInfo& allocateInfo, VkCommandBuffer* pCommandBuffers) const;

	PFN_vkAllocateDescriptorSets AllocateDescriptorSets;
public:
	VkResult allocateDescriptorSets(const vk::DescriptorSetAllocateInfo& allocateInfo, VkDescriptorSet* pDescriptorSets) const;
	VkResult allocate(const vk::DescriptorSetAllocateInfo& allocateInfo, VkDescriptorSet* pDescriptorSets) const;
	VkDescriptorSet allocate(VkDescriptorPool pool, VkDescriptorSetLayout layout) const;

	PFN_vkAllocateMemory AllocateMemory;
public:
	VkResult allocateMemory(const vk::MemoryAllocateInfo& allocateInfo, VkDeviceMemory& outDeviceMemory) const;
	VkResult allocate(const vk::MemoryAllocateInfo& allocateInfo, VkDeviceMemory& outDeviceMemory) const;

	PFN_vkBindBufferMemory            BindBufferMemory;
	PFN_vkBindBufferMemory2           BindBufferMemory2;
	PFN_vkBindImageMemory             BindImageMemory;
	PFN_vkBindImageMemory2            BindImageMemory2;

	PFN_vkBeginCommandBuffer          BeginCommandBuffer;
	PFN_vkCmdBeginQuery               CmdBeginQuery;
	PFN_vkCmdBeginRenderPass          CmdBeginRenderPass;
	PFN_vkCmdBeginRenderPass2         CmdBeginRenderPass2;
	PFN_vkCmdBindDescriptorSets       CmdBindDescriptorSets;
	PFN_vkCmdBindIndexBuffer          CmdBindIndexBuffer;
	PFN_vkCmdBindPipeline             CmdBindPipeline;
	PFN_vkCmdBindVertexBuffers        CmdBindVertexBuffers;
	PFN_vkCmdBlitImage                CmdBlitImage;
	PFN_vkCmdClearAttachments         CmdClearAttachments;
	PFN_vkCmdClearColorImage          CmdClearColorImage;
	PFN_vkCmdClearDepthStencilImage   CmdClearDepthStencilImage;
	PFN_vkCmdCopyBuffer               CmdCopyBuffer;
	PFN_vkCmdCopyBufferToImage        CmdCopyBufferToImage;
	PFN_vkCmdCopyImage                CmdCopyImage;
	PFN_vkCmdCopyImageToBuffer        CmdCopyImageToBuffer;
	PFN_vkCmdCopyQueryPoolResults     CmdCopyQueryPoolResults;
	PFN_vkCmdDispatch                 CmdDispatch;
	PFN_vkCmdDispatchBase             CmdDispatchBase;
	PFN_vkCmdDispatchIndirect         CmdDispatchIndirect;
	PFN_vkCmdDraw                     CmdDraw;
	PFN_vkCmdDrawIndexed              CmdDrawIndexed;
	PFN_vkCmdDrawIndexedIndirect      CmdDrawIndexedIndirect;
	PFN_vkCmdDrawIndexedIndirectCount CmdDrawIndexedIndirectCount;
	PFN_vkCmdDrawIndirect             CmdDrawIndirect;
	PFN_vkCmdDrawIndirectCount        CmdDrawIndirectCount;
	PFN_vkCmdEndQuery                 CmdEndQuery;
	PFN_vkCmdEndRenderPass            CmdEndRenderPass;
	PFN_vkCmdEndRenderPass2           CmdEndRenderPass2;
	PFN_vkCmdExecuteCommands          CmdExecuteCommands;
	PFN_vkCmdFillBuffer               CmdFillBuffer;
	PFN_vkCmdNextSubpass              CmdNextSubpass;
	PFN_vkCmdNextSubpass2             CmdNextSubpass2;
	PFN_vkCmdPipelineBarrier          CmdPipelineBarrier;
	PFN_vkCmdPushConstants            CmdPushConstants;
	PFN_vkCmdResetEvent               CmdResetEvent;
	PFN_vkCmdResetQueryPool           CmdResetQueryPool;
	PFN_vkCmdResolveImage             CmdResolveImage;
	PFN_vkCmdSetBlendConstants        CmdSetBlendConstants;
	PFN_vkCmdSetDepthBias             CmdSetDepthBias;
	PFN_vkCmdSetDepthBounds           CmdSetDepthBounds;
	PFN_vkCmdSetDeviceMask            CmdSetDeviceMask;
	PFN_vkCmdSetEvent                 CmdSetEvent;
	PFN_vkCmdSetLineWidth             CmdSetLineWidth;
	PFN_vkCmdSetScissor               CmdSetScissor;
	PFN_vkCmdSetStencilCompareMask    CmdSetStencilCompareMask;
	PFN_vkCmdSetStencilReference      CmdSetStencilReference;
	PFN_vkCmdSetStencilWriteMask      CmdSetStencilWriteMask;
	PFN_vkCmdSetViewport              CmdSetViewport;
	PFN_vkCmdUpdateBuffer             CmdUpdateBuffer;
	PFN_vkCmdWaitEvents               CmdWaitEvents;
	PFN_vkCmdWriteTimestamp           CmdWriteTimestamp;
	PFN_vkEndCommandBuffer            EndCommandBuffer;
	PFN_vkResetCommandBuffer          ResetCommandBuffer;



	PFN_vkDestroyDevice DestroyDevice;
public:
	void destroyDevice() const;
	void destroy() const;

#define DEFINE_DEVICE_DESTROY(ObjectName)                        \
                                                                 \
	PFN_vkDestroy##ObjectName Destroy##ObjectName;               \
public:                                                          \
	void destroy##ObjectName(Vk##ObjectName a##ObjectName) const \
	{                                                            \
		Destroy##ObjectName(vkDevice(), a##ObjectName, nullptr); \
	}                                                            \
	void destroy(Vk##ObjectName a##ObjectName) const             \
	{                                                            \
		destroy##ObjectName(a##ObjectName);                      \
	}

	DEFINE_DEVICE_DESTROY(Buffer);
	DEFINE_DEVICE_DESTROY(BufferView);
	DEFINE_DEVICE_DESTROY(CommandPool);
	DEFINE_DEVICE_DESTROY(DescriptorPool);
	DEFINE_DEVICE_DESTROY(DescriptorSetLayout);
	DEFINE_DEVICE_DESTROY(DescriptorUpdateTemplate);
	DEFINE_DEVICE_DESTROY(Event);
	DEFINE_DEVICE_DESTROY(Fence);
	DEFINE_DEVICE_DESTROY(Framebuffer);
	DEFINE_DEVICE_DESTROY(Image);
	DEFINE_DEVICE_DESTROY(ImageView);
	DEFINE_DEVICE_DESTROY(Pipeline);
	DEFINE_DEVICE_DESTROY(PipelineCache);
	DEFINE_DEVICE_DESTROY(PipelineLayout);
	DEFINE_DEVICE_DESTROY(QueryPool);
	DEFINE_DEVICE_DESTROY(RenderPass);
	DEFINE_DEVICE_DESTROY(Sampler);
	DEFINE_DEVICE_DESTROY(SamplerYcbcrConversion);
	DEFINE_DEVICE_DESTROY(Semaphore);
	DEFINE_DEVICE_DESTROY(ShaderModule);



	//using UniqueBuffer =                   Unique<VkBuffer,                   VulkanDevice, &VulkanDevice::destroyBuffer                  >;
	//using UniqueBufferView =               Unique<VkBufferView,               VulkanDevice, &VulkanDevice::destroyBufferView              >;
	//using UniqueCommandPool =              Unique<VkCommandPool,              VulkanDevice, &VulkanDevice::destroyCommandPool             >;
	//using UniqueDescriptorPool =           Unique<VkDescriptorPool,           VulkanDevice, &VulkanDevice::destroyDescriptorPool          >;
	//using UniqueDescriptorSetLayout =      Unique<VkDescriptorSetLayout,      VulkanDevice, &VulkanDevice::destroyDescriptorSetLayout     >;
	//using UniqueDescriptorUpdateTemplate = Unique<VkDescriptorUpdateTemplate, VulkanDevice, &VulkanDevice::destroyDescriptorUpdateTemplate>;
	//using UniqueEvent =                    Unique<VkEvent,                    VulkanDevice, &VulkanDevice::destroyEvent                   >;
	//using UniqueFence =                    Unique<VkFence,                    VulkanDevice, &VulkanDevice::destroyFence                   >;
	//using UniqueFramebuffer =              Unique<VkFramebuffer,              VulkanDevice, &VulkanDevice::destroyFramebuffer             >;
	//using UniqueImage =                    Unique<VkImage,                    VulkanDevice, &VulkanDevice::destroyImage                   >;
	//using UniqueImageView =                Unique<VkImageView,                VulkanDevice, &VulkanDevice::destroyImageView               >;
	//using UniquePipeline =                 Unique<VkPipeline,                 VulkanDevice, &VulkanDevice::destroyPipeline                >;
	//using UniquePipelineCache =            Unique<VkPipelineCache,            VulkanDevice, &VulkanDevice::destroyPipelineCache           >;
	//using UniquePipelineLayout =           Unique<VkPipelineLayout,           VulkanDevice, &VulkanDevice::destroyPipelineLayout          >;
	//using UniqueQueryPool =                Unique<VkQueryPool,                VulkanDevice, &VulkanDevice::destroyQueryPool               >;
	//using UniqueRenderPass =               Unique<VkRenderPass,               VulkanDevice, &VulkanDevice::destroyRenderPass              >;
	//using UniqueSampler =                  Unique<VkSampler,                  VulkanDevice, &VulkanDevice::destroySampler                 >;
	//using UniqueSamplerYcbcrConversion =   Unique<VkSamplerYcbcrConversion,   VulkanDevice, &VulkanDevice::destroySamplerYcbcrConversion  >;
	//using UniqueSemaphore =                Unique<VkSemaphore,                VulkanDevice, &VulkanDevice::destroySemaphore               >;
	//using UniqueShaderModule =             Unique<VkShaderModule,             VulkanDevice, &VulkanDevice::destroyShaderModule            >;
	//
	//class UniqueGraphicsPipeline : public UniquePipeline {};
	//class UniqueComputePipeline : public UniquePipeline {};

	PFN_vkCreateComputePipelines CreateComputePipelines;
public:
	VkResult createComputePipelines(uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines, VkPipelineCache pipelineCache = nullptr) const;
	VkResult create(uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines, VkPipelineCache pipelineCache = nullptr) const;
	VkResult createComputePipelines(uint32_t createInfoCount, const vk::ComputePipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines, VkPipelineCache pipelineCache = nullptr) const;
	VkResult create(uint32_t createInfoCount, const vk::ComputePipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines, VkPipelineCache pipelineCache = nullptr) const;
	VkResult createComputePipeline(const vk::ComputePipelineCreateInfo& createInfo, VkPipeline& outPipeline, VkPipelineCache pipelineCache = nullptr) const;
	VkResult create(const vk::ComputePipelineCreateInfo& createInfo, VkPipeline& outPipeline, VkPipelineCache pipelineCache = nullptr) const;
	//UniqueComputePipeline createComputePipeline(const vk::ComputePipelineCreateInfo& createInfo, VkPipelineCache pipelineCache = nullptr) const;
	//UniqueComputePipeline create(const vk::ComputePipelineCreateInfo& createInfo, VkPipelineCache pipelineCache = nullptr) const
	//{
	//	return createComputePipeline(createInfo, pipelineCache);
	//}


	PFN_vkCreateGraphicsPipelines CreateGraphicsPipelines;
public:
	VkResult createGraphicsPipelines(uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines, VkPipelineCache pipelineCache = nullptr) const;
	VkResult create(uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines, VkPipelineCache pipelineCache = nullptr) const;
	VkResult createGraphicsPipelines(uint32_t createInfoCount, const vk::GraphicsPipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines, VkPipelineCache pipelineCache = nullptr) const;
	VkResult create(uint32_t createInfoCount, const vk::GraphicsPipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines, VkPipelineCache pipelineCache = nullptr) const;
	VkResult createGraphicsPipeline(const vk::GraphicsPipelineCreateInfo& createInfo, VkPipeline& outPipeline, VkPipelineCache pipelineCache = nullptr) const;
	VkResult create(const vk::GraphicsPipelineCreateInfo& createInfo, VkPipeline& outPipeline, VkPipelineCache pipelineCache = nullptr) const;
	//UniqueGraphicsPipeline createGraphicsPipeline(const vk::GraphicsPipelineCreateInfo& createInfo, VkPipelineCache pipelineCache = nullptr) const;
	//UniqueGraphicsPipeline create(const vk::GraphicsPipelineCreateInfo& createInfo, VkPipelineCache pipelineCache = nullptr) const
	//{
	//	return createGraphicsPipeline(createInfo, pipelineCache);
	//}


	PFN_vkCreateRenderPass2 CreateRenderPass2;
public:
	VkResult createRenderPass2(const vk::RenderPassCreateInfo2& createInfo, VkRenderPass& outRenderPass) const;
	VkResult create(const vk::RenderPassCreateInfo2& createInfo, VkRenderPass& outRenderPass) const;
	//UniqueRenderPass createRenderPass2(const vk::RenderPassCreateInfo2& createInfo) const;
	//UniqueRenderPass create(const vk::RenderPassCreateInfo2& createInfo) const
	//{
	//	return createRenderPass2(createInfo);
	//}

#define DEFINE_DEVICE_CREATE(ObjectName)                                                                                    \
                                                                                                                            \
	PFN_vkCreate##ObjectName Create##ObjectName;                                                                            \
public:                                                                                                                     \
	VkResult create##ObjectName(const vk::##ObjectName##CreateInfo& createInfo, Vk##ObjectName& out##ObjectName) const      \
	{                                                                                                                       \
		return Create##ObjectName(vkDevice(), &createInfo, nullptr, &out##ObjectName);                                      \
	}                                                                                                                       \
	VkResult create(const vk::##ObjectName##CreateInfo& createInfo, Vk##ObjectName& out##ObjectName) const                  \
	{                                                                                                                       \
		return create##ObjectName(createInfo, out##ObjectName);                                                             \
	}

	DEFINE_DEVICE_CREATE(Buffer);
	DEFINE_DEVICE_CREATE(BufferView);
	DEFINE_DEVICE_CREATE(CommandPool);
	DEFINE_DEVICE_CREATE(DescriptorPool);
	DEFINE_DEVICE_CREATE(DescriptorSetLayout);
	DEFINE_DEVICE_CREATE(DescriptorUpdateTemplate);
	DEFINE_DEVICE_CREATE(Event);
	DEFINE_DEVICE_CREATE(Fence);
	DEFINE_DEVICE_CREATE(Framebuffer);
	DEFINE_DEVICE_CREATE(Image);
	DEFINE_DEVICE_CREATE(ImageView);
	DEFINE_DEVICE_CREATE(PipelineCache);
	DEFINE_DEVICE_CREATE(PipelineLayout);
	DEFINE_DEVICE_CREATE(QueryPool);
	DEFINE_DEVICE_CREATE(RenderPass);
	DEFINE_DEVICE_CREATE(Sampler);
	DEFINE_DEVICE_CREATE(SamplerYcbcrConversion);
	DEFINE_DEVICE_CREATE(Semaphore);
	DEFINE_DEVICE_CREATE(ShaderModule);

	//UniqueBuffer                   createBuffer                   (const vk::BufferCreateInfo& createInfo)                   const;
	//UniqueBuffer                   create                         (const vk::BufferCreateInfo& createInfo)                   const { return createBuffer(createInfo); }
	//UniqueBufferView               createBufferView               (const vk::BufferViewCreateInfo& createInfo)               const;
	//UniqueBufferView               create                         (const vk::BufferViewCreateInfo& createInfo)               const { return createBufferView(createInfo); }
	//UniqueCommandPool              createCommandPool              (const vk::CommandPoolCreateInfo& createInfo)              const;
	//UniqueCommandPool              create                         (const vk::CommandPoolCreateInfo& createInfo)              const { return createCommandPool(createInfo); }
	//UniqueDescriptorPool           createDescriptorPool           (const vk::DescriptorPoolCreateInfo& createInfo)           const;
	//UniqueDescriptorPool           create                         (const vk::DescriptorPoolCreateInfo& createInfo)           const { return createDescriptorPool(createInfo); }
	//UniqueDescriptorSetLayout      createDescriptorSetLayout      (const vk::DescriptorSetLayoutCreateInfo& createInfo)      const;
	//UniqueDescriptorSetLayout      create                         (const vk::DescriptorSetLayoutCreateInfo& createInfo)      const { return createDescriptorSetLayout(createInfo); }
	//UniqueDescriptorUpdateTemplate createDescriptorUpdateTemplate (const vk::DescriptorUpdateTemplateCreateInfo& createInfo) const;
	//UniqueDescriptorUpdateTemplate create                         (const vk::DescriptorUpdateTemplateCreateInfo& createInfo) const { return createDescriptorUpdateTemplate(createInfo); }
	//UniqueEvent                    createEvent                    (const vk::EventCreateInfo& createInfo)                    const;
	//UniqueEvent                    create                         (const vk::EventCreateInfo& createInfo)                    const { return createEvent(createInfo); }
	//UniqueFence                    createFence                    (const vk::FenceCreateInfo& createInfo)                    const;
	//UniqueFence                    createFence                    (VkFenceCreateFlags flags = VkFenceCreateFlags{})          const;
	//UniqueFence                    create                         (const vk::FenceCreateInfo& createInfo)                    const { return createFence(createInfo); }
	//UniqueFramebuffer              createFramebuffer              (const vk::FramebufferCreateInfo& createInfo)              const;
	//UniqueFramebuffer              create                         (const vk::FramebufferCreateInfo& createInfo)              const { return createFramebuffer(createInfo); }
	//UniqueImage                    createImage                    (const vk::ImageCreateInfo& createInfo)                    const;
	//UniqueImage                    create                         (const vk::ImageCreateInfo& createInfo)                    const { return createImage(createInfo); }
	//UniqueImageView                createImageView                (const vk::ImageViewCreateInfo& createInfo)                const;
	//UniqueImageView                create                         (const vk::ImageViewCreateInfo& createInfo)                const { return createImageView(createInfo); }
	//UniquePipelineCache            createPipelineCache            (const vk::PipelineCacheCreateInfo& createInfo)            const;
	//UniquePipelineCache            create                         (const vk::PipelineCacheCreateInfo& createInfo)            const { return createPipelineCache(createInfo); }
	//UniquePipelineLayout           createPipelineLayout           (const vk::PipelineLayoutCreateInfo& createInfo)           const;
	//UniquePipelineLayout           create                         (const vk::PipelineLayoutCreateInfo& createInfo)           const { return createPipelineLayout(createInfo); }
	//UniqueQueryPool                createQueryPool                (const vk::QueryPoolCreateInfo& createInfo)                const;
	//UniqueQueryPool                create                         (const vk::QueryPoolCreateInfo& createInfo)                const { return createQueryPool(createInfo); }
	//UniqueRenderPass               createRenderPass               (const vk::RenderPassCreateInfo& createInfo)               const;
	//UniqueRenderPass               create                         (const vk::RenderPassCreateInfo& createInfo)               const { return createRenderPass(createInfo); }
	//UniqueSampler                  createSampler                  (const vk::SamplerCreateInfo& createInfo)                  const;
	//UniqueSampler                  create                         (const vk::SamplerCreateInfo& createInfo)                  const { return createSampler(createInfo); }
	//UniqueSamplerYcbcrConversion   createSamplerYcbcrConversion   (const vk::SamplerYcbcrConversionCreateInfo& createInfo)   const;
	//UniqueSamplerYcbcrConversion   create                         (const vk::SamplerYcbcrConversionCreateInfo& createInfo)   const { return createSamplerYcbcrConversion(createInfo); }
	//UniqueSemaphore                createSemaphore                (const vk::SemaphoreCreateInfo& createInfo)                const;
	//UniqueSemaphore                createSemaphore                (bool signaled = false)                                    const;
	//UniqueSemaphore                create                         (const vk::SemaphoreCreateInfo& createInfo)                const { return createSemaphore(createInfo); }
	//UniqueShaderModule             createShaderModule             (const vk::ShaderModuleCreateInfo& createInfo)             const;
	//UniqueShaderModule             create                         (const vk::ShaderModuleCreateInfo& createInfo)             const { return createShaderModule(createInfo); }


	PFN_vkDeviceWaitIdle DeviceWaitIdle;
public:
	VkResult waitIdle() const;
	VkResult wait() const;

	PFN_vkEnumerateInstanceVersion EnumerateInstanceVersion;
	PFN_vkFlushMappedMemoryRanges FlushMappedMemoryRanges;


	PFN_vkFreeCommandBuffers FreeCommandBuffers;
public:
	void freeCommandBuffers(VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers) const;
	void free(VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers) const;
	void freeCommandBuffer(VkCommandPool commandPool, VkCommandBuffer CommandBuffer) const;
	void free(VkCommandPool commandPool, VkCommandBuffer CommandBuffer) const;


	PFN_vkFreeDescriptorSets FreeDescriptorSets;
public:
	VkResult freeDescriptorSets(VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets) const;
	VkResult free(VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets) const;
	VkResult freeDescriptorSets(VkDescriptorPool descriptorPool, VkDescriptorSet DescriptorSet) const;
	VkResult free(VkDescriptorPool descriptorPool, VkDescriptorSet DescriptorSet) const;


	PFN_vkFreeMemory FreeMemory;
public:
	void freeMemory(VkDeviceMemory aDeviceMemory) const;
	void free(VkDeviceMemory aDeviceMemory) const;

	PFN_vkGetBufferDeviceAddress GetBufferDeviceAddress;
	PFN_vkGetBufferMemoryRequirements GetBufferMemoryRequirements;
	PFN_vkGetBufferMemoryRequirements2 GetBufferMemoryRequirements2;
	PFN_vkGetBufferOpaqueCaptureAddress GetBufferOpaqueCaptureAddress;
	PFN_vkGetDescriptorSetLayoutSupport GetDescriptorSetLayoutSupport;
	PFN_vkGetDeviceGroupPeerMemoryFeatures GetDeviceGroupPeerMemoryFeatures;
	PFN_vkGetDeviceMemoryCommitment GetDeviceMemoryCommitment;
	PFN_vkGetDeviceMemoryOpaqueCaptureAddress GetDeviceMemoryOpaqueCaptureAddress;
	PFN_vkGetDeviceQueue GetDeviceQueue;
	PFN_vkGetDeviceQueue2 GetDeviceQueue2;
	PFN_vkGetEventStatus GetEventStatus;
	PFN_vkGetFenceStatus GetFenceStatus;
	VkResult getFenceStatus(VkFence fence) const;
	VkResult getStatus(VkFence fence) const;

	PFN_vkGetImageMemoryRequirements GetImageMemoryRequirements;
	PFN_vkGetImageMemoryRequirements2 GetImageMemoryRequirements2;
	PFN_vkGetImageSparseMemoryRequirements GetImageSparseMemoryRequirements;
	PFN_vkGetImageSparseMemoryRequirements2 GetImageSparseMemoryRequirements2;
	PFN_vkGetImageSubresourceLayout GetImageSubresourceLayout;
	PFN_vkGetPipelineCacheData GetPipelineCacheData;
	PFN_vkGetQueryPoolResults GetQueryPoolResults;
	PFN_vkGetRenderAreaGranularity GetRenderAreaGranularity;
	PFN_vkGetSemaphoreCounterValue GetSemaphoreCounterValue;
	PFN_vkInvalidateMappedMemoryRanges InvalidateMappedMemoryRanges;
	PFN_vkMapMemory MapMemory;
	PFN_vkMergePipelineCaches MergePipelineCaches;
	PFN_vkQueueBindSparse QueueBindSparse;
	PFN_vkQueueSubmit QueueSubmit;
	VkResult queueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* submits, VkFence fence = nullptr) const;
	VkResult queueSubmit(VkQueue queue, const VkSubmitInfo& submit, VkFence fence = nullptr) const;
	VkResult queueSubmit(VkQueue queue, VkCommandBuffer commandBuffer, VkFence fence = nullptr) const;
	VkResult queueSubmit(VkQueue queue, VkCommandBuffer commandBuffer, VkSemaphore waitSemaphore, VkPipelineStageFlags waitDstStageMask, VkFence fence = nullptr) const;
	VkResult queueSubmit(VkQueue queue, VkCommandBuffer commandBuffer, VkSemaphore signalSemaphore, VkFence fence = nullptr) const;
	VkResult queueSubmit(VkQueue queue, VkCommandBuffer commandBuffer, VkSemaphore waitSemaphore, VkPipelineStageFlags waitDstStageMask, VkSemaphore signalSemaphore, VkFence fence = nullptr) const;

	PFN_vkQueueWaitIdle QueueWaitIdle;
public:
	VkResult queueWaitIdle(VkQueue queue) const;
	VkResult waitIdle(VkQueue queue) const;
	VkResult wait(VkQueue queue) const;

	PFN_vkResetCommandPool ResetCommandPool;
	PFN_vkResetDescriptorPool ResetDescriptorPool;
	PFN_vkResetEvent ResetEvent;

	PFN_vkResetFences ResetFences;
	VkResult resetFences(uint32_t fenceCount, const VkFence* fences) const;
	VkResult resetFences(std::initializer_list<VkFence> fences) const;
	VkResult resetFence(const VkFence& fence) const;

	PFN_vkResetQueryPool ResetQueryPool;
	PFN_vkSetEvent SetEvent;
	PFN_vkSignalSemaphore SignalSemaphore;
	PFN_vkTrimCommandPool TrimCommandPool;
	PFN_vkUnmapMemory UnmapMemory;

	PFN_vkUpdateDescriptorSets UpdateDescriptorSets;
	void updateDescriptorSets(uint32_t descriptorWriteCount, const vk::WriteDescriptorSet* descriptorWrites, uint32_t descriptorCopyCount, const vk::CopyDescriptorSet* descriptorCopies) const;
	void updateDescriptorSets(uint32_t descriptorWriteCount, const vk::WriteDescriptorSet* descriptorWrites) const;
	void updateDescriptorSets(uint32_t descriptorCopyCount, const vk::CopyDescriptorSet* descriptorCopies) const;
	void updateDescriptorSets(const vk::WriteDescriptorSet& descriptorWrite) const;
	void updateDescriptorSets(std::initializer_list<vk::WriteDescriptorSet> descriptorWrites) const;
	void updateDescriptorSets(const vk::CopyDescriptorSet& descriptorCopy) const;
	void updateDescriptorSets(std::initializer_list<vk::CopyDescriptorSet> descriptorCopies) const;

	PFN_vkUpdateDescriptorSetWithTemplate UpdateDescriptorSetWithTemplate;


	PFN_vkWaitForFences WaitForFences;
public:
	VkResult waitForFences(uint32_t fenceCount, const VkFence* pFences, bool waitAll, uint64_t timeout = UINT64_MAX) const;
	VkResult wait(uint32_t fenceCount, const VkFence* pFences, bool waitAll, uint64_t timeout = UINT64_MAX) const;
	VkResult waitForFence(VkFence fence, uint64_t timeout = UINT64_MAX) const;
	VkResult wait(VkFence fence, uint64_t timeout = UINT64_MAX) const;


	PFN_vkWaitSemaphores WaitSemaphores;
public:
	VkResult waitSemaphores(vk::SemaphoreWaitInfo& waitInfo, uint64_t timeout = UINT64_MAX) const;
	VkResult wait(vk::SemaphoreWaitInfo& waitInfo, uint64_t timeout = UINT64_MAX) const;

	// PFN_vkDestroySurfaceKHR DestroySurfaceKHR = nullptr;
	// PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR
	// GetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
	// PFN_vkGetPhysicalDeviceSurfaceFormatsKHR
	// GetPhysicalDeviceSurfaceFormatsKHR = nullptr;
	// PFN_vkGetPhysicalDeviceSurfacePresentModesKHR
	// GetPhysicalDeviceSurfacePresentModesKHR = nullptr;
	// PFN_vkGetPhysicalDeviceSurfaceSupportKHR
	// GetPhysicalDeviceSurfaceSupportKHR = nullptr;

	PFN_vkAcquireNextImage2KHR AcquireNextImage2KHR = nullptr;
	PFN_vkAcquireNextImageKHR AcquireNextImageKHR = nullptr;


	PFN_vkCreateSwapchainKHR CreateSwapchainKHR = nullptr;
public:
	VkResult createSwapchain(const vk::SwapchainCreateInfoKHR& createInfo, VkSwapchainKHR& outSwapchain) const;
	VkResult create(const vk::SwapchainCreateInfoKHR& createInfo, VkSwapchainKHR& outSwapchain) const;


	PFN_vkDestroySwapchainKHR DestroySwapchainKHR = nullptr;
public:
	void destroySwapchain(VkSwapchainKHR swapchain) const;
	void destroy(VkSwapchainKHR swapchain) const;

	PFN_vkGetDeviceGroupPresentCapabilitiesKHR GetDeviceGroupPresentCapabilitiesKHR = nullptr;
	PFN_vkGetDeviceGroupSurfacePresentModesKHR GetDeviceGroupSurfacePresentModesKHR = nullptr;
	// PFN_vkGetPhysicalDevicePresentRectanglesKHR GetPhysicalDevicePresentRectanglesKHR = nullptr;
	PFN_vkGetSwapchainImagesKHR GetSwapchainImagesKHR = nullptr;

	PFN_vkQueuePresentKHR QueuePresentKHR = nullptr;
	VkResult queuePresent(VkQueue queue, const cdm::vk::PresentInfoKHR& present) const;
	VkResult queuePresent(VkQueue queue, VkSwapchainKHR swapchain, uint32_t index) const;
	VkResult queuePresent(VkQueue queue, VkSwapchainKHR swapchain, uint32_t index, VkSemaphore waitSemaphore) const;


	// PFN_vkCreateDisplayModeKHR CreateDisplayModeKHR = nullptr;
	// PFN_vkCreateDisplayPlaneSurfaceKHR CreateDisplayPlaneSurfaceKHR =
	// nullptr; PFN_vkGetDisplayModePropertiesKHR GetDisplayModePropertiesKHR =
	// nullptr; PFN_vkGetDisplayPlaneCapabilitiesKHR
	// GetDisplayPlaneCapabilitiesKHR = nullptr;
	// PFN_vkGetDisplayPlaneSupportedDisplaysKHR
	// GetDisplayPlaneSupportedDisplaysKHR = nullptr;
	// PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR
	// GetPhysicalDeviceDisplayPlanePropertiesKHR = nullptr;
	// PFN_vkGetPhysicalDeviceDisplayPropertiesKHR
	// GetPhysicalDeviceDisplayPropertiesKHR = nullptr;

	PFN_vkCreateSharedSwapchainsKHR CreateSharedSwapchainsKHR = nullptr;

	// PFN_vkGetPhysicalDeviceFeatures2KHR GetPhysicalDeviceFeatures2KHR =
	// nullptr; PFN_vkGetPhysicalDeviceFormatProperties2KHR
	// GetPhysicalDeviceFormatProperties2KHR = nullptr;
	// PFN_vkGetPhysicalDeviceImageFormatProperties2KHR
	// GetPhysicalDeviceImageFormatProperties2KHR = nullptr;
	// PFN_vkGetPhysicalDeviceMemoryProperties2KHR
	// GetPhysicalDeviceMemoryProperties2KHR = nullptr;
	// PFN_vkGetPhysicalDeviceProperties2KHR GetPhysicalDeviceProperties2KHR =
	// nullptr; PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR
	// GetPhysicalDeviceQueueFamilyProperties2KHR = nullptr;
	// PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR
	// GetPhysicalDeviceSparseImageFormatProperties2KHR = nullptr;

	PFN_vkCmdDispatchBaseKHR CmdDispatchBaseKHR = nullptr;
	PFN_vkCmdSetDeviceMaskKHR CmdSetDeviceMaskKHR = nullptr;
	PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR GetDeviceGroupPeerMemoryFeaturesKHR = nullptr;

	PFN_vkTrimCommandPoolKHR TrimCommandPoolKHR = nullptr;

	// PFN_vkEnumeratePhysicalDeviceGroupsKHR EnumeratePhysicalDeviceGroupsKHR
	// = nullptr;

	// PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR
	// GetPhysicalDeviceExternalBufferPropertiesKHR = nullptr;

	PFN_vkGetMemoryFdKHR GetMemoryFdKHR = nullptr;
	PFN_vkGetMemoryFdPropertiesKHR GetMemoryFdPropertiesKHR = nullptr;

	// PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR
	// GetPhysicalDeviceExternalSemaphorePropertiesKHR = nullptr;

	PFN_vkGetSemaphoreFdKHR GetSemaphoreFdKHR = nullptr;
	PFN_vkImportSemaphoreFdKHR ImportSemaphoreFdKHR = nullptr;

	PFN_vkCmdPushDescriptorSetKHR CmdPushDescriptorSetKHR = nullptr;
	PFN_vkCmdPushDescriptorSetWithTemplateKHR CmdPushDescriptorSetWithTemplateKHR = nullptr;

	PFN_vkCreateDescriptorUpdateTemplateKHR CreateDescriptorUpdateTemplateKHR = nullptr;
	PFN_vkDestroyDescriptorUpdateTemplateKHR DestroyDescriptorUpdateTemplateKHR = nullptr;
	PFN_vkUpdateDescriptorSetWithTemplateKHR UpdateDescriptorSetWithTemplateKHR = nullptr;

	PFN_vkCmdBeginRenderPass2KHR CmdBeginRenderPass2KHR = nullptr;
	PFN_vkCmdEndRenderPass2KHR CmdEndRenderPass2KHR = nullptr;
	PFN_vkCmdNextSubpass2KHR CmdNextSubpass2KHR = nullptr;
	PFN_vkCreateRenderPass2KHR CreateRenderPass2KHR = nullptr;

	PFN_vkGetSwapchainStatusKHR GetSwapchainStatusKHR = nullptr;

	// PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR
	// GetPhysicalDeviceExternalFencePropertiesKHR = nullptr;

	PFN_vkGetFenceFdKHR GetFenceFdKHR = nullptr;
	PFN_vkImportFenceFdKHR ImportFenceFdKHR = nullptr;

	PFN_vkAcquireProfilingLockKHR AcquireProfilingLockKHR = nullptr;
	// PFN_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR
	// EnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR = nullptr;
	// PFN_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR
	// GetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR = nullptr;
	PFN_vkReleaseProfilingLockKHR ReleaseProfilingLockKHR = nullptr;

	// PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR
	// GetPhysicalDeviceSurfaceCapabilities2KHR = nullptr;
	// PFN_vkGetPhysicalDeviceSurfaceFormats2KHR
	// GetPhysicalDeviceSurfaceFormats2KHR = nullptr;

	// PFN_vkGetDisplayModeProperties2KHR GetDisplayModeProperties2KHR =
	// nullptr; PFN_vkGetDisplayPlaneCapabilities2KHR
	// GetDisplayPlaneCapabilities2KHR = nullptr;
	// PFN_vkGetPhysicalDeviceDisplayPlaneProperties2KHR
	// GetPhysicalDeviceDisplayPlaneProperties2KHR = nullptr;
	// PFN_vkGetPhysicalDeviceDisplayProperties2KHR
	// GetPhysicalDeviceDisplayProperties2KHR = nullptr;

	PFN_vkGetBufferMemoryRequirements2KHR GetBufferMemoryRequirements2KHR = nullptr;
	PFN_vkGetImageMemoryRequirements2KHR GetImageMemoryRequirements2KHR = nullptr;
	PFN_vkGetImageSparseMemoryRequirements2KHR GetImageSparseMemoryRequirements2KHR = nullptr;

	PFN_vkCreateSamplerYcbcrConversionKHR CreateSamplerYcbcrConversionKHR = nullptr;
	PFN_vkDestroySamplerYcbcrConversionKHR DestroySamplerYcbcrConversionKHR = nullptr;

	PFN_vkBindBufferMemory2KHR BindBufferMemory2KHR = nullptr;
	PFN_vkBindImageMemory2KHR BindImageMemory2KHR = nullptr;

	PFN_vkGetDescriptorSetLayoutSupportKHR GetDescriptorSetLayoutSupportKHR = nullptr;

	PFN_vkCmdDrawIndexedIndirectCountKHR CmdDrawIndexedIndirectCountKHR = nullptr;
	PFN_vkCmdDrawIndirectCountKHR CmdDrawIndirectCountKHR = nullptr;

	PFN_vkGetSemaphoreCounterValueKHR GetSemaphoreCounterValueKHR = nullptr;
	PFN_vkSignalSemaphoreKHR SignalSemaphoreKHR = nullptr;
	PFN_vkWaitSemaphoresKHR WaitSemaphoresKHR = nullptr;

	PFN_vkGetBufferDeviceAddressKHR GetBufferDeviceAddressKHR = nullptr;
	PFN_vkGetBufferOpaqueCaptureAddressKHR GetBufferOpaqueCaptureAddressKHR = nullptr;
	PFN_vkGetDeviceMemoryOpaqueCaptureAddressKHR GetDeviceMemoryOpaqueCaptureAddressKHR = nullptr;

	PFN_vkGetPipelineExecutableInternalRepresentationsKHR GetPipelineExecutableInternalRepresentationsKHR = nullptr;
	PFN_vkGetPipelineExecutablePropertiesKHR GetPipelineExecutablePropertiesKHR = nullptr;
	PFN_vkGetPipelineExecutableStatisticsKHR GetPipelineExecutableStatisticsKHR = nullptr;

	// PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallbackEXT =
	// nullptr; PFN_vkDebugReportMessageEXT DebugReportMessageEXT = nullptr;
	// PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallbackEXT =
	// nullptr;

	PFN_vkCmdDebugMarkerBeginEXT CmdDebugMarkerBeginEXT = nullptr;
	PFN_vkCmdDebugMarkerEndEXT CmdDebugMarkerEndEXT = nullptr;
	PFN_vkCmdDebugMarkerInsertEXT CmdDebugMarkerInsertEXT = nullptr;
	PFN_vkDebugMarkerSetObjectNameEXT DebugMarkerSetObjectNameEXT = nullptr;
	PFN_vkDebugMarkerSetObjectTagEXT DebugMarkerSetObjectTagEXT = nullptr;
	VkResult debugMarkerSetObjectName(const vk::DebugMarkerObjectNameInfoEXT& nameInfo) const;
	template<typename VkHandle>
	VkResult debugMarkerSetObjectName(VkHandle object, VkDebugReportObjectTypeEXT objectType, std::string_view objectName) const;
	VkResult debugMarkerSetObjectName(VkInstance object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkPhysicalDevice object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkDevice object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkQueue object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkSemaphore object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkCommandBuffer object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkFence object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkDeviceMemory object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkBuffer object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkImage object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkEvent object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkQueryPool object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkBufferView object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkImageView object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkShaderModule object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkPipelineCache object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkPipelineLayout object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkRenderPass object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkPipeline object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkDescriptorSetLayout object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkSampler object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkDescriptorPool object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkDescriptorSet object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkFramebuffer object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkCommandPool object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkSurfaceKHR object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT, objectName); }
    VkResult debugMarkerSetObjectName(VkSwapchainKHR object, std::string_view objectName) const { return debugMarkerSetObjectName(object, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT, objectName); }

	VkResult debugMarkerSetObjectTag(const vk::DebugMarkerObjectTagInfoEXT& tagInfo) const;
	template<typename VkHandle, typename T>
	VkResult debugMarkerSetObjectTag(VkHandle object, VkDebugReportObjectTypeEXT objectType, uint64_t tagName, const T& tag) const;

	PFN_vkCmdBeginQueryIndexedEXT CmdBeginQueryIndexedEXT = nullptr;
	PFN_vkCmdBeginTransformFeedbackEXT CmdBeginTransformFeedbackEXT = nullptr;
	PFN_vkCmdBindTransformFeedbackBuffersEXT CmdBindTransformFeedbackBuffersEXT = nullptr;
	PFN_vkCmdDrawIndirectByteCountEXT CmdDrawIndirectByteCountEXT = nullptr;
	PFN_vkCmdEndQueryIndexedEXT CmdEndQueryIndexedEXT = nullptr;
	PFN_vkCmdEndTransformFeedbackEXT CmdEndTransformFeedbackEXT = nullptr;

	PFN_vkGetImageViewHandleNVX GetImageViewHandleNVX = nullptr;

	PFN_vkCmdDrawIndexedIndirectCountAMD CmdDrawIndexedIndirectCountAMD = nullptr;
	PFN_vkCmdDrawIndirectCountAMD CmdDrawIndirectCountAMD = nullptr;

	PFN_vkGetShaderInfoAMD GetShaderInfoAMD = nullptr;

	// PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV
	// GetPhysicalDeviceExternalImageFormatPropertiesNV = nullptr;

	PFN_vkCmdBeginConditionalRenderingEXT CmdBeginConditionalRenderingEXT = nullptr;
	PFN_vkCmdEndConditionalRenderingEXT CmdEndConditionalRenderingEXT = nullptr;

	//PFN_vkCmdProcessCommandsNVX CmdProcessCommandsNVX = nullptr;
	//PFN_vkCmdReserveSpaceForCommandsNVX CmdReserveSpaceForCommandsNVX = nullptr;
	//PFN_vkCreateIndirectCommandsLayoutNVX CreateIndirectCommandsLayoutNVX = nullptr;
	//PFN_vkCreateObjectTableNVX CreateObjectTableNVX = nullptr;
	//PFN_vkDestroyIndirectCommandsLayoutNVX DestroyIndirectCommandsLayoutNVX = nullptr;
	//PFN_vkDestroyObjectTableNVX DestroyObjectTableNVX = nullptr;
	//// PFN_vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX
	//// GetPhysicalDeviceGeneratedCommandsPropertiesNVX = nullptr;
	//PFN_vkRegisterObjectsNVX RegisterObjectsNVX = nullptr;
	//PFN_vkUnregisterObjectsNVX UnregisterObjectsNVX = nullptr;

	PFN_vkCmdSetViewportWScalingNV CmdSetViewportWScalingNV = nullptr;

	// PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT
	// GetPhysicalDeviceSurfaceCapabilities2EXT = nullptr;

	PFN_vkDisplayPowerControlEXT DisplayPowerControlEXT = nullptr;
	PFN_vkGetSwapchainCounterEXT GetSwapchainCounterEXT = nullptr;
	PFN_vkRegisterDeviceEventEXT RegisterDeviceEventEXT = nullptr;
	PFN_vkRegisterDisplayEventEXT RegisterDisplayEventEXT = nullptr;

	PFN_vkGetPastPresentationTimingGOOGLE GetPastPresentationTimingGOOGLE = nullptr;
	PFN_vkGetRefreshCycleDurationGOOGLE GetRefreshCycleDurationGOOGLE = nullptr;

	PFN_vkCmdSetDiscardRectangleEXT CmdSetDiscardRectangleEXT = nullptr;

	PFN_vkSetHdrMetadataEXT SetHdrMetadataEXT = nullptr;

	PFN_vkCmdBeginDebugUtilsLabelEXT CmdBeginDebugUtilsLabelEXT = nullptr;
	PFN_vkCmdEndDebugUtilsLabelEXT CmdEndDebugUtilsLabelEXT = nullptr;
	PFN_vkCmdInsertDebugUtilsLabelEXT CmdInsertDebugUtilsLabelEXT = nullptr;
	// PFN_vkCreateDebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT =
	// nullptr; PFN_vkDestroyDebugUtilsMessengerEXT
	// DestroyDebugUtilsMessengerEXT = nullptr;
	PFN_vkQueueBeginDebugUtilsLabelEXT QueueBeginDebugUtilsLabelEXT = nullptr;
	PFN_vkQueueEndDebugUtilsLabelEXT QueueEndDebugUtilsLabelEXT = nullptr;
	PFN_vkQueueInsertDebugUtilsLabelEXT QueueInsertDebugUtilsLabelEXT = nullptr;
	PFN_vkSetDebugUtilsObjectNameEXT SetDebugUtilsObjectNameEXT = nullptr;
	PFN_vkSetDebugUtilsObjectTagEXT SetDebugUtilsObjectTagEXT = nullptr;
	// PFN_vkSubmitDebugUtilsMessageEXT SubmitDebugUtilsMessageEXT = nullptr;

	PFN_vkCmdSetSampleLocationsEXT CmdSetSampleLocationsEXT = nullptr;
	// PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT
	// GetPhysicalDeviceMultisamplePropertiesEXT = nullptr;

	PFN_vkGetImageDrmFormatModifierPropertiesEXT GetImageDrmFormatModifierPropertiesEXT = nullptr;

	PFN_vkCreateValidationCacheEXT CreateValidationCacheEXT = nullptr;
	PFN_vkDestroyValidationCacheEXT DestroyValidationCacheEXT = nullptr;
	PFN_vkGetValidationCacheDataEXT GetValidationCacheDataEXT = nullptr;
	PFN_vkMergeValidationCachesEXT MergeValidationCachesEXT = nullptr;

	PFN_vkCmdBindShadingRateImageNV CmdBindShadingRateImageNV = nullptr;
	PFN_vkCmdSetCoarseSampleOrderNV CmdSetCoarseSampleOrderNV = nullptr;
	PFN_vkCmdSetViewportShadingRatePaletteNV CmdSetViewportShadingRatePaletteNV = nullptr;

	PFN_vkBindAccelerationStructureMemoryNV BindAccelerationStructureMemoryNV = nullptr;
	PFN_vkCmdBuildAccelerationStructureNV CmdBuildAccelerationStructureNV = nullptr;
	PFN_vkCmdCopyAccelerationStructureNV CmdCopyAccelerationStructureNV = nullptr;
	PFN_vkCmdTraceRaysNV CmdTraceRaysNV = nullptr;
	PFN_vkCmdWriteAccelerationStructuresPropertiesNV CmdWriteAccelerationStructuresPropertiesNV = nullptr;
	PFN_vkCompileDeferredNV CompileDeferredNV = nullptr;
	PFN_vkCreateAccelerationStructureNV CreateAccelerationStructureNV = nullptr;
	PFN_vkCreateRayTracingPipelinesNV CreateRayTracingPipelinesNV = nullptr;
	PFN_vkDestroyAccelerationStructureNV DestroyAccelerationStructureNV = nullptr;
	PFN_vkGetAccelerationStructureHandleNV GetAccelerationStructureHandleNV = nullptr;
	PFN_vkGetAccelerationStructureMemoryRequirementsNV GetAccelerationStructureMemoryRequirementsNV = nullptr;
	PFN_vkGetRayTracingShaderGroupHandlesNV GetRayTracingShaderGroupHandlesNV = nullptr;

	PFN_vkGetMemoryHostPointerPropertiesEXT GetMemoryHostPointerPropertiesEXT = nullptr;

	PFN_vkCmdWriteBufferMarkerAMD CmdWriteBufferMarkerAMD = nullptr;

	PFN_vkGetCalibratedTimestampsEXT GetCalibratedTimestampsEXT = nullptr;
	// PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT
	// GetPhysicalDeviceCalibrateableTimeDomainsEXT = nullptr;

	PFN_vkCmdDrawMeshTasksIndirectCountNV CmdDrawMeshTasksIndirectCountNV = nullptr;
	PFN_vkCmdDrawMeshTasksIndirectNV CmdDrawMeshTasksIndirectNV = nullptr;
	PFN_vkCmdDrawMeshTasksNV CmdDrawMeshTasksNV = nullptr;

	PFN_vkCmdSetExclusiveScissorNV CmdSetExclusiveScissorNV = nullptr;

	PFN_vkAcquirePerformanceConfigurationINTEL AcquirePerformanceConfigurationINTEL = nullptr;
	PFN_vkCmdSetPerformanceMarkerINTEL CmdSetPerformanceMarkerINTEL = nullptr;
	PFN_vkCmdSetPerformanceOverrideINTEL CmdSetPerformanceOverrideINTEL = nullptr;
	PFN_vkCmdSetPerformanceStreamMarkerINTEL CmdSetPerformanceStreamMarkerINTEL = nullptr;
	PFN_vkGetPerformanceParameterINTEL GetPerformanceParameterINTEL = nullptr;
	PFN_vkInitializePerformanceApiINTEL InitializePerformanceApiINTEL = nullptr;
	PFN_vkQueueSetPerformanceConfigurationINTEL QueueSetPerformanceConfigurationINTEL = nullptr;
	PFN_vkReleasePerformanceConfigurationINTEL ReleasePerformanceConfigurationINTEL = nullptr;
	PFN_vkUninitializePerformanceApiINTEL UninitializePerformanceApiINTEL = nullptr;

	PFN_vkSetLocalDimmingAMD SetLocalDimmingAMD = nullptr;

	PFN_vkGetBufferDeviceAddressEXT GetBufferDeviceAddressEXT = nullptr;

	// PFN_vkGetPhysicalDeviceToolPropertiesEXT
	// GetPhysicalDeviceToolPropertiesEXT = nullptr;

	// PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV
	// GetPhysicalDeviceCooperativeMatrixPropertiesNV = nullptr;

	// PFN_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV
	// GetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV =
	// nullptr;

	// PFN_vkCreateHeadlessSurfaceEXT CreateHeadlessSurfaceEXT = nullptr;

	PFN_vkCmdSetLineStippleEXT CmdSetLineStippleEXT = nullptr;

	PFN_vkResetQueryPoolEXT ResetQueryPoolEXT = nullptr;

	PFN_vkGetMemoryWin32HandleKHR GetMemoryWin32HandleKHR = nullptr;
	PFN_vkGetMemoryWin32HandlePropertiesKHR GetMemoryWin32HandlePropertiesKHR = nullptr;

	PFN_vkGetSemaphoreWin32HandleKHR GetSemaphoreWin32HandleKHR = nullptr;
	PFN_vkImportSemaphoreWin32HandleKHR ImportSemaphoreWin32HandleKHR = nullptr;

	PFN_vkGetFenceWin32HandleKHR GetFenceWin32HandleKHR = nullptr;
	PFN_vkImportFenceWin32HandleKHR ImportFenceWin32HandleKHR = nullptr;

	PFN_vkGetMemoryWin32HandleNV GetMemoryWin32HandleNV = nullptr;

	PFN_vkAcquireFullScreenExclusiveModeEXT AcquireFullScreenExclusiveModeEXT = nullptr;
	PFN_vkGetDeviceGroupSurfacePresentModes2EXT GetDeviceGroupSurfacePresentModes2EXT = nullptr;
	PFN_vkGetPhysicalDeviceSurfacePresentModes2EXT GetPhysicalDeviceSurfacePresentModes2EXT = nullptr;
	PFN_vkReleaseFullScreenExclusiveModeEXT ReleaseFullScreenExclusiveModeEXT = nullptr;
	// clang-format on
};

// clang-format off
using UniqueBuffer =                   Unique<VkBuffer,                   VulkanDeviceDestroyer, &VulkanDeviceDestroyer::destroyBuffer                  >;
using UniqueBufferView =               Unique<VkBufferView,               VulkanDeviceDestroyer, &VulkanDeviceDestroyer::destroyBufferView              >;
using UniqueCommandPool =              Unique<VkCommandPool,              VulkanDeviceDestroyer, &VulkanDeviceDestroyer::destroyCommandPool             >;
using UniqueDescriptorPool =           Unique<VkDescriptorPool,           VulkanDeviceDestroyer, &VulkanDeviceDestroyer::destroyDescriptorPool          >;
using UniqueDescriptorSetLayout =      Unique<VkDescriptorSetLayout,      VulkanDeviceDestroyer, &VulkanDeviceDestroyer::destroyDescriptorSetLayout     >;
using UniqueDescriptorUpdateTemplate = Unique<VkDescriptorUpdateTemplate, VulkanDeviceDestroyer, &VulkanDeviceDestroyer::destroyDescriptorUpdateTemplate>;
using UniqueEvent =                    Unique<VkEvent,                    VulkanDeviceDestroyer, &VulkanDeviceDestroyer::destroyEvent                   >;
using UniqueFence =                    Unique<VkFence,                    VulkanDeviceDestroyer, &VulkanDeviceDestroyer::destroyFence                   >;
using UniqueFramebuffer =              Unique<VkFramebuffer,              VulkanDeviceDestroyer, &VulkanDeviceDestroyer::destroyFramebuffer             >;
using UniqueImage =                    Unique<VkImage,                    VulkanDeviceDestroyer, &VulkanDeviceDestroyer::destroyImage                   >;
using UniqueImageView =                Unique<VkImageView,                VulkanDeviceDestroyer, &VulkanDeviceDestroyer::destroyImageView               >;
using UniquePipeline =                 Unique<VkPipeline,                 VulkanDeviceDestroyer, &VulkanDeviceDestroyer::destroyPipeline                >;
using UniquePipelineCache =            Unique<VkPipelineCache,            VulkanDeviceDestroyer, &VulkanDeviceDestroyer::destroyPipelineCache           >;
using UniquePipelineLayout =           Unique<VkPipelineLayout,           VulkanDeviceDestroyer, &VulkanDeviceDestroyer::destroyPipelineLayout          >;
using UniqueQueryPool =                Unique<VkQueryPool,                VulkanDeviceDestroyer, &VulkanDeviceDestroyer::destroyQueryPool               >;
using UniqueRenderPass =               Unique<VkRenderPass,               VulkanDeviceDestroyer, &VulkanDeviceDestroyer::destroyRenderPass              >;
using UniqueSampler =                  Unique<VkSampler,                  VulkanDeviceDestroyer, &VulkanDeviceDestroyer::destroySampler                 >;
using UniqueSamplerYcbcrConversion =   Unique<VkSamplerYcbcrConversion,   VulkanDeviceDestroyer, &VulkanDeviceDestroyer::destroySamplerYcbcrConversion  >;
using UniqueSemaphore =                Unique<VkSemaphore,                VulkanDeviceDestroyer, &VulkanDeviceDestroyer::destroySemaphore               >;
using UniqueShaderModule =             Unique<VkShaderModule,             VulkanDeviceDestroyer, &VulkanDeviceDestroyer::destroyShaderModule            >;

class UniqueGraphicsPipeline : public UniquePipeline { public: using UniquePipeline::UniquePipeline; };
class UniqueComputePipeline : public UniquePipeline  { public: using UniquePipeline::UniquePipeline; };
// clang-format on

class VulkanDevice final : public VulkanDeviceDestroyer
{
public:
	using VulkanDeviceDestroyer::VulkanDeviceDestroyer;
	using VulkanDeviceDestroyer::createBuffer                   ;
	using VulkanDeviceDestroyer::create                         ;
	using VulkanDeviceDestroyer::createBufferView               ;
	using VulkanDeviceDestroyer::createCommandPool              ;
	using VulkanDeviceDestroyer::createDescriptorPool           ;
	using VulkanDeviceDestroyer::createDescriptorSetLayout      ;
	using VulkanDeviceDestroyer::createDescriptorUpdateTemplate ;
	using VulkanDeviceDestroyer::createEvent                    ;
	using VulkanDeviceDestroyer::createFence                    ;
	using VulkanDeviceDestroyer::createFramebuffer              ;
	using VulkanDeviceDestroyer::createImage                    ;
	using VulkanDeviceDestroyer::createImageView                ;
	using VulkanDeviceDestroyer::createPipelineCache            ;
	using VulkanDeviceDestroyer::createPipelineLayout           ;
	using VulkanDeviceDestroyer::createQueryPool                ;
	using VulkanDeviceDestroyer::createRenderPass               ;
	using VulkanDeviceDestroyer::createRenderPass2              ;
	using VulkanDeviceDestroyer::createSampler                  ;
	using VulkanDeviceDestroyer::createSamplerYcbcrConversion   ;
	using VulkanDeviceDestroyer::createSemaphore                ;
	using VulkanDeviceDestroyer::createShaderModule             ;
	using VulkanDeviceDestroyer::createComputePipelines         ;
	using VulkanDeviceDestroyer::createComputePipeline          ;
	using VulkanDeviceDestroyer::createGraphicsPipelines        ;
	using VulkanDeviceDestroyer::createGraphicsPipeline         ;

	UniqueComputePipeline createComputePipeline(const vk::ComputePipelineCreateInfo& createInfo, VkPipelineCache pipelineCache = nullptr) const;
	UniqueComputePipeline create(const vk::ComputePipelineCreateInfo& createInfo, VkPipelineCache pipelineCache = nullptr) const
	{
		return createComputePipeline(createInfo, pipelineCache);
	}

	UniqueGraphicsPipeline createGraphicsPipeline(const vk::GraphicsPipelineCreateInfo& createInfo, VkPipelineCache pipelineCache = nullptr) const;
	UniqueGraphicsPipeline create(const vk::GraphicsPipelineCreateInfo& createInfo, VkPipelineCache pipelineCache = nullptr) const
	{
		return createGraphicsPipeline(createInfo, pipelineCache);
	}

	UniqueRenderPass createRenderPass2(const vk::RenderPassCreateInfo2& createInfo) const;
	UniqueRenderPass create(const vk::RenderPassCreateInfo2& createInfo) const
	{
		return createRenderPass2(createInfo);
	}

	UniqueBuffer                   createBuffer                   (const vk::BufferCreateInfo& createInfo)                   const;
	UniqueBuffer                   create                         (const vk::BufferCreateInfo& createInfo)                   const { return createBuffer(createInfo); }
	UniqueBufferView               createBufferView               (const vk::BufferViewCreateInfo& createInfo)               const;
	UniqueBufferView               create                         (const vk::BufferViewCreateInfo& createInfo)               const { return createBufferView(createInfo); }
	UniqueCommandPool              createCommandPool              (const vk::CommandPoolCreateInfo& createInfo)              const;
	UniqueCommandPool              create                         (const vk::CommandPoolCreateInfo& createInfo)              const { return createCommandPool(createInfo); }
	UniqueDescriptorPool           createDescriptorPool           (const vk::DescriptorPoolCreateInfo& createInfo)           const;
	UniqueDescriptorPool           create                         (const vk::DescriptorPoolCreateInfo& createInfo)           const { return createDescriptorPool(createInfo); }
	UniqueDescriptorSetLayout      createDescriptorSetLayout      (const vk::DescriptorSetLayoutCreateInfo& createInfo)      const;
	UniqueDescriptorSetLayout      create                         (const vk::DescriptorSetLayoutCreateInfo& createInfo)      const { return createDescriptorSetLayout(createInfo); }
	UniqueDescriptorUpdateTemplate createDescriptorUpdateTemplate (const vk::DescriptorUpdateTemplateCreateInfo& createInfo) const;
	UniqueDescriptorUpdateTemplate create                         (const vk::DescriptorUpdateTemplateCreateInfo& createInfo) const { return createDescriptorUpdateTemplate(createInfo); }
	UniqueEvent                    createEvent                    (const vk::EventCreateInfo& createInfo)                    const;
	UniqueEvent                    create                         (const vk::EventCreateInfo& createInfo)                    const { return createEvent(createInfo); }
	UniqueFence                    createFence                    (const vk::FenceCreateInfo& createInfo)                    const;
	UniqueFence                    createFence                    (VkFenceCreateFlags flags = VkFenceCreateFlags{})          const;
	UniqueFence                    create                         (const vk::FenceCreateInfo& createInfo)                    const { return createFence(createInfo); }
	UniqueFramebuffer              createFramebuffer              (const vk::FramebufferCreateInfo& createInfo)              const;
	UniqueFramebuffer              create                         (const vk::FramebufferCreateInfo& createInfo)              const { return createFramebuffer(createInfo); }
	UniqueImage                    createImage                    (const vk::ImageCreateInfo& createInfo)                    const;
	UniqueImage                    create                         (const vk::ImageCreateInfo& createInfo)                    const { return createImage(createInfo); }
	UniqueImageView                createImageView                (const vk::ImageViewCreateInfo& createInfo)                const;
	UniqueImageView                create                         (const vk::ImageViewCreateInfo& createInfo)                const { return createImageView(createInfo); }
	UniquePipelineCache            createPipelineCache            (const vk::PipelineCacheCreateInfo& createInfo)            const;
	UniquePipelineCache            create                         (const vk::PipelineCacheCreateInfo& createInfo)            const { return createPipelineCache(createInfo); }
	UniquePipelineLayout           createPipelineLayout           (const vk::PipelineLayoutCreateInfo& createInfo)           const;
	UniquePipelineLayout           create                         (const vk::PipelineLayoutCreateInfo& createInfo)           const { return createPipelineLayout(createInfo); }
	UniqueQueryPool                createQueryPool                (const vk::QueryPoolCreateInfo& createInfo)                const;
	UniqueQueryPool                create                         (const vk::QueryPoolCreateInfo& createInfo)                const { return createQueryPool(createInfo); }
	UniqueRenderPass               createRenderPass               (const vk::RenderPassCreateInfo& createInfo)               const;
	UniqueRenderPass               create                         (const vk::RenderPassCreateInfo& createInfo)               const { return createRenderPass(createInfo); }
	UniqueSampler                  createSampler                  (const vk::SamplerCreateInfo& createInfo)                  const;
	UniqueSampler                  create                         (const vk::SamplerCreateInfo& createInfo)                  const { return createSampler(createInfo); }
	UniqueSamplerYcbcrConversion   createSamplerYcbcrConversion   (const vk::SamplerYcbcrConversionCreateInfo& createInfo)   const;
	UniqueSamplerYcbcrConversion   create                         (const vk::SamplerYcbcrConversionCreateInfo& createInfo)   const { return createSamplerYcbcrConversion(createInfo); }
	UniqueSemaphore                createSemaphore                (const vk::SemaphoreCreateInfo& createInfo)                const;
	UniqueSemaphore                createSemaphore                (bool signaled = false)                                    const;
	UniqueSemaphore                create                         (const vk::SemaphoreCreateInfo& createInfo)                const { return createSemaphore(createInfo); }
	UniqueShaderModule             createShaderModule             (const vk::ShaderModuleCreateInfo& createInfo)             const;
	UniqueShaderModule             create                         (const vk::ShaderModuleCreateInfo& createInfo)             const { return createShaderModule(createInfo); }
};

class VulkanDeviceObject
{
	std::reference_wrapper<const VulkanDevice> vkDevice;
	double m_creationTime = 0.0;

protected:
	void setCreationTime();
	// void resetCreationTime() { m_creationTime = 0.0; }

public:
	VulkanDeviceObject(const VulkanDevice& device) : vkDevice(device) {}
	VulkanDeviceObject(const VulkanDeviceObject&) = default;
	VulkanDeviceObject(VulkanDeviceObject&&) = default;

	VulkanDeviceObject& operator=(const VulkanDeviceObject&) = default;
	VulkanDeviceObject& operator=(VulkanDeviceObject&&) = default;

	const VulkanDevice& device() const { return vkDevice.get(); }
	double creationTime() const { return m_creationTime; }
};

class LogRRID
{
	std::reference_wrapper<const VulkanDevice> m_device;

public:
	LogRRID(const VulkanDevice& vulkanDevice);
	LogRRID(const LogRRID&) = default;
	LogRRID(LogRRID&&) = default;
	~LogRRID();

	LogRRID& operator=(const LogRRID&) = default;
	LogRRID& operator=(LogRRID&&) = default;
};
}  // namespace cdm

#include "VulkanDevice.inl"
