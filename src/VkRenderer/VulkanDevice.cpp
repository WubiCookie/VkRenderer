#include "VulkanDevice.hpp"

#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include "cdm_vulkan.hpp"

#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

namespace cdm
{
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDeviceBase::DebugReportCallback(
    VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
    uint64_t object, size_t location, int32_t messageCode,
    const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
	if (LogActive)
		std::cerr << "report: " << pLayerPrefix << ": " << pMessage
		          << std::endl;

	return false;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDeviceBase::DebugUtilsMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	if (LogActive)
	{
		if (pCallbackData->pMessageIdName && pCallbackData->pMessage)
			std::cerr << "messenger: " << pCallbackData->pMessageIdName << ": "
			          << pCallbackData->pMessage << std::endl;
		else if (pCallbackData->pMessage)
			std::cerr << "messenger: " << pCallbackData->pMessage << std::endl;
	}

	return false;
}

VulkanDeviceBase::VulkanDeviceBase(bool layers) noexcept : m_layers(layers)
{
	VulkanLibrary = LoadLibraryW(L"vulkan-1.dll");

	if (VulkanLibrary == nullptr)
	{
		std::cerr << "error: could not load Vulkan library" << std::endl;
		exit(1);
	}

	GetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(
	    VulkanLibrary, "vkGetInstanceProcAddr");
	if (!GetInstanceProcAddr)
	{
		std::cerr << "error: could not load vkGetInstanceProcAddr"
		          << std::endl;
		exit(1);
	}

#define LOAD(func)                                                            \
	func = (PFN_vk##func)GetInstanceProcAddr(nullptr, "vk" #func);            \
	if (!func)                                                                \
	{                                                                         \
		std::cerr << "Could not load vk" #func "!" << std::endl;              \
		exit(1);                                                              \
	}

	LOAD(CreateInstance);
	LOAD(EnumerateInstanceExtensionProperties);
	LOAD(EnumerateInstanceLayerProperties);

#undef LOAD

	vk::ApplicationInfo appInfo;
	appInfo.apiVersion = VK_API_VERSION_1_2;
	appInfo.applicationVersion = 0;
	appInfo.engineVersion = 0;
	appInfo.pApplicationName = "VkRenderer";
	appInfo.pEngineName = "VkRenderer";

	std::vector<const char*> validationLayers;

	if (layers)
	{
		validationLayers.push_back("VK_LAYER_KHRONOS_validation");
		//validationLayers.push_back("VK_LAYER_LUNARG_standard_validation");
		 //validationLayers.push_back("VK_LAYER_LUNARG_api_dump");

		uint32_t layerCount;
		EnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		EnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				std::cerr << "false" << std::endl;
			}
		}
	}

	std::vector<const char*> instanceExtensions;
	instanceExtensions.push_back(
	    VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
	instanceExtensions.push_back(
	    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	vk::InstanceCreateInfo instanceInfo = {};
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledLayerCount = uint32_t(validationLayers.size());
	instanceInfo.ppEnabledLayerNames = validationLayers.data();
	instanceInfo.enabledExtensionCount = uint32_t(instanceExtensions.size());
	instanceInfo.ppEnabledExtensionNames = instanceExtensions.data();

	if (CreateInstance(&instanceInfo, nullptr, &m_instance) != VK_SUCCESS)
	{
		std::cerr << "error: vulkan instance not created" << std::endl;
		exit(1);
	}

#define LOAD(func)                                                            \
	func = (PFN_vk##func)GetInstanceProcAddr(instance(), "vk" #func);         \
	if (!func)                                                                \
	{                                                                         \
		std::cerr << "Could not load vk" #func "!" << std::endl;              \
		exit(1);                                                              \
	}

	LOAD(CreateDevice);
	LOAD(DestroyInstance);
	LOAD(EnumerateDeviceExtensionProperties);
	LOAD(EnumerateDeviceLayerProperties);
	LOAD(EnumeratePhysicalDeviceGroups);
	LOAD(EnumeratePhysicalDevices);
	LOAD(GetDeviceProcAddr);
	LOAD(GetInstanceProcAddr);
	LOAD(GetPhysicalDeviceExternalBufferProperties);
	LOAD(GetPhysicalDeviceExternalFenceProperties);
	LOAD(GetPhysicalDeviceExternalSemaphoreProperties);
	LOAD(GetPhysicalDeviceFeatures);
	LOAD(GetPhysicalDeviceFeatures2);
	LOAD(GetPhysicalDeviceFormatProperties);
	LOAD(GetPhysicalDeviceFormatProperties2);
	// LOAD(GetPhysicalDeviceGeneratedCommandsPropertiesNVX);
	LOAD(GetPhysicalDeviceImageFormatProperties);
	LOAD(GetPhysicalDeviceImageFormatProperties2);
	LOAD(GetPhysicalDeviceMemoryProperties);
	LOAD(GetPhysicalDeviceMemoryProperties2);
	LOAD(GetPhysicalDeviceProperties);
	LOAD(GetPhysicalDeviceProperties2);
	LOAD(GetPhysicalDeviceQueueFamilyProperties);
	LOAD(GetPhysicalDeviceQueueFamilyProperties2);
	LOAD(GetPhysicalDeviceSparseImageFormatProperties);
	LOAD(GetPhysicalDeviceSparseImageFormatProperties2);

	uint32_t extensionCount;
	std::vector<VkExtensionProperties> availableExtensions;

	EnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	availableExtensions.resize(extensionCount);
	EnumerateInstanceExtensionProperties(nullptr, &extensionCount,
	                                     availableExtensions.data());

	bool khrSurfaceFound = false;
	bool khrWin32SurfaceFound = false;
	bool khrGetSurfaceCapabilities2Found = false;
	bool khrGetPhysicalDeviceProperties2Found = false;
	bool extDebugReportFound = false;
	bool extDebugUtilsFound = false;
	for (const auto& ext : availableExtensions)
	{
		std::string_view name(ext.extensionName);
		if (name == VK_KHR_SURFACE_EXTENSION_NAME)
			khrSurfaceFound = true;
		else if (name == VK_KHR_WIN32_SURFACE_EXTENSION_NAME)
			khrWin32SurfaceFound = true;
		else if (name == VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME)
			khrGetSurfaceCapabilities2Found = true;
		else if (name ==
		         VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)
			khrGetPhysicalDeviceProperties2Found = true;
		else if (name == VK_EXT_DEBUG_REPORT_EXTENSION_NAME)
			extDebugReportFound = true;
		else if (name == VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
			extDebugUtilsFound = true;
	}

	if (khrSurfaceFound)
	{
		LOAD(DestroySurfaceKHR);
		LOAD(GetPhysicalDeviceSurfaceSupportKHR);
		LOAD(GetPhysicalDeviceSurfaceCapabilitiesKHR);
		LOAD(GetPhysicalDeviceSurfaceFormatsKHR);
		LOAD(GetPhysicalDeviceSurfacePresentModesKHR);
	}
	if (khrWin32SurfaceFound)
	{
		LOAD(CreateWin32SurfaceKHR);
		LOAD(GetPhysicalDeviceWin32PresentationSupportKHR);
	}
	if (khrGetSurfaceCapabilities2Found)
	{
		LOAD(GetPhysicalDeviceSurfaceCapabilities2KHR);
		LOAD(GetPhysicalDeviceSurfaceFormats2KHR);
	}
	if (khrGetPhysicalDeviceProperties2Found)
	{
		LOAD(GetPhysicalDeviceFeatures2KHR);
		LOAD(GetPhysicalDeviceFormatProperties2KHR);
		LOAD(GetPhysicalDeviceImageFormatProperties2KHR);
		LOAD(GetPhysicalDeviceMemoryProperties2KHR);
		LOAD(GetPhysicalDeviceProperties2KHR);
		LOAD(GetPhysicalDeviceQueueFamilyProperties2KHR);
		LOAD(GetPhysicalDeviceSparseImageFormatProperties2KHR);
	}
	if (extDebugReportFound)
	{
		LOAD(CreateDebugReportCallbackEXT);
		LOAD(DebugReportMessageEXT);
		LOAD(DestroyDebugReportCallbackEXT);
	}
	if (extDebugUtilsFound)
	{
		LOAD(CreateDebugUtilsMessengerEXT);
		LOAD(DestroyDebugUtilsMessengerEXT);
		LOAD(SubmitDebugUtilsMessageEXT);
	}

	// LOAD(CreateDebugReportCallbackEXT);
	// LOAD(CreateDebugUtilsMessengerEXT);
	// LOAD(CreateDisplayModeKHR);
	// LOAD(CreateDisplayPlaneSurfaceKHR);
	// LOAD(CreateHeadlessSurfaceEXT);
	// LOAD(DebugReportMessageEXT);
	// LOAD(DestroyDebugReportCallbackEXT);
	// LOAD(DestroyDebugUtilsMessengerEXT);
	// LOAD(DestroySurfaceKHR);
	// LOAD(EnumeratePhysicalDeviceGroupsKHR);
	// LOAD(EnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR);
	// LOAD(GetDisplayModeProperties2KHR);
	// LOAD(GetDisplayModePropertiesKHR);
	// LOAD(GetDisplayPlaneCapabilities2KHR);
	// LOAD(GetDisplayPlaneCapabilitiesKHR);
	// LOAD(GetDisplayPlaneSupportedDisplaysKHR);
	// LOAD(GetPhysicalDeviceCalibrateableTimeDomainsEXT);
	// LOAD(GetPhysicalDeviceCooperativeMatrixPropertiesNV);
	// LOAD(GetPhysicalDeviceDisplayPlaneProperties2KHR);
	// LOAD(GetPhysicalDeviceDisplayPlanePropertiesKHR);
	// LOAD(GetPhysicalDeviceDisplayProperties2KHR);
	// LOAD(GetPhysicalDeviceDisplayPropertiesKHR);
	// LOAD(GetPhysicalDeviceExternalBufferPropertiesKHR);
	// LOAD(GetPhysicalDeviceExternalFencePropertiesKHR);
	// LOAD(GetPhysicalDeviceExternalImageFormatPropertiesNV);
	// LOAD(GetPhysicalDeviceExternalSemaphorePropertiesKHR);
	// LOAD(GetPhysicalDeviceFeatures2KHR);
	// LOAD(GetPhysicalDeviceFormatProperties2KHR);
	// LOAD(GetPhysicalDeviceImageFormatProperties2KHR);
	// LOAD(GetPhysicalDeviceMemoryProperties2KHR);
	// LOAD(GetPhysicalDeviceMultisamplePropertiesEXT);
	// LOAD(GetPhysicalDevicePresentRectanglesKHR);
	// LOAD(GetPhysicalDeviceProperties2KHR);
	// LOAD(GetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR);
	// LOAD(GetPhysicalDeviceQueueFamilyProperties2KHR);
	// LOAD(GetPhysicalDeviceSparseImageFormatProperties2KHR);
	// LOAD(GetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV);
	// LOAD(GetPhysicalDeviceSurfaceCapabilities2EXT);
	// LOAD(GetPhysicalDeviceSurfaceCapabilities2KHR);
	// LOAD(GetPhysicalDeviceSurfaceCapabilitiesKHR);
	// LOAD(GetPhysicalDeviceSurfaceFormats2KHR);
	// LOAD(GetPhysicalDeviceSurfaceFormatsKHR);
	// LOAD(GetPhysicalDeviceSurfacePresentModesKHR);
	// LOAD(GetPhysicalDeviceSurfaceSupportKHR);
	// LOAD(GetPhysicalDeviceToolPropertiesEXT);
	// LOAD(ReleaseDisplayEXT);
	// LOAD(SubmitDebugUtilsMessageEXT);

#undef LOAD

	if (CreateDebugUtilsMessengerEXT)
	{
		vk::DebugUtilsMessengerCreateInfoEXT debugInfo;
		debugInfo.messageSeverity =
		    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
		    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
		debugInfo.messageType =
		    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugInfo.pfnUserCallback =
		    VulkanDeviceBase::DebugUtilsMessengerCallback;
		debugInfo.pUserData = nullptr;  // Optional

		CreateDebugUtilsMessengerEXT(instance(), &debugInfo, nullptr,
		                             &m_messenger);
	}
}

VulkanDeviceBase::~VulkanDeviceBase()
{
	if (m_messenger && DestroyDebugUtilsMessengerEXT)
	{
		DestroyDebugUtilsMessengerEXT(instance(), m_messenger, nullptr);
	}
	if (m_callback && DestroyDebugReportCallbackEXT)
	{
		DestroyDebugReportCallbackEXT(instance(), m_callback, nullptr);
	}
	if (m_instance && DestroyInstance)
	{
		DestroyInstance(m_instance, nullptr);
	}
}

// ================================================================

static int rateDeviceSuitability(VkPhysicalDevice physicalDevice,
                                 const VulkanDevice& vk)
{
	VkPhysicalDeviceProperties deviceProperties;
	vk.GetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;
	vk.GetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

	int score = 0;

	// Discrete GPUs have a significant performance advantage
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		score += 1000;
	}

	//   // Maximum possible size of textures affects graphics quality
	//   score += deviceProperties.limits.maxImageDimension2D;

	//   // Application can't function without geometry shaders
	//   if (!deviceFeatures.geometryShader)
	//{
	//       return 0;
	//   }

	return score;
}

static void pickPhysicalDevice(
    const std::vector<VkPhysicalDevice>& physicalDevices,
    VkPhysicalDevice& physicalDevice, const VulkanDevice& vk)
{
	// Use an ordered map to automatically sort candidates by increasing score
	std::multimap<int, VkPhysicalDevice> candidates;

	for (const auto& physicalDevice : physicalDevices)
	{
		int score = rateDeviceSuitability(physicalDevice, vk);
		candidates.insert(std::make_pair(score, physicalDevice));
	}

	// Check if the best candidate is suitable at all
	if (candidates.rbegin()->first > 0)
	{
		physicalDevice = candidates.rbegin()->second;
	}
	else
	{
		std::cerr << "error: failed to find a suitable GPU" << std::endl;
		exit(1);
	}
}

VulkanDevice::VulkanDevice(bool layers) noexcept : VulkanDeviceBase(layers)
{
	uint32_t deviceCount = 0;
	EnumeratePhysicalDevices(instance(), &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		std::cerr << "error: failed to find GPUs with Vulkan support"
		          << std::endl;
		exit(1);
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	EnumeratePhysicalDevices(instance(), &deviceCount, devices.data());

	pickPhysicalDevice(devices, m_physicalDevice, *this);

	if (m_physicalDevice == nullptr)
	{
		std::cerr << "error: failed to find a suitable GPU" << std::endl;
		exit(1);
	}
}

VulkanDevice::~VulkanDevice()
{
	if (m_device && DestroyDevice)
	{
		DestroyDevice(m_device, nullptr);
	}
}

void VulkanDevice::createDevice(VkSurfaceKHR surface,
                                QueueFamilyIndices queueFamilyIndices)
{
	m_queueFamilyIndices = queueFamilyIndices;

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {
		m_queueFamilyIndices.graphicsFamily.value(),
		m_queueFamilyIndices.presentFamily.value()
	};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		vk::DeviceQueueCreateInfo queueCreateInfo;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	uint32_t extensionCount;
	EnumerateDeviceExtensionProperties(physicalDevice(), nullptr,
	                                   &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	EnumerateDeviceExtensionProperties(physicalDevice(), nullptr,
	                                   &extensionCount,
	                                   availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(),
	                                         deviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	if (!requiredExtensions.empty())
	{
		std::cerr << "error: requiered extension(s) not present" << std::endl;
		for (auto& ext : requiredExtensions)
			std::cerr << "    " << ext << std::endl;
		exit(1);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.shaderFloat64 = true;

	vk::DeviceCreateInfo createInfo;
	createInfo.queueCreateInfoCount = uint32_t(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = uint32_t(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (m_layers)
	{
		std::vector<const char*> validationLayers;
		validationLayers.push_back("VK_LAYER_KHRONOS_validation");

		// createInfo.enabledLayerCount = uint32_t(validationLayers.size());
		// createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (CreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) !=
	    VK_SUCCESS)
	{
		std::cerr << "error: failed to create logical device" << std::endl;
		exit(1);
	}

#define LOAD(func)                                                            \
	func = (PFN_vk##func)GetInstanceProcAddr(instance(), "vk" #func);         \
	if (!func)                                                                \
	{                                                                         \
		std::cerr << "Could not load vk" #func "!" << std::endl;              \
		exit(1);                                                              \
	}

	LOAD(CreateDevice);
	LOAD(DestroyInstance);
	LOAD(EnumerateDeviceExtensionProperties);
	LOAD(EnumerateDeviceLayerProperties);
	LOAD(EnumeratePhysicalDeviceGroups);
	LOAD(EnumeratePhysicalDevices);
	LOAD(GetDeviceProcAddr);
	LOAD(GetInstanceProcAddr);
	LOAD(GetPhysicalDeviceExternalBufferProperties);
	LOAD(GetPhysicalDeviceExternalFenceProperties);
	LOAD(GetPhysicalDeviceExternalSemaphoreProperties);
	LOAD(GetPhysicalDeviceFeatures);
	LOAD(GetPhysicalDeviceFeatures2);
	LOAD(GetPhysicalDeviceFormatProperties);
	LOAD(GetPhysicalDeviceFormatProperties2);
	// LOAD(GetPhysicalDeviceGeneratedCommandsPropertiesNVX);
	LOAD(GetPhysicalDeviceImageFormatProperties);
	LOAD(GetPhysicalDeviceImageFormatProperties2);
	LOAD(GetPhysicalDeviceMemoryProperties);
	LOAD(GetPhysicalDeviceMemoryProperties2);
	LOAD(GetPhysicalDeviceProperties);
	LOAD(GetPhysicalDeviceProperties2);
	LOAD(GetPhysicalDeviceQueueFamilyProperties);
	LOAD(GetPhysicalDeviceQueueFamilyProperties2);
	LOAD(GetPhysicalDeviceSparseImageFormatProperties);
	LOAD(GetPhysicalDeviceSparseImageFormatProperties2);
	LOAD(DestroySurfaceKHR);
	LOAD(GetPhysicalDeviceSurfaceSupportKHR);
	LOAD(GetPhysicalDeviceSurfaceCapabilitiesKHR);
	LOAD(GetPhysicalDeviceSurfaceFormatsKHR);
	LOAD(GetPhysicalDeviceSurfacePresentModesKHR);
	LOAD(CreateWin32SurfaceKHR);
	LOAD(GetPhysicalDeviceWin32PresentationSupportKHR);
	LOAD(GetPhysicalDeviceSurfaceCapabilities2KHR);
	LOAD(GetPhysicalDeviceSurfaceFormats2KHR);
	LOAD(GetPhysicalDeviceFeatures2KHR);
	LOAD(GetPhysicalDeviceFormatProperties2KHR);
	LOAD(GetPhysicalDeviceImageFormatProperties2KHR);
	LOAD(GetPhysicalDeviceMemoryProperties2KHR);
	LOAD(GetPhysicalDeviceProperties2KHR);
	LOAD(GetPhysicalDeviceQueueFamilyProperties2KHR);
	LOAD(GetPhysicalDeviceSparseImageFormatProperties2KHR);
	LOAD(CreateDebugReportCallbackEXT);
	LOAD(DebugReportMessageEXT);
	LOAD(DestroyDebugReportCallbackEXT);
	LOAD(CreateDebugUtilsMessengerEXT);
	LOAD(DestroyDebugUtilsMessengerEXT);
	LOAD(SubmitDebugUtilsMessageEXT);

#undef LOAD
#define LOAD(func)                                                            \
	func = (PFN_vk##func)GetDeviceProcAddr(vkDevice(), "vk" #func);           \
	if (!func)                                                                \
	{                                                                         \
		std::cerr << "Could not load vk" #func "!" << std::endl;              \
		exit(1);                                                              \
	}

	LOAD(AllocateCommandBuffers);
	LOAD(AllocateDescriptorSets);
	LOAD(AllocateMemory);
	LOAD(BeginCommandBuffer);
	LOAD(BindBufferMemory);
	LOAD(BindBufferMemory2);
	LOAD(BindImageMemory);
	LOAD(BindImageMemory2);

	LOAD(CmdBeginQuery);
	LOAD(CmdBeginRenderPass);
	// LOAD(CmdBeginRenderPass2);
	LOAD(CmdBindDescriptorSets);
	LOAD(CmdBindIndexBuffer);
	LOAD(CmdBindPipeline);
	LOAD(CmdBindVertexBuffers);
	LOAD(CmdBlitImage);
	LOAD(CmdClearAttachments);
	LOAD(CmdClearColorImage);
	LOAD(CmdClearDepthStencilImage);
	LOAD(CmdCopyBuffer);
	LOAD(CmdCopyBufferToImage);
	LOAD(CmdCopyImage);
	LOAD(CmdCopyImageToBuffer);
	LOAD(CmdCopyQueryPoolResults);
	LOAD(CmdDispatch);
	LOAD(CmdDispatchBase);
	LOAD(CmdDispatchIndirect);
	LOAD(CmdDraw);
	LOAD(CmdDrawIndexed);
	LOAD(CmdDrawIndexedIndirect);
	// LOAD(CmdDrawIndexedIndirectCount);
	LOAD(CmdDrawIndirect);
	// LOAD(CmdDrawIndirectCount);
	LOAD(CmdEndQuery);
	LOAD(CmdEndRenderPass);
	// LOAD(CmdEndRenderPass2);
	LOAD(CmdExecuteCommands);
	LOAD(CmdFillBuffer);
	LOAD(CmdNextSubpass);
	// LOAD(CmdNextSubpass2);
	LOAD(CmdPipelineBarrier);
	LOAD(CmdPushConstants);
	LOAD(CmdResetEvent);
	LOAD(CmdResetQueryPool);
	LOAD(CmdResolveImage);
	LOAD(CmdSetBlendConstants);
	LOAD(CmdSetDepthBias);
	LOAD(CmdSetDepthBounds);
	LOAD(CmdSetDeviceMask);
	LOAD(CmdSetEvent);
	LOAD(CmdSetLineWidth);
	LOAD(CmdSetScissor);
	LOAD(CmdSetStencilCompareMask);
	LOAD(CmdSetStencilReference);
	LOAD(CmdSetStencilWriteMask);
	LOAD(CmdSetViewport);
	LOAD(CmdUpdateBuffer);
	LOAD(CmdWaitEvents);
	LOAD(CmdWriteTimestamp);

	LOAD(CreateBuffer);
	LOAD(CreateBufferView);
	LOAD(CreateCommandPool);
	LOAD(CreateComputePipelines);
	LOAD(CreateDescriptorPool);
	LOAD(CreateDescriptorSetLayout);
	LOAD(CreateDescriptorUpdateTemplate);
	LOAD(CreateEvent);
	LOAD(CreateFence);
	LOAD(CreateFramebuffer);
	LOAD(CreateGraphicsPipelines);
	LOAD(CreateImage);
	LOAD(CreateImageView);
	LOAD(CreatePipelineCache);
	LOAD(CreatePipelineLayout);
	LOAD(CreateQueryPool);
	LOAD(CreateRenderPass);
	// LOAD(CreateRenderPass2);
	LOAD(CreateSampler);
	LOAD(CreateSamplerYcbcrConversion);
	LOAD(CreateSemaphore);
	LOAD(CreateShaderModule);
	LOAD(DestroyBuffer);
	LOAD(DestroyBufferView);
	LOAD(DestroyCommandPool);
	LOAD(DestroyDescriptorPool);
	LOAD(DestroyDescriptorSetLayout);
	LOAD(DestroyDescriptorUpdateTemplate);
	LOAD(DestroyDevice);
	LOAD(DestroyEvent);
	LOAD(DestroyFence);
	LOAD(DestroyFramebuffer);
	LOAD(DestroyImage);
	LOAD(DestroyImageView);
	LOAD(DestroyPipeline);
	LOAD(DestroyPipelineCache);
	LOAD(DestroyPipelineLayout);
	LOAD(DestroyQueryPool);
	LOAD(DestroyRenderPass);
	LOAD(DestroySampler);
	LOAD(DestroySamplerYcbcrConversion);
	LOAD(DestroySemaphore);
	LOAD(DestroyShaderModule);
	LOAD(DeviceWaitIdle);
	LOAD(EndCommandBuffer);
	// LOAD(EnumerateDeviceExtensionProperties);
	// LOAD(EnumerateDeviceLayerProperties);
	// LOAD(EnumerateInstanceVersion);
	// LOAD(EnumeratePhysicalDeviceGroups);
	LOAD(FlushMappedMemoryRanges);
	LOAD(FreeCommandBuffers);
	LOAD(FreeDescriptorSets);
	LOAD(FreeMemory);
	// LOAD(GetBufferDeviceAddress);
	LOAD(GetBufferMemoryRequirements);
	// LOAD(GetBufferMemoryRequirements2);
	// LOAD(GetBufferOpaqueCaptureAddress);
	LOAD(GetDescriptorSetLayoutSupport);
	LOAD(GetDeviceGroupPeerMemoryFeatures);
	LOAD(GetDeviceMemoryCommitment);
	// LOAD(GetDeviceMemoryOpaqueCaptureAddress);
	LOAD(GetDeviceQueue);
	// LOAD(GetDeviceQueue2);
	LOAD(GetEventStatus);
	LOAD(GetFenceStatus);
	LOAD(GetImageMemoryRequirements);
	// LOAD(GetImageMemoryRequirements2);
	LOAD(GetImageSparseMemoryRequirements);
	// LOAD(GetImageSparseMemoryRequirements2);
	LOAD(GetImageSubresourceLayout);
	// LOAD(GetPhysicalDeviceExternalBufferProperties);
	// LOAD(GetPhysicalDeviceExternalFenceProperties);
	// LOAD(GetPhysicalDeviceExternalSemaphoreProperties);
	// LOAD(GetPhysicalDeviceFeatures2);
	// LOAD(GetPhysicalDeviceFormatProperties2);
	// LOAD(GetPhysicalDeviceImageFormatProperties2);
	// LOAD(GetPhysicalDeviceMemoryProperties2);
	// LOAD(GetPhysicalDeviceProperties2);
	// LOAD(GetPhysicalDeviceQueueFamilyProperties2);
	// LOAD(GetPhysicalDeviceSparseImageFormatProperties);
	// LOAD(GetPhysicalDeviceSparseImageFormatProperties2);
	LOAD(GetPipelineCacheData);
	LOAD(GetQueryPoolResults);
	LOAD(GetRenderAreaGranularity);
	// LOAD(GetSemaphoreCounterValue);
	LOAD(InvalidateMappedMemoryRanges);
	LOAD(MapMemory);
	LOAD(MergePipelineCaches);
	LOAD(QueueBindSparse);
	LOAD(QueueSubmit);
	LOAD(QueueWaitIdle);
	LOAD(ResetCommandBuffer);
	LOAD(ResetCommandPool);
	LOAD(ResetDescriptorPool);
	LOAD(ResetEvent);
	LOAD(ResetFences);
	// LOAD(ResetQueryPool);
	LOAD(SetEvent);
	// LOAD(SignalSemaphore);
	LOAD(TrimCommandPool);
	LOAD(UnmapMemory);
	LOAD(UpdateDescriptorSets);
	LOAD(UpdateDescriptorSetWithTemplate);
	LOAD(WaitForFences);
	// LOAD(WaitSemaphores);

	uint32_t extensionCount2;
	std::vector<VkExtensionProperties> availableExtensions2;

	EnumerateDeviceExtensionProperties(physicalDevice(), nullptr,
	                                   &extensionCount2, nullptr);
	availableExtensions2.resize(extensionCount2);
	EnumerateDeviceExtensionProperties(physicalDevice(), nullptr,
	                                   &extensionCount2,
	                                   availableExtensions2.data());

	bool khrSwapchainFound = false;
	for (const auto& ext : availableExtensions2)
	{
		std::string_view name(ext.extensionName);
		if (name == VK_KHR_SWAPCHAIN_EXTENSION_NAME)
			khrSwapchainFound = true;
	}

	if (khrSwapchainFound)
	{
		LOAD(AcquireNextImage2KHR);
		LOAD(AcquireNextImageKHR);
		LOAD(CreateSwapchainKHR);
		LOAD(DestroySwapchainKHR);
		LOAD(GetDeviceGroupPresentCapabilitiesKHR);
		LOAD(GetDeviceGroupSurfacePresentModesKHR);
		LOAD(GetSwapchainImagesKHR);
		LOAD(QueuePresentKHR);
	}

#undef LOAD

	GetDeviceQueue(m_device, m_queueFamilyIndices.graphicsFamily.value(), 0,
	               &m_graphicsQueue);
	GetDeviceQueue(m_device, m_queueFamilyIndices.presentFamily.value(), 0,
	               &m_presentQueue);
}
}  // namespace cdm
