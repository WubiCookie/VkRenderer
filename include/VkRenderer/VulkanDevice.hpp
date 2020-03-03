#pragma once

//#include "VulkanFunctions.hpp"

#define NOCOMM
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#define VK_NO_PROTOTYPES
#include <libloaderapi.h>
#include "cdm_vulkan.hpp"

#include <optional>
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

public:
	static inline bool LogActive = false;

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
VULKAN_BASE_FUNCTIONS_VISIBILITY:
	PFN_vkDestroySurfaceKHR DestroySurfaceKHR;
public:
	inline void destroySurface(VkSurfaceKHR surface) const;
	inline void destroy(VkSurfaceKHR surface) const;

	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR GetPhysicalDeviceSurfaceCapabilitiesKHR;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR GetPhysicalDeviceSurfaceFormatsKHR;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR GetPhysicalDeviceSurfacePresentModesKHR;
	PFN_vkGetPhysicalDeviceSurfaceSupportKHR GetPhysicalDeviceSurfaceSupportKHR;

	// VK_KHR_WIN32_SURFACE_EXTENSION_NAME
VULKAN_BASE_FUNCTIONS_VISIBILITY:
	PFN_vkCreateWin32SurfaceKHR CreateWin32SurfaceKHR;
public:
	inline VkResult createSurface(const cdm::vk::Win32SurfaceCreateInfoKHR& createInfo, VkSurfaceKHR& outSurface) const;
	inline VkResult create(const cdm::vk::Win32SurfaceCreateInfoKHR& createInfo, VkSurfaceKHR& outSurface) const;

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

class VulkanDevice final : public VulkanDeviceBase
{
	VkPhysicalDevice m_physicalDevice = nullptr;
	VkDevice m_device = nullptr;
	VkQueue m_graphicsQueue = nullptr;
	VkQueue m_presentQueue = nullptr;
	QueueFamilyIndices m_queueFamilyIndices;

public:
	VulkanDevice(bool layers = false) noexcept;
	~VulkanDevice() override;

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

	using VulkanDeviceBase::create;
	using VulkanDeviceBase::createSurface;
	using VulkanDeviceBase::destroy;
	using VulkanDeviceBase::destroySurface;

	// clang-format off
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

	// PFN_vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX
	// GetPhysicalDeviceGeneratedCommandsPropertiesNVX;

	// Device functions
VULKAN_FUNCTIONS_VISIBILITY:
	PFN_vkAllocateCommandBuffers AllocateCommandBuffers;
public:
	inline VkResult allocateCommandBuffers(const cdm::vk::CommandBufferAllocateInfo& allocateInfo, VkCommandBuffer* pCommandBuffers) const;
	inline VkResult allocate(const cdm::vk::CommandBufferAllocateInfo& allocateInfo, VkCommandBuffer* pCommandBuffers) const;

VULKAN_FUNCTIONS_VISIBILITY:
	PFN_vkAllocateDescriptorSets AllocateDescriptorSets;
public:
	inline VkResult allocateDescriptorSets(const cdm::vk::DescriptorSetAllocateInfo& allocateInfo, VkDescriptorSet* pDescriptorSets) const;
	inline VkResult allocate(const cdm::vk::DescriptorSetAllocateInfo& allocateInfo, VkDescriptorSet* pDescriptorSets) const;

VULKAN_FUNCTIONS_VISIBILITY:
	PFN_vkAllocateMemory AllocateMemory;
public:
	inline VkResult allocateMemory(const cdm::vk::MemoryAllocateInfo& allocateInfo, VkDeviceMemory& outDeviceMemory) const;
	inline VkResult allocate(const cdm::vk::MemoryAllocateInfo& allocateInfo, VkDeviceMemory& outDeviceMemory) const;

	PFN_vkBindBufferMemory BindBufferMemory;
	PFN_vkBindBufferMemory2 BindBufferMemory2;
	PFN_vkBindImageMemory BindImageMemory;
	PFN_vkBindImageMemory2 BindImageMemory2;

	PFN_vkBeginCommandBuffer BeginCommandBuffer;
	PFN_vkCmdBeginQuery CmdBeginQuery;
	PFN_vkCmdBeginRenderPass CmdBeginRenderPass;
	PFN_vkCmdBeginRenderPass2 CmdBeginRenderPass2;
	PFN_vkCmdBindDescriptorSets CmdBindDescriptorSets;
	PFN_vkCmdBindIndexBuffer CmdBindIndexBuffer;
	PFN_vkCmdBindPipeline CmdBindPipeline;
	PFN_vkCmdBindVertexBuffers CmdBindVertexBuffers;
	PFN_vkCmdBlitImage CmdBlitImage;
	PFN_vkCmdClearAttachments CmdClearAttachments;
	PFN_vkCmdClearColorImage CmdClearColorImage;
	PFN_vkCmdClearDepthStencilImage CmdClearDepthStencilImage;
	PFN_vkCmdCopyBuffer CmdCopyBuffer;
	PFN_vkCmdCopyBufferToImage CmdCopyBufferToImage;
	PFN_vkCmdCopyImage CmdCopyImage;
	PFN_vkCmdCopyImageToBuffer CmdCopyImageToBuffer;
	PFN_vkCmdCopyQueryPoolResults CmdCopyQueryPoolResults;
	PFN_vkCmdDispatch CmdDispatch;
	PFN_vkCmdDispatchBase CmdDispatchBase;
	PFN_vkCmdDispatchIndirect CmdDispatchIndirect;
	PFN_vkCmdDraw CmdDraw;
	PFN_vkCmdDrawIndexed CmdDrawIndexed;
	PFN_vkCmdDrawIndexedIndirect CmdDrawIndexedIndirect;
	PFN_vkCmdDrawIndexedIndirectCount CmdDrawIndexedIndirectCount;
	PFN_vkCmdDrawIndirect CmdDrawIndirect;
	PFN_vkCmdDrawIndirectCount CmdDrawIndirectCount;
	PFN_vkCmdEndQuery CmdEndQuery;
	PFN_vkCmdEndRenderPass CmdEndRenderPass;
	PFN_vkCmdEndRenderPass2 CmdEndRenderPass2;
	PFN_vkCmdExecuteCommands CmdExecuteCommands;
	PFN_vkCmdFillBuffer CmdFillBuffer;
	PFN_vkCmdNextSubpass CmdNextSubpass;
	PFN_vkCmdNextSubpass2 CmdNextSubpass2;
	PFN_vkCmdPipelineBarrier CmdPipelineBarrier;
	PFN_vkCmdPushConstants CmdPushConstants;
	PFN_vkCmdResetEvent CmdResetEvent;
	PFN_vkCmdResetQueryPool CmdResetQueryPool;
	PFN_vkCmdResolveImage CmdResolveImage;
	PFN_vkCmdSetBlendConstants CmdSetBlendConstants;
	PFN_vkCmdSetDepthBias CmdSetDepthBias;
	PFN_vkCmdSetDepthBounds CmdSetDepthBounds;
	PFN_vkCmdSetDeviceMask CmdSetDeviceMask;
	PFN_vkCmdSetEvent CmdSetEvent;
	PFN_vkCmdSetLineWidth CmdSetLineWidth;
	PFN_vkCmdSetScissor CmdSetScissor;
	PFN_vkCmdSetStencilCompareMask CmdSetStencilCompareMask;
	PFN_vkCmdSetStencilReference CmdSetStencilReference;
	PFN_vkCmdSetStencilWriteMask CmdSetStencilWriteMask;
	PFN_vkCmdSetViewport CmdSetViewport;
	PFN_vkCmdUpdateBuffer CmdUpdateBuffer;
	PFN_vkCmdWaitEvents CmdWaitEvents;
	PFN_vkCmdWriteTimestamp CmdWriteTimestamp;
	PFN_vkEndCommandBuffer EndCommandBuffer;
	PFN_vkResetCommandBuffer ResetCommandBuffer;


VULKAN_FUNCTIONS_VISIBILITY:
	PFN_vkCreateComputePipelines CreateComputePipelines;
public:
	inline VkResult createComputePipelines(uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines, VkPipelineCache pipelineCache = nullptr) const;
	inline VkResult create(uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines, VkPipelineCache pipelineCache = nullptr) const;
	inline VkResult createComputePipelines(uint32_t createInfoCount, const cdm::vk::ComputePipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines, VkPipelineCache pipelineCache = nullptr) const;
	inline VkResult create(uint32_t createInfoCount, const cdm::vk::ComputePipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines, VkPipelineCache pipelineCache = nullptr) const;
	inline VkResult createComputePipeline(const cdm::vk::ComputePipelineCreateInfo& createInfo, VkPipeline& outPipeline, VkPipelineCache pipelineCache = nullptr) const;
	inline VkResult create(const cdm::vk::ComputePipelineCreateInfo& createInfo, VkPipeline& outPipeline, VkPipelineCache pipelineCache = nullptr) const;

VULKAN_FUNCTIONS_VISIBILITY:
	PFN_vkCreateGraphicsPipelines CreateGraphicsPipelines;
public:
	inline VkResult createGraphicsPipelines(uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines, VkPipelineCache pipelineCache = nullptr) const;
	inline VkResult create(uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines, VkPipelineCache pipelineCache = nullptr) const;
	inline VkResult createGraphicsPipelines(uint32_t createInfoCount, const cdm::vk::GraphicsPipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines, VkPipelineCache pipelineCache = nullptr) const;
	inline VkResult create(uint32_t createInfoCount, const cdm::vk::GraphicsPipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines, VkPipelineCache pipelineCache = nullptr) const;
	inline VkResult createGraphicsPipeline(const cdm::vk::GraphicsPipelineCreateInfo& createInfo, VkPipeline& outPipeline, VkPipelineCache pipelineCache = nullptr) const;
	inline VkResult create(const cdm::vk::GraphicsPipelineCreateInfo& createInfo, VkPipeline& outPipeline, VkPipelineCache pipelineCache = nullptr) const;

VULKAN_FUNCTIONS_VISIBILITY:
	PFN_vkCreateRenderPass2 CreateRenderPass2;
public:
	inline VkResult createRenderPass2(const cdm::vk::RenderPassCreateInfo2& createInfo, VkRenderPass& outRenderPass) const;
	inline VkResult create(const cdm::vk::RenderPassCreateInfo2& createInfo, VkRenderPass& outRenderPass) const;

#define DEFINE_DEVICE_CREATE(ObjectName)                                                                                    \
VULKAN_FUNCTIONS_VISIBILITY:                                                                                                \
	PFN_vkCreate##ObjectName Create##ObjectName;                                                                            \
public:                                                                                                                     \
	VkResult create##ObjectName(const cdm::vk::##ObjectName##CreateInfo& createInfo, Vk##ObjectName& out##ObjectName) const \
	{                                                                                                                       \
		return Create##ObjectName(vkDevice(), &createInfo, nullptr, &out##ObjectName);                                      \
	}                                                                                                                       \
	VkResult create(const cdm::vk::##ObjectName##CreateInfo& createInfo, Vk##ObjectName& out##ObjectName) const             \
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

VULKAN_FUNCTIONS_VISIBILITY:
	PFN_vkDestroyDevice DestroyDevice;
public:
	inline void destroyDevice() const;
	inline void destroy() const;

#define DEFINE_DEVICE_DESTROY(ObjectName)                        \
VULKAN_FUNCTIONS_VISIBILITY:                                     \
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

VULKAN_FUNCTIONS_VISIBILITY:
	PFN_vkDeviceWaitIdle DeviceWaitIdle;
public:
	inline VkResult waitIdle() const;
	inline VkResult wait() const;

	PFN_vkEnumerateInstanceVersion EnumerateInstanceVersion;
	PFN_vkFlushMappedMemoryRanges FlushMappedMemoryRanges;

VULKAN_FUNCTIONS_VISIBILITY:
	PFN_vkFreeCommandBuffers FreeCommandBuffers;
public:
	inline void freeCommandBuffers(VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers) const;
	inline void free(VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers) const;
	inline void freeCommandBuffer(VkCommandPool commandPool, VkCommandBuffer CommandBuffer) const;
	inline void free(VkCommandPool commandPool, VkCommandBuffer CommandBuffer) const;

VULKAN_FUNCTIONS_VISIBILITY:
	PFN_vkFreeDescriptorSets FreeDescriptorSets;
public:
	inline VkResult freeDescriptorSets(VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets) const;
	inline VkResult free(VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets) const;
	inline VkResult freeDescriptorSets(VkDescriptorPool descriptorPool, VkDescriptorSet DescriptorSet) const;
	inline VkResult free(VkDescriptorPool descriptorPool, VkDescriptorSet DescriptorSet) const;

VULKAN_FUNCTIONS_VISIBILITY:
	PFN_vkFreeMemory FreeMemory;
public:
	inline void freeMemory(VkDeviceMemory aDeviceMemory) const;
	inline void free(VkDeviceMemory aDeviceMemory) const;

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

VULKAN_FUNCTIONS_VISIBILITY:
	PFN_vkQueueWaitIdle QueueWaitIdle;
public:
	inline VkResult queueWaitIdle(VkQueue queue) const;
	inline VkResult waitIdle(VkQueue queue) const;
	inline VkResult wait(VkQueue queue) const;

	PFN_vkResetCommandPool ResetCommandPool;
	PFN_vkResetDescriptorPool ResetDescriptorPool;
	PFN_vkResetEvent ResetEvent;
	PFN_vkResetFences ResetFences;
	PFN_vkResetQueryPool ResetQueryPool;
	PFN_vkSetEvent SetEvent;
	PFN_vkSignalSemaphore SignalSemaphore;
	PFN_vkTrimCommandPool TrimCommandPool;
	PFN_vkUnmapMemory UnmapMemory;
	PFN_vkUpdateDescriptorSets UpdateDescriptorSets;
	PFN_vkUpdateDescriptorSetWithTemplate UpdateDescriptorSetWithTemplate;

VULKAN_FUNCTIONS_VISIBILITY:
	PFN_vkWaitForFences WaitForFences;
public:
	inline VkResult waitForFences(uint32_t fenceCount, const VkFence* pFences, bool waitAll, uint64_t timeout = UINT64_MAX) const;
	inline VkResult wait(uint32_t fenceCount, const VkFence* pFences, bool waitAll, uint64_t timeout = UINT64_MAX) const;
	inline VkResult waitForFence(VkFence fence, uint64_t timeout = UINT64_MAX) const;
	inline VkResult wait(VkFence fence, uint64_t timeout = UINT64_MAX) const;

VULKAN_FUNCTIONS_VISIBILITY:
	PFN_vkWaitSemaphores WaitSemaphores;
public:
	inline VkResult waitSemaphores(cdm::vk::SemaphoreWaitInfo& waitInfo, uint64_t timeout = UINT64_MAX) const;
	inline VkResult wait(cdm::vk::SemaphoreWaitInfo& waitInfo, uint64_t timeout = UINT64_MAX) const;

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

VULKAN_FUNCTIONS_VISIBILITY:
	PFN_vkCreateSwapchainKHR CreateSwapchainKHR = nullptr;
public:
	inline VkResult createSwapchain(const cdm::vk::SwapchainCreateInfoKHR& createInfo, VkSwapchainKHR& outSwapchain) const;
	inline VkResult create(const cdm::vk::SwapchainCreateInfoKHR& createInfo, VkSwapchainKHR& outSwapchain) const;

VULKAN_FUNCTIONS_VISIBILITY:
	PFN_vkDestroySwapchainKHR DestroySwapchainKHR = nullptr;
public:
	inline void destroySwapchain(VkSwapchainKHR swapchain) const;
	inline void destroy(VkSwapchainKHR swapchain) const;

	PFN_vkGetDeviceGroupPresentCapabilitiesKHR GetDeviceGroupPresentCapabilitiesKHR = nullptr;
	PFN_vkGetDeviceGroupSurfacePresentModesKHR GetDeviceGroupSurfacePresentModesKHR = nullptr;
	// PFN_vkGetPhysicalDevicePresentRectanglesKHR GetPhysicalDevicePresentRectanglesKHR = nullptr;
	PFN_vkGetSwapchainImagesKHR GetSwapchainImagesKHR = nullptr;
	PFN_vkQueuePresentKHR QueuePresentKHR = nullptr;

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

	PFN_vkCmdProcessCommandsNVX CmdProcessCommandsNVX = nullptr;
	PFN_vkCmdReserveSpaceForCommandsNVX CmdReserveSpaceForCommandsNVX = nullptr;
	PFN_vkCreateIndirectCommandsLayoutNVX CreateIndirectCommandsLayoutNVX = nullptr;
	PFN_vkCreateObjectTableNVX CreateObjectTableNVX = nullptr;
	PFN_vkDestroyIndirectCommandsLayoutNVX DestroyIndirectCommandsLayoutNVX = nullptr;
	PFN_vkDestroyObjectTableNVX DestroyObjectTableNVX = nullptr;
	// PFN_vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX
	// GetPhysicalDeviceGeneratedCommandsPropertiesNVX = nullptr;
	PFN_vkRegisterObjectsNVX RegisterObjectsNVX = nullptr;
	PFN_vkUnregisterObjectsNVX UnregisterObjectsNVX = nullptr;

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

	PFN_vkCmdSetCheckpointNV CmdSetCheckpointNV = nullptr;
	PFN_vkGetQueueCheckpointDataNV GetQueueCheckpointDataNV = nullptr;

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

class VulkanDeviceObject
{
	std::reference_wrapper<const VulkanDevice> vkDevice;
	double m_creationTime = 0.0;

protected:
	void setCreationTime();
	//void resetCreationTime() { m_creationTime = 0.0; }

public:
	VulkanDeviceObject(const VulkanDevice& device) : vkDevice(device) {}

	const VulkanDevice& device() const { return vkDevice.get(); }
	double creationTime() const { return m_creationTime; }
};
}  // namespace cdm

#include "VulkanDevice.inl"
