#define VMA_IMPLEMENTATION
#include "VulkanDevice.hpp"

//#define VK_NO_PROTOTYPES
//#define VK_USE_PLATFORM_WIN32_KHR
//#include "cdm_vulkan.hpp"

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
	// if (LogActive)
	std::cerr << "report: " << pLayerPrefix << ": " << pMessage << std::endl;

	return false;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDeviceBase::DebugUtilsMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	// if (LogActive)
	{
		if (pCallbackData->pMessageIdName && pCallbackData->pMessage)
			std::cerr << "messenger: " << pCallbackData->pMessageIdName
			          << ":\n"
			          << pCallbackData->pMessage << "\n"
			          << std::endl;
		else if (pCallbackData->pMessage)
			std::cerr << "messenger: " << pCallbackData->pMessage << "\n"
			          << std::endl;
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
		validationLayers.push_back("VK_LAYER_RENDERDOC_Capture");
		// validationLayers.push_back("VK_LAYER_LUNARG_standard_validation");
		// validationLayers.push_back("VK_LAYER_LUNARG_api_dump");

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

void VulkanDeviceBase::destroySurface(VkSurfaceKHR surface) const
{
	DestroySurfaceKHR(instance(), surface, nullptr);
}

void VulkanDeviceBase::destroy(VkSurfaceKHR surface) const
{
	destroySurface(surface);
}

VkResult VulkanDeviceBase::createSurface(
    const cdm::vk::Win32SurfaceCreateInfoKHR& createInfo,
    VkSurfaceKHR& outSurface) const
{
	return CreateWin32SurfaceKHR(instance(), &createInfo, nullptr,
	                             &outSurface);
}

VkResult VulkanDeviceBase::create(
    const cdm::vk::Win32SurfaceCreateInfoKHR& createInfo,
    VkSurfaceKHR& outSurface) const
{
	return createSurface(createInfo, outSurface);
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
	vmaDestroyAllocator(m_allocator.get());

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

	std::vector<const char*> requiredDeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
		VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
		VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
		VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
	};

	std::vector<const char*> optionalDeviceExtensions = {
		VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
	};

	uint32_t extensionCount;
	EnumerateDeviceExtensionProperties(physicalDevice(), nullptr,
	                                   &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	EnumerateDeviceExtensionProperties(physicalDevice(), nullptr,
	                                   &extensionCount,
	                                   availableExtensions.data());

	std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(),
	                                         requiredDeviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	if (!requiredExtensions.empty())
	{
		std::set<std::string> requiredExtensionsCopy = requiredExtensions;
		for (const auto& extension : requiredExtensionsCopy)
		{
			auto found = requiredExtensions.find(extension);
			if (found != requiredExtensions.end())
			{
				requiredExtensions.erase(extension);

				auto found2 =
				    std::find(requiredDeviceExtensions.begin(),
				              requiredDeviceExtensions.end(), extension);
				if (found2 != requiredDeviceExtensions.end())
					requiredDeviceExtensions.erase(found2);
			}
		}
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
	deviceFeatures.fillModeNonSolid = true;

	vk::DeviceCreateInfo createInfo;
	createInfo.queueCreateInfoCount = uint32_t(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount =
	    uint32_t(requiredDeviceExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

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
#define LOAD_OPTIONAL(func)                                                   \
	func = (PFN_vk##func)GetDeviceProcAddr(vkDevice(), "vk" #func);           \
	if (!func)                                                                \
	{                                                                         \
		std::cerr << "Could not load vk" #func "!" << std::endl;              \
	}

	LOAD(AllocateCommandBuffers);
	LOAD(AllocateDescriptorSets);
	LOAD(AllocateMemory);
	LOAD(BeginCommandBuffer);
	LOAD(BindBufferMemory);
	LOAD(BindBufferMemory2);
	LOAD(BindBufferMemory2KHR);
	LOAD(BindImageMemory);
	LOAD(BindImageMemory2);
	LOAD(BindImageMemory2KHR);

	LOAD(CmdBeginQuery);
	LOAD(CmdBeginRenderPass);
	// LOAD(CmdBeginRenderPass2);
	LOAD(CmdBeginRenderPass2KHR);
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
	LOAD(CmdEndRenderPass2KHR);
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
	LOAD(CreateRenderPass2KHR);
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
	LOAD(GetBufferMemoryRequirements2);
	LOAD(GetBufferMemoryRequirements2KHR);
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
	LOAD(GetImageMemoryRequirements2);
	LOAD(GetImageMemoryRequirements2KHR);
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

	LOAD_OPTIONAL(CmdDebugMarkerBeginEXT);
	LOAD_OPTIONAL(CmdDebugMarkerEndEXT);
	LOAD_OPTIONAL(CmdDebugMarkerInsertEXT);
	LOAD_OPTIONAL(DebugMarkerSetObjectNameEXT);
	LOAD_OPTIONAL(DebugMarkerSetObjectTagEXT);

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
#undef LOAD_OPTIONAL

	GetDeviceQueue(m_device, m_queueFamilyIndices.graphicsFamily.value(), 0,
	               &m_graphicsQueue);
	GetDeviceQueue(m_device, m_queueFamilyIndices.presentFamily.value(), 0,
	               &m_presentQueue);

#pragma region allocator
	VmaVulkanFunctions vulkanFunction = {};
	vulkanFunction.vkGetPhysicalDeviceProperties = GetPhysicalDeviceProperties;
	vulkanFunction.vkGetPhysicalDeviceMemoryProperties =
	    GetPhysicalDeviceMemoryProperties;
	vulkanFunction.vkAllocateMemory = AllocateMemory;
	vulkanFunction.vkFreeMemory = FreeMemory;
	vulkanFunction.vkMapMemory = MapMemory;
	vulkanFunction.vkUnmapMemory = UnmapMemory;
	vulkanFunction.vkFlushMappedMemoryRanges = FlushMappedMemoryRanges;
	vulkanFunction.vkInvalidateMappedMemoryRanges =
	    InvalidateMappedMemoryRanges;
	vulkanFunction.vkBindBufferMemory = BindBufferMemory;
	vulkanFunction.vkBindImageMemory = BindImageMemory;
	vulkanFunction.vkGetBufferMemoryRequirements = GetBufferMemoryRequirements;
	vulkanFunction.vkGetImageMemoryRequirements = GetImageMemoryRequirements;
	vulkanFunction.vkCreateBuffer = CreateBuffer;
	vulkanFunction.vkDestroyBuffer = DestroyBuffer;
	vulkanFunction.vkCreateImage = CreateImage;
	vulkanFunction.vkDestroyImage = DestroyImage;
	vulkanFunction.vkCmdCopyBuffer = CmdCopyBuffer;
	vulkanFunction.vkGetBufferMemoryRequirements2KHR =
	    GetBufferMemoryRequirements2KHR;
	vulkanFunction.vkGetImageMemoryRequirements2KHR =
	    GetImageMemoryRequirements2KHR;
	vulkanFunction.vkBindBufferMemory2KHR = BindBufferMemory2KHR;
	vulkanFunction.vkBindImageMemory2KHR = BindImageMemory2KHR;
	vulkanFunction.vkGetPhysicalDeviceMemoryProperties2KHR =
	    GetPhysicalDeviceMemoryProperties2KHR;

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
	allocatorInfo.instance = instance();
	allocatorInfo.physicalDevice = physicalDevice();
	allocatorInfo.device = vkDevice();
	allocatorInfo.pVulkanFunctions = &vulkanFunction;

	vmaCreateAllocator(&allocatorInfo, &m_allocator.get());
#pragma endregion allocator
}

VkFormatProperties VulkanDevice::getPhysicalDeviceFormatProperties(
    VkFormat format) const
{
	VkFormatProperties res{};
	GetPhysicalDeviceFormatProperties(physicalDevice(), format, &res);
	return res;
}

VkResult VulkanDevice::allocateCommandBuffers(
    const cdm::vk::CommandBufferAllocateInfo& allocateInfo,
    VkCommandBuffer* pCommandBuffers) const
{
	return AllocateCommandBuffers(vkDevice(), &allocateInfo, pCommandBuffers);
}

VkResult VulkanDevice::allocate(
    const cdm::vk::CommandBufferAllocateInfo& allocateInfo,
    VkCommandBuffer* pCommandBuffers) const
{
	return allocateCommandBuffers(allocateInfo, pCommandBuffers);
}

VkResult VulkanDevice::allocateDescriptorSets(
    const cdm::vk::DescriptorSetAllocateInfo& allocateInfo,
    VkDescriptorSet* pDescriptorSets) const
{
	return AllocateDescriptorSets(vkDevice(), &allocateInfo, pDescriptorSets);
}

VkResult VulkanDevice::allocate(
    const cdm::vk::DescriptorSetAllocateInfo& allocateInfo,
    VkDescriptorSet* pDescriptorSets) const
{
	return allocateDescriptorSets(allocateInfo, pDescriptorSets);
}

VkDescriptorSet VulkanDevice::allocate(VkDescriptorPool pool,
                                       VkDescriptorSetLayout layout) const
{
	cdm::vk::DescriptorSetAllocateInfo allocateInfo;
	allocateInfo.descriptorPool = pool;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &layout;

	VkDescriptorSet res = nullptr;
	allocate(allocateInfo, &res);
	return res;
}

VkResult VulkanDevice::allocateMemory(
    const cdm::vk::MemoryAllocateInfo& allocateInfo,
    VkDeviceMemory& outDeviceMemory) const
{
	return AllocateMemory(vkDevice(), &allocateInfo, nullptr,
	                      &outDeviceMemory);
}

VkResult VulkanDevice::allocate(
    const cdm::vk::MemoryAllocateInfo& allocateInfo,
    VkDeviceMemory& outDeviceMemory) const
{
	return allocateMemory(allocateInfo, outDeviceMemory);
}

VkResult VulkanDevice::createComputePipelines(
    uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos,
    VkPipeline* pPipelines, VkPipelineCache pipelineCache) const
{
	return CreateComputePipelines(vkDevice(), pipelineCache, createInfoCount,
	                              pCreateInfos, nullptr, pPipelines);
}

VkResult VulkanDevice::create(uint32_t createInfoCount,
                              const VkComputePipelineCreateInfo* pCreateInfos,
                              VkPipeline* pPipelines,
                              VkPipelineCache pipelineCache) const
{
	return createComputePipelines(createInfoCount, pCreateInfos, pPipelines,
	                              pipelineCache);
}

VkResult VulkanDevice::createComputePipelines(
    uint32_t createInfoCount,
    const cdm::vk::ComputePipelineCreateInfo* pCreateInfos,
    VkPipeline* pPipelines, VkPipelineCache pipelineCache) const
{
	return CreateComputePipelines(vkDevice(), pipelineCache, createInfoCount,
	                              pCreateInfos, nullptr, pPipelines);
}

VkResult VulkanDevice::create(
    uint32_t createInfoCount,
    const cdm::vk::ComputePipelineCreateInfo* pCreateInfos,
    VkPipeline* pPipelines, VkPipelineCache pipelineCache) const
{
	return createComputePipelines(createInfoCount, pCreateInfos, pPipelines,
	                              pipelineCache);
}

VkResult VulkanDevice::createComputePipeline(
    const cdm::vk::ComputePipelineCreateInfo& createInfo,
    VkPipeline& outPipeline, VkPipelineCache pipelineCache) const
{
	return CreateComputePipelines(vkDevice(), pipelineCache, 1, &createInfo,
	                              nullptr, &outPipeline);
}

VkResult VulkanDevice::create(
    const cdm::vk::ComputePipelineCreateInfo& createInfo,
    VkPipeline& outPipeline, VkPipelineCache pipelineCache) const
{
	return createComputePipeline(createInfo, outPipeline, pipelineCache);
}

UniquePipeline VulkanDevice::createComputePipeline(
    const cdm::vk::ComputePipelineCreateInfo& createInfo,
    VkPipelineCache pipelineCache) const
{
	VkPipeline res = nullptr;
	createComputePipeline(createInfo, res, pipelineCache);
	return UniquePipeline(res, *this);
}

VkResult VulkanDevice::createGraphicsPipelines(
    uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos,
    VkPipeline* pPipelines, VkPipelineCache pipelineCache) const
{
	return CreateGraphicsPipelines(vkDevice(), pipelineCache, createInfoCount,
	                               pCreateInfos, nullptr, pPipelines);
}

VkResult VulkanDevice::create(uint32_t createInfoCount,
                              const VkGraphicsPipelineCreateInfo* pCreateInfos,
                              VkPipeline* pPipelines,
                              VkPipelineCache pipelineCache) const
{
	return createGraphicsPipelines(createInfoCount, pCreateInfos, pPipelines,
	                               pipelineCache);
}

VkResult VulkanDevice::createGraphicsPipelines(
    uint32_t createInfoCount,
    const cdm::vk::GraphicsPipelineCreateInfo* pCreateInfos,
    VkPipeline* pPipelines, VkPipelineCache pipelineCache) const
{
	return CreateGraphicsPipelines(vkDevice(), pipelineCache, createInfoCount,
	                               pCreateInfos, nullptr, pPipelines);
}

VkResult VulkanDevice::create(
    uint32_t createInfoCount,
    const cdm::vk::GraphicsPipelineCreateInfo* pCreateInfos,
    VkPipeline* pPipelines, VkPipelineCache pipelineCache) const
{
	return createGraphicsPipelines(createInfoCount, pCreateInfos, pPipelines,
	                               pipelineCache);
}

VkResult VulkanDevice::createGraphicsPipeline(
    const cdm::vk::GraphicsPipelineCreateInfo& createInfo,
    VkPipeline& outPipeline, VkPipelineCache pipelineCache) const
{
	return CreateGraphicsPipelines(vkDevice(), pipelineCache, 1, &createInfo,
	                               nullptr, &outPipeline);
}

VkResult VulkanDevice::create(
    const cdm::vk::GraphicsPipelineCreateInfo& createInfo,
    VkPipeline& outPipeline, VkPipelineCache pipelineCache) const
{
	return createGraphicsPipeline(createInfo, outPipeline, pipelineCache);
}

UniquePipeline VulkanDevice::createGraphicsPipeline(
    const cdm::vk::GraphicsPipelineCreateInfo& createInfo,
    VkPipelineCache pipelineCache) const
{
	VkPipeline res = nullptr;
	createGraphicsPipeline(createInfo, res, pipelineCache);
	return UniquePipeline(res, *this);
}

VkResult VulkanDevice::createRenderPass2(
    const cdm::vk::RenderPassCreateInfo2& createInfo,
    VkRenderPass& outRenderPass) const
{
	return CreateRenderPass2KHR(vkDevice(), &createInfo, nullptr,
	                            &outRenderPass);
}

VkResult VulkanDevice::create(const cdm::vk::RenderPassCreateInfo2& createInfo,
                              VkRenderPass& outRenderPass) const
{
	return createRenderPass2(createInfo, outRenderPass);
}

UniqueRenderPass VulkanDevice::createRenderPass2(
    const cdm::vk::RenderPassCreateInfo2& createInfo) const
{
	VkRenderPass res = nullptr;
	createRenderPass2(createInfo, res);
	return UniqueRenderPass(res, *this);
}

UniqueBuffer VulkanDevice::createBuffer(
    const cdm::vk::BufferCreateInfo& createInfo) const
{
	VkBuffer res = nullptr;
	createBuffer(createInfo, res);
	return UniqueBuffer(res, *this);
}

UniqueBufferView VulkanDevice::createBufferView(
    const cdm::vk::BufferViewCreateInfo& createInfo) const
{
	VkBufferView res = nullptr;
	createBufferView(createInfo, res);
	return UniqueBufferView(res, *this);
}

UniqueCommandPool VulkanDevice::createCommandPool(
    const cdm::vk::CommandPoolCreateInfo& createInfo) const
{
	VkCommandPool res = nullptr;
	createCommandPool(createInfo, res);
	return UniqueCommandPool(res, *this);
}

UniqueDescriptorPool VulkanDevice::createDescriptorPool(
    const cdm::vk::DescriptorPoolCreateInfo& createInfo) const
{
	VkDescriptorPool res = nullptr;
	createDescriptorPool(createInfo, res);
	return UniqueDescriptorPool(res, *this);
}

UniqueDescriptorSetLayout VulkanDevice::createDescriptorSetLayout(
    const cdm::vk::DescriptorSetLayoutCreateInfo& createInfo) const
{
	VkDescriptorSetLayout res = nullptr;
	createDescriptorSetLayout(createInfo, res);
	return UniqueDescriptorSetLayout(res, *this);
}

UniqueDescriptorUpdateTemplate VulkanDevice::createDescriptorUpdateTemplate(
    const cdm::vk::DescriptorUpdateTemplateCreateInfo& createInfo) const
{
	VkDescriptorUpdateTemplate res = nullptr;
	createDescriptorUpdateTemplate(createInfo, res);
	return UniqueDescriptorUpdateTemplate(res, *this);
}

UniqueEvent VulkanDevice::createEvent(
    const cdm::vk::EventCreateInfo& createInfo) const
{
	VkEvent res = nullptr;
	createEvent(createInfo, res);
	return UniqueEvent(res, *this);
}

UniqueFence VulkanDevice::createFence(
    const cdm::vk::FenceCreateInfo& createInfo) const
{
	VkFence res = nullptr;
	createFence(createInfo, res);
	return UniqueFence(res, *this);
}

UniqueFence VulkanDevice::createFence(VkFenceCreateFlags flags) const
{
	cdm::vk::FenceCreateInfo createInfo;
	createInfo.flags = flags;
	return createFence(createInfo);
}

UniqueFramebuffer VulkanDevice::createFramebuffer(
    const cdm::vk::FramebufferCreateInfo& createInfo) const
{
	VkFramebuffer res = nullptr;
	createFramebuffer(createInfo, res);
	return UniqueFramebuffer(res, *this);
}

UniqueImage VulkanDevice::createImage(
    const cdm::vk::ImageCreateInfo& createInfo) const
{
	VkImage res = nullptr;
	createImage(createInfo, res);
	return UniqueImage(res, *this);
}

UniqueImageView VulkanDevice::createImageView(
    const cdm::vk::ImageViewCreateInfo& createInfo) const
{
	VkImageView res = nullptr;
	createImageView(createInfo, res);
	return UniqueImageView(res, *this);
}

UniquePipelineCache VulkanDevice::createPipelineCache(
    const cdm::vk::PipelineCacheCreateInfo& createInfo) const
{
	VkPipelineCache res = nullptr;
	createPipelineCache(createInfo, res);
	return UniquePipelineCache(res, *this);
}

UniquePipelineLayout VulkanDevice::createPipelineLayout(
    const cdm::vk::PipelineLayoutCreateInfo& createInfo) const
{
	VkPipelineLayout res = nullptr;
	createPipelineLayout(createInfo, res);
	return UniquePipelineLayout(res, *this);
}

UniqueQueryPool VulkanDevice::createQueryPool(
    const cdm::vk::QueryPoolCreateInfo& createInfo) const
{
	VkQueryPool res = nullptr;
	createQueryPool(createInfo, res);
	return UniqueQueryPool(res, *this);
}

UniqueRenderPass VulkanDevice::createRenderPass(
    const cdm::vk::RenderPassCreateInfo& createInfo) const
{
	VkRenderPass res = nullptr;
	createRenderPass(createInfo, res);
	return UniqueRenderPass(res, *this);
}

UniqueSampler VulkanDevice::createSampler(
    const cdm::vk::SamplerCreateInfo& createInfo) const
{
	VkSampler res = nullptr;
	createSampler(createInfo, res);
	return UniqueSampler(res, *this);
}

UniqueSamplerYcbcrConversion VulkanDevice::createSamplerYcbcrConversion(
    const cdm::vk::SamplerYcbcrConversionCreateInfo& createInfo) const
{
	VkSamplerYcbcrConversion res = nullptr;
	createSamplerYcbcrConversion(createInfo, res);
	return UniqueSamplerYcbcrConversion(res, *this);
}

UniqueSemaphore VulkanDevice::createSemaphore(
    const cdm::vk::SemaphoreCreateInfo& createInfo) const
{
	VkSemaphore res = nullptr;
	createSemaphore(createInfo, res);
	return UniqueSemaphore(res, *this);
}

UniqueSemaphore VulkanDevice::createSemaphore(bool signaled) const
{
	cdm::vk::SemaphoreCreateInfo createInfo;
	createInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

	return createSemaphore(createInfo);
}

UniqueShaderModule VulkanDevice::createShaderModule(
    const cdm::vk::ShaderModuleCreateInfo& createInfo) const
{
	VkShaderModule res = nullptr;
	createShaderModule(createInfo, res);
	return UniqueShaderModule(res, *this);
}

void VulkanDevice::destroyDevice() const
{
	DestroyDevice(vkDevice(), nullptr);
}

void VulkanDevice::destroy() const { destroyDevice(); }

VkResult VulkanDevice::waitIdle() const { return DeviceWaitIdle(vkDevice()); }
VkResult VulkanDevice::wait() const { return waitIdle(); }

void VulkanDevice::freeCommandBuffers(
    VkCommandPool commandPool, uint32_t commandBufferCount,
    const VkCommandBuffer* pCommandBuffers) const
{
	FreeCommandBuffers(vkDevice(), commandPool, commandBufferCount,
	                   pCommandBuffers);
}

void VulkanDevice::free(VkCommandPool commandPool, uint32_t commandBufferCount,
                        const VkCommandBuffer* pCommandBuffers) const
{
	freeCommandBuffers(commandPool, commandBufferCount, pCommandBuffers);
}

void VulkanDevice::freeCommandBuffer(VkCommandPool commandPool,
                                     VkCommandBuffer CommandBuffer) const
{
	FreeCommandBuffers(vkDevice(), commandPool, 1, &CommandBuffer);
}

void VulkanDevice::free(VkCommandPool commandPool,
                        VkCommandBuffer CommandBuffer) const
{
	freeCommandBuffers(commandPool, 1, &CommandBuffer);
}

VkResult VulkanDevice::freeDescriptorSets(
    VkDescriptorPool descriptorPool, uint32_t descriptorSetCount,
    const VkDescriptorSet* pDescriptorSets) const
{
	return FreeDescriptorSets(vkDevice(), descriptorPool, descriptorSetCount,
	                          pDescriptorSets);
}

VkResult VulkanDevice::free(VkDescriptorPool descriptorPool,
                            uint32_t descriptorSetCount,
                            const VkDescriptorSet* pDescriptorSets) const
{
	return freeDescriptorSets(descriptorPool, descriptorSetCount,
	                          pDescriptorSets);
}
VkResult VulkanDevice::freeDescriptorSets(VkDescriptorPool descriptorPool,
                                          VkDescriptorSet DescriptorSet) const
{
	return FreeDescriptorSets(vkDevice(), descriptorPool, 1, &DescriptorSet);
}

VkResult VulkanDevice::free(VkDescriptorPool descriptorPool,
                            VkDescriptorSet DescriptorSet) const
{
	return freeDescriptorSets(descriptorPool, 1, &DescriptorSet);
}

void VulkanDevice::freeMemory(VkDeviceMemory aDeviceMemory) const
{
	FreeMemory(vkDevice(), aDeviceMemory, nullptr);
}

void VulkanDevice::free(VkDeviceMemory aDeviceMemory) const
{
	freeMemory(aDeviceMemory);
}

VkResult VulkanDevice::getFenceStatus(VkFence fence) const
{
	return GetFenceStatus(vkDevice(), fence);
}

VkResult VulkanDevice::getStatus(VkFence fence) const
{
	return getFenceStatus(fence);
}

VkResult VulkanDevice::queueSubmit(VkQueue queue, uint32_t submitCount,
                                   const VkSubmitInfo* submits,
                                   VkFence fence) const
{
	return QueueSubmit(queue, submitCount, submits, fence);
}

VkResult VulkanDevice::queueSubmit(VkQueue queue, const VkSubmitInfo& submit,
                                   VkFence fence) const
{
	return queueSubmit(queue, 1, &submit, fence);
}

VkResult VulkanDevice::queueSubmit(VkQueue queue,
                                   VkCommandBuffer commandBuffer,
                                   VkFence fence) const
{
	vk::SubmitInfo submit;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &commandBuffer;

	return queueSubmit(queue, 1, &submit, fence);
}

VkResult VulkanDevice::queueSubmit(VkQueue queue,
                                   VkCommandBuffer commandBuffer,
                                   VkSemaphore waitSemaphore,
                                   VkPipelineStageFlags waitDstStageMask,
                                   VkFence fence) const
{
	vk::SubmitInfo submit;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &commandBuffer;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &waitSemaphore;
	submit.pWaitDstStageMask = &waitDstStageMask;

	return queueSubmit(queue, 1, &submit, fence);
}

VkResult VulkanDevice::queueSubmit(VkQueue queue,
                                   VkCommandBuffer commandBuffer,
                                   VkSemaphore signalSemaphore,
                                   VkFence fence) const
{
	vk::SubmitInfo submit;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &commandBuffer;
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &signalSemaphore;

	return queueSubmit(queue, 1, &submit, fence);
}

VkResult VulkanDevice::queueSubmit(VkQueue queue,
                                   VkCommandBuffer commandBuffer,
                                   VkSemaphore waitSemaphore,
                                   VkPipelineStageFlags waitDstStageMask,
                                   VkSemaphore signalSemaphore,
                                   VkFence fence) const
{
	vk::SubmitInfo submit;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &commandBuffer;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &waitSemaphore;
	submit.pWaitDstStageMask = &waitDstStageMask;
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &signalSemaphore;

	return queueSubmit(queue, 1, &submit, fence);
}

VkResult VulkanDevice::queueWaitIdle(VkQueue queue) const
{
	return QueueWaitIdle(queue);
}

VkResult VulkanDevice::waitIdle(VkQueue queue) const
{
	return QueueWaitIdle(queue);
}

VkResult VulkanDevice::wait(VkQueue queue) const
{
	return QueueWaitIdle(queue);
}

VkResult VulkanDevice::waitForFences(uint32_t fenceCount,
                                     const VkFence* pFences, bool waitAll,
                                     uint64_t timeout) const
{
	return WaitForFences(vkDevice(), fenceCount, pFences, waitAll, timeout);
}

VkResult VulkanDevice::resetFences(uint32_t fenceCount,
                                   const VkFence* fences) const
{
	return ResetFences(vkDevice(), fenceCount, fences);
}

VkResult VulkanDevice::resetFences(std::initializer_list<VkFence> fences) const
{
	return resetFences(uint32_t(fences.size()), fences.begin());
}

VkResult VulkanDevice::resetFence(const VkFence& fence) const
{
	return resetFences(1, &fence);
}

void VulkanDevice::updateDescriptorSets(
    uint32_t descriptorWriteCount,
    const vk::WriteDescriptorSet* descriptorWrites,
    uint32_t descriptorCopyCount,
    const vk::CopyDescriptorSet* descriptorCopies) const
{
	UpdateDescriptorSets(vkDevice(), descriptorWriteCount, descriptorWrites,
	                     descriptorCopyCount, descriptorCopies);
}

void VulkanDevice::updateDescriptorSets(
    uint32_t descriptorWriteCount,
    const vk::WriteDescriptorSet* descriptorWrites) const
{
	updateDescriptorSets(descriptorWriteCount, descriptorWrites, 0, nullptr);
}

void VulkanDevice::updateDescriptorSets(
    uint32_t descriptorCopyCount,
    const vk::CopyDescriptorSet* descriptorCopies) const
{
	updateDescriptorSets(0, nullptr, descriptorCopyCount, descriptorCopies);
}

void VulkanDevice::updateDescriptorSets(
    const vk::WriteDescriptorSet& descriptorWrite) const
{
	updateDescriptorSets(1, &descriptorWrite);
}

void VulkanDevice::updateDescriptorSets(
    std::initializer_list<vk::WriteDescriptorSet> descriptorWrites) const
{
	updateDescriptorSets(uint32_t(descriptorWrites.size()),
	                     descriptorWrites.begin());
}

void VulkanDevice::updateDescriptorSets(
    const vk::CopyDescriptorSet& descriptorCopy) const
{
	updateDescriptorSets(1, &descriptorCopy);
}

void VulkanDevice::updateDescriptorSets(
    std::initializer_list<vk::CopyDescriptorSet> descriptorCopies) const
{
	updateDescriptorSets(uint32_t(descriptorCopies.size()),
	                     descriptorCopies.begin());
}

VkResult VulkanDevice::wait(uint32_t fenceCount, const VkFence* pFences,
                            bool waitAll, uint64_t timeout) const
{
	return waitForFences(fenceCount, pFences, waitAll, timeout);
}

VkResult VulkanDevice::waitForFence(VkFence fence, uint64_t timeout) const
{
	return WaitForFences(vkDevice(), 1, &fence, true, timeout);
}

VkResult VulkanDevice::wait(VkFence fence, uint64_t timeout) const
{
	return waitForFence(fence, timeout);
}

VkResult VulkanDevice::waitSemaphores(cdm::vk::SemaphoreWaitInfo& waitInfo,
                                      uint64_t timeout) const
{
	return WaitSemaphores(vkDevice(), &waitInfo, timeout);
}

VkResult VulkanDevice::wait(cdm::vk::SemaphoreWaitInfo& waitInfo,
                            uint64_t timeout) const
{
	return waitSemaphores(waitInfo, timeout);
}

VkResult VulkanDevice::createSwapchain(
    const cdm::vk::SwapchainCreateInfoKHR& createInfo,
    VkSwapchainKHR& outSwapchain) const
{
	return CreateSwapchainKHR(vkDevice(), &createInfo, nullptr, &outSwapchain);
}

VkResult VulkanDevice::create(
    const cdm::vk::SwapchainCreateInfoKHR& createInfo,
    VkSwapchainKHR& outSwapchain) const
{
	return createSwapchain(createInfo, outSwapchain);
}

void VulkanDevice::destroySwapchain(VkSwapchainKHR swapchain) const
{
	DestroySwapchainKHR(vkDevice(), swapchain, nullptr);
}

void VulkanDevice::destroy(VkSwapchainKHR swapchain) const
{
	destroySwapchain(swapchain);
}

VkResult VulkanDevice::queuePresent(
    VkQueue queue, const cdm::vk::PresentInfoKHR& present) const
{
	return QueuePresentKHR(queue, &present);
}

VkResult VulkanDevice::queuePresent(VkQueue queue, VkSwapchainKHR swapchain,
                                    uint32_t index) const
{
	cdm::vk::PresentInfoKHR present;
	present.swapchainCount = 1;
	present.pSwapchains = &swapchain;
	present.pImageIndices = &index;

	return queuePresent(queue, present);
}

VkResult VulkanDevice::queuePresent(VkQueue queue, VkSwapchainKHR swapchain,
                                    uint32_t index,
                                    VkSemaphore waitSemaphore) const
{
	cdm::vk::PresentInfoKHR present;
	present.swapchainCount = 1;
	present.pSwapchains = &swapchain;
	present.pImageIndices = &index;
	present.waitSemaphoreCount = 1;
	present.pWaitSemaphores = &waitSemaphore;

	return queuePresent(queue, present);
}

VkResult VulkanDevice::debugMarkerSetObjectName(
    const cdm::vk::DebugMarkerObjectNameInfoEXT& nameInfo) const
{
	if (DebugMarkerSetObjectNameEXT)
		return DebugMarkerSetObjectNameEXT(vkDevice(), &nameInfo);
	else
		return VkResult::VK_ERROR_EXTENSION_NOT_PRESENT;
}

VkResult VulkanDevice::debugMarkerSetObjectTag(
    const cdm::vk::DebugMarkerObjectTagInfoEXT& tagInfo) const
{
	if (DebugMarkerSetObjectNameEXT)
		return DebugMarkerSetObjectTagEXT(vkDevice(), &tagInfo);
	else
		return VkResult::VK_ERROR_EXTENSION_NOT_PRESENT;
}

}  // namespace cdm
