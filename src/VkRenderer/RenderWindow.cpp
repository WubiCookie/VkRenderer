#include "RenderWindow.hpp"
#include "VulkanDevice.hpp"
#include "ImageView.hpp"
#include "Image.hpp"

#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include "cdm_vulkan.hpp"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <algorithm>
#include <iostream>
#include <limits>
#include <vector>

void VulkanDeviceObject::setCreationTime() { m_creationTime = glfwGetTime(); }

struct SwapChainSupportDetails
{
	cdm::vk::SurfaceCapabilities2KHR capabilities{};
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

void error_callback(int error, const char* description)
{
	std::cerr << "GLFW error " << error << ": " << description << std::endl;
}

struct RenderWindowPrivate
{
	static inline size_t glfwInitCounter = 0;

	VulkanDevice vulkanDevice;

	GLFWwindow* window;

	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;

	std::vector<SwapchainImage> swapchainImages;
	std::vector<std::unique_ptr<ImageView>> swapchainImageViews;
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;

	SwapChainSupportDetails swapChainSupport;

	std::vector<std::shared_ptr<VkFramebuffer>> screenSizedFramebuffers;
	std::vector<std::shared_ptr<VkPipeline>> screenSizedPipelines;
	std::vector<std::shared_ptr<VkRenderPass>> screenFormatedRenderPasses;

	std::vector<PFN_keyCallback> keyCallbacks;

	RenderWindowPrivate(int width, int height, bool layers) noexcept;
	~RenderWindowPrivate();

	void createImageViews();
	void recreateSwapchain(int width, int height);

	static void keyCallback(GLFWwindow* window, int key, int scancode,
	                        int action, int mods);

	static void framebufferSizeCallback(GLFWwindow* window, int width,
	                                    int height);
	void framebufferSizeCallback(int width, int height);

	static void iconifyCallback(GLFWwindow* window, int iconified);
	void iconifyCallback(bool iconified);

	static void maximizeCallback(GLFWwindow* window, int maximized);
	void maximizeCallback(bool maximized);
};

// static void findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface,
// uint32_t& outPresentFamily, const VulkanFunctions& vk)
//{
//	uint32_t queueFamilyCount = 0;
//	vk.GetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
// nullptr);
//
//	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
//	vk.GetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
// queueFamilies.data());
//
//	int i = 0;
//	for (const auto& queueFamily : queueFamilies)
//	{
//		VkBool32 presentSupport = false;
//		vk.GetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
//&presentSupport);
//
//		if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport)
//		{
//			outPresentFamily = i;
//			break;
//		}
//
//		i++;
//	}
//
//	std::cerr << "error: find present queue family" << std::endl;
//	exit(1);
//}

static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice,
                                            VkSurfaceKHR surface,
                                            const VulkanDevice& vk)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vk.GetPhysicalDeviceQueueFamilyProperties(physicalDevice,
	                                          &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vk.GetPhysicalDeviceQueueFamilyProperties(
	    physicalDevice, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		VkBool32 presentSupport = false;
		vk.GetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface,
		                                      &presentSupport);

		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;
		}
		if (presentSupport)
		{
			indices.presentFamily = i;
		}

		if (indices.isComplete())
		{
			break;
		}

		i++;
	}

	return indices;
}

static SwapChainSupportDetails querySwapChainSupport(
    VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
    const VulkanDevice& vk)
{
	SwapChainSupportDetails details;

	cdm::vk::PhysicalDeviceSurfaceInfo2KHR surfaceInfo;
	surfaceInfo.surface = surface;

	vk.GetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice, &surfaceInfo,
	                                            &details.capabilities);

	uint32_t formatCount;
	vk.GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface,
	                                      &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vk.GetPhysicalDeviceSurfaceFormatsKHR(
		    physicalDevice, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vk.GetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
	                                           &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vk.GetPhysicalDeviceSurfacePresentModesKHR(
		    physicalDevice, surface, &presentModeCount,
		    details.presentModes.data());
	}

	return details;
}

static VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats)
	{
		 if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
		 availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		//if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
		//    availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

static VkPresentModeKHR chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D chooseSwapExtent(
    const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width,
    uint32_t height)
{
	if (capabilities.currentExtent.width !=
	    std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		VkExtent2D actualExtent = { width, height };

		actualExtent.width = std::max(
		    capabilities.minImageExtent.width,
		    std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(
		    capabilities.minImageExtent.height,
		    std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

RenderWindowPrivate::RenderWindowPrivate(int width, int height,
                                         bool layers) noexcept
    : vulkanDevice(layers)
{
	using namespace cdm;

	auto& vk = vulkanDevice;

	if (!glfwInit())
	{
		std::cerr << "error: could not init GLFW" << std::endl;
		exit(1);
	}

	if (glfwVulkanSupported() == false)
	{
		glfwTerminate();
		std::cerr << "error: vulkan not supported" << std::endl;
		exit(1);
	}

	glfwSetErrorCallback(error_callback);

	glfwInitCounter++;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(width, height, "Window", nullptr, nullptr);

	glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	glfwSetWindowIconifyCallback(window, iconifyCallback);
	glfwSetWindowMaximizeCallback(window, maximizeCallback);

	if (window == nullptr)
	{
		std::cerr << "error: failed to create window" << std::endl;

		const char* error;
		glfwGetError(&error);
		std::cerr << error << std::endl;

		exit(1);
	}

	vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo;
	surfaceCreateInfo.hwnd = glfwGetWin32Window(window);
	surfaceCreateInfo.hinstance = GetModuleHandleW(nullptr);

	if (vk.create(surfaceCreateInfo, surface) != VK_SUCCESS)
	{
		std::cerr << "error: failed to create window surface" << std::endl;
		exit(1);
	}

	swapChainSupport = querySwapChainSupport(vk.physicalDevice(), surface, vk);

	if (swapChainSupport.formats.empty() ||
	    swapChainSupport.presentModes.empty())
	{
		std::cerr << "error: swapchain not supported" << std::endl;
		exit(1);
	}

	QueueFamilyIndices indices =
	    findQueueFamilies(vk.physicalDevice(), surface, vk);
	vk.createDevice(surface, indices);

	glfwGetFramebufferSize(window, &width, &height);

	VkSurfaceFormatKHR surfaceFormat =
	    chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode =
	    chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent =
	    chooseSwapExtent(swapChainSupport.capabilities.surfaceCapabilities,
	                     uint32_t(width), uint32_t(height));

	swapchainImageFormat = surfaceFormat.format;
	swapchainExtent = extent;

	uint32_t imageCount =
	    swapChainSupport.capabilities.surfaceCapabilities.minImageCount + 1;

	if (swapChainSupport.capabilities.surfaceCapabilities.maxImageCount > 0 &&
	    imageCount >
	        swapChainSupport.capabilities.surfaceCapabilities.maxImageCount)
	{
		imageCount =
		    swapChainSupport.capabilities.surfaceCapabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR swapchainCreateInfo;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = extent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(),
		                              indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily)
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;      // Optional
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;  // Optional
	}

	swapchainCreateInfo.preTransform =
	    swapChainSupport.capabilities.surfaceCapabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = true;
	swapchainCreateInfo.oldSwapchain = nullptr;

	if (vk.create(swapchainCreateInfo, swapchain) != VK_SUCCESS)
	{
		std::cerr << "error: failed to create swapchain" << std::endl;
		exit(1);
	}

	vk.GetSwapchainImagesKHR(vk.vkDevice(), swapchain, &imageCount, nullptr);
	//swapchainImages.resize(imageCount);
	swapchainImages.clear();
	std::vector<VkImage> vkImages(imageCount);
	vk.GetSwapchainImagesKHR(vk.vkDevice(), swapchain, &imageCount,
	                         vkImages.data());
	for (uint32_t i = 0; i < imageCount; i++)
		swapchainImages.emplace_back(SwapchainImage(vk, vkImages[i], surfaceFormat.format));

	createImageViews();
}

RenderWindowPrivate::~RenderWindowPrivate()
{
	auto& vk = vulkanDevice;
	auto instance = vk.instance();

	swapchainImageViews.clear();

	vk.destroy(swapchain);
	vk.destroy(surface);

	if (window)
		glfwDestroyWindow(window);

	--glfwInitCounter;

	if (glfwInitCounter == 0)
	{
		glfwTerminate();
	}
}

void RenderWindowPrivate::createImageViews()
{
	using namespace cdm;

	auto& vk = vulkanDevice;
	
	swapchainImageViews.resize(std::max(swapchainImages.size(), swapchainImageViews.size()));

	for (size_t i = 0; i < swapchainImages.size(); i++)
	{
		if (swapchainImageViews[i] == nullptr)
		{
			swapchainImageViews[i] = std::make_unique<ImageView>(vk, swapchainImages[i], swapchainImageFormat);
		}
		else
		{
			swapchainImageViews[i]->setImage(swapchainImages[i], swapchainImageFormat);
		}
	}
}

void RenderWindowPrivate::recreateSwapchain(int width, int height)
{
	using namespace cdm;

	auto& vk = vulkanDevice;

	vk.wait();

	//for (auto imageView : swapchainImageViews)
	//{
	//	vk.destroy(imageView);
	//}

	vk.destroy(swapchain);

	QueueFamilyIndices indices =
	    findQueueFamilies(vk.physicalDevice(), surface, vk);

	glfwGetFramebufferSize(window, &width, &height);

	VkSurfaceFormatKHR surfaceFormat =
	    chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode =
	    chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent =
	    chooseSwapExtent(swapChainSupport.capabilities.surfaceCapabilities,
	                     uint32_t(width), uint32_t(height));

	swapchainImageFormat = surfaceFormat.format;
	swapchainExtent = extent;

	uint32_t imageCount =
	    swapChainSupport.capabilities.surfaceCapabilities.minImageCount + 1;

	if (swapChainSupport.capabilities.surfaceCapabilities.maxImageCount > 0 &&
	    imageCount >
	        swapChainSupport.capabilities.surfaceCapabilities.maxImageCount)
	{
		imageCount =
		    swapChainSupport.capabilities.surfaceCapabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR swapchainCreateInfo;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = extent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(),
		                              indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily)
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;      // Optional
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;  // Optional
	}

	swapchainCreateInfo.preTransform =
	    swapChainSupport.capabilities.surfaceCapabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = true;
	swapchainCreateInfo.oldSwapchain = nullptr;

	if (vk.create(swapchainCreateInfo, swapchain) != VK_SUCCESS)
	{
		std::cerr << "error: failed to create swapchain" << std::endl;
		exit(1);
	}

	vk.GetSwapchainImagesKHR(vk.vkDevice(), swapchain, &imageCount, nullptr);
	//swapchainImages.resize(imageCount);
	swapchainImages.clear();
	std::vector<VkImage> vkImages(imageCount);
	vk.GetSwapchainImagesKHR(vk.vkDevice(), swapchain, &imageCount,
	                         vkImages.data());
	for (uint32_t i = 0; i < imageCount; i++)
		swapchainImages.emplace_back(SwapchainImage(vk, vkImages[i], surfaceFormat.format));

	createImageViews();
}

void RenderWindowPrivate::keyCallback(GLFWwindow* window, int key,
                                      int scancode, int action, int mods)
{
	RenderWindowPrivate* rwp =
	    (RenderWindowPrivate*)glfwGetWindowUserPointer(window);

	if (rwp)
		for (auto cb : rwp->keyCallbacks)
			cb(key, scancode, action, mods);
}

void RenderWindowPrivate::framebufferSizeCallback(GLFWwindow* window,
                                                  int width, int height)
{
	RenderWindowPrivate* rwp =
	    (RenderWindowPrivate*)glfwGetWindowUserPointer(window);

	if (rwp)
		rwp->framebufferSizeCallback(width, height);
}

void RenderWindowPrivate::framebufferSizeCallback(int width, int height)
{
	/// TODO rebuild swapchain
}

void RenderWindowPrivate::iconifyCallback(GLFWwindow* window, int iconified)
{
	RenderWindowPrivate* rwp =
	    (RenderWindowPrivate*)glfwGetWindowUserPointer(window);

	if (rwp)
		rwp->iconifyCallback(iconified);
}

void RenderWindowPrivate::iconifyCallback(bool iconified)
{
	/// TODO stop rendering?
}

void RenderWindowPrivate::maximizeCallback(GLFWwindow* window, int maximized)
{
	RenderWindowPrivate* rwp =
	    (RenderWindowPrivate*)glfwGetWindowUserPointer(window);

	if (rwp)
		rwp->maximizeCallback(maximized);
}

void RenderWindowPrivate::maximizeCallback(bool maximized) {}

// ========================================================================

RenderWindow::RenderWindow(int width, int height, bool layers)
    : p(std::make_unique<RenderWindowPrivate>(width, height, layers))
{
	using namespace cdm;

	auto& vk = p->vulkanDevice;

	QueueFamilyIndices queueFamilyIndices =
	    findQueueFamilies(vk.physicalDevice(), p->surface, vk);

	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolInfo.flags = 0;  // Optional

	if (vk.create(poolInfo, m_commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("error: failed to create command pool");
	}

	poolInfo.flags = VkCommandPoolCreateFlagBits::
	    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vk.create(poolInfo, m_oneTimeCommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error(
		    "error: failed to create one time command pool");
	}
}

RenderWindow::~RenderWindow()
{
	auto& vk = p->vulkanDevice;
	auto device = vk.vkDevice();

	vk.destroy(m_oneTimeCommandPool);
	vk.destroy(m_commandPool);
}

void RenderWindow::pollEvents() { glfwPollEvents(); }

void RenderWindow::show() { glfwShowWindow(p->window); }

void RenderWindow::hide() { glfwHideWindow(p->window); }

bool RenderWindow::visible()
{
	return glfwGetWindowAttrib(p->window, GLFW_VISIBLE);
}

void RenderWindow::size(int& width, int& height)
{
	glfwGetWindowSize(p->window, &width, &height);
}

void RenderWindow::pos(int& xpos, int& ypos)
{
	glfwGetWindowPos(p->window, &xpos, &ypos);
}

void RenderWindow::framebufferSize(int& width, int& height)
{
	glfwGetFramebufferSize(p->window, &width, &height);
}

void RenderWindow::setShouldClose(bool close)
{
	glfwSetWindowShouldClose(p->window, close);
}

bool RenderWindow::shouldClose() { return glfwWindowShouldClose(p->window); }

void RenderWindow::iconify() { glfwIconifyWindow(p->window); }

void RenderWindow::maximize() { glfwMaximizeWindow(p->window); }

void RenderWindow::restore() { glfwRestoreWindow(p->window); }

bool RenderWindow::iconified()
{
	return glfwGetWindowAttrib(p->window, GLFW_ICONIFIED);
}

bool RenderWindow::maximized()
{
	return glfwGetWindowAttrib(p->window, GLFW_MAXIMIZED);
}

void RenderWindow::focus() { glfwFocusWindow(p->window); }

bool RenderWindow::focused()
{
	return glfwGetWindowAttrib(p->window, GLFW_FOCUSED);
}

void RenderWindow::registerKeyCallback(PFN_keyCallback keyCallback)
{
	if (keyCallback)
		p->keyCallbacks.push_back(keyCallback);
}

void RenderWindow::unregisterKeyCallback(PFN_keyCallback keyCallback)
{
	if (keyCallback)
	{
		auto found = std::find(p->keyCallbacks.begin(), p->keyCallbacks.end(),
		                       keyCallback);
		if (found != p->keyCallbacks.end())
			p->keyCallbacks.erase(found);
	}
}

const VulkanDevice& RenderWindow::device() const { return p->vulkanDevice; }

VkSwapchainKHR RenderWindow::swapchain() const { return p->swapchain; }

VkExtent2D RenderWindow::swapchainExtent() { return p->swapchainExtent; }

VkFormat RenderWindow::swapchainImageFormat()
{
	return p->swapchainImageFormat;
}

std::vector<std::reference_wrapper<ImageView>>
RenderWindow::swapchainImageViews() const
{
	std::vector<std::reference_wrapper<ImageView>> res;
	
	//for (auto& iv : p->swapchainImageViews)
	for (size_t i = 0; i < p->swapchainImages.size(); i++)
	{
		auto& iv = p->swapchainImageViews[i];
		res.emplace_back(*iv.get());
	}

	return res;
}

VkCommandPool RenderWindow::commandPool() const { return m_commandPool; }

// VkCommandPool RenderWindow::oneTimeCommandPool() const
//{
//	return m_oneTimeCommandPool;
//}
