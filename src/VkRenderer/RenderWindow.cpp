#include "RenderWindow.hpp"
#include "CommandBuffer.hpp"
#include "CommandBufferPool.hpp"
#include "Image.hpp"
#include "ImageView.hpp"
#include "Texture2D.hpp"
#include "VulkanDevice.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "my_imgui_impl_vulkan.h"

#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include "cdm_stack_vector.hpp"
#include "cdm_vulkan.hpp"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <algorithm>
#include <forward_list>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace cdm
{
void VulkanDeviceObject::setCreationTime() { m_creationTime = glfwGetTime(); }
// void VulkanDeviceObject::setCreationTime() { m_creationTime++; }

struct SwapChainSupportDetails
{
	vk::SurfaceCapabilities2KHR capabilities{};
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

	GLFWwindow* window = nullptr;

	VkSurfaceKHR surface = nullptr;
	VkSwapchainKHR swapchain = nullptr;

	VkCommandPool commandPool = nullptr;
	VkCommandPool oneTimeCommandPool = nullptr;

	std::forward_list<FrameCommandBuffer> frameCommandBuffers;

	UniqueDescriptorPool imguiDescriptorPool;
	UniqueRenderPass imguiRenderPass;

	std::vector<SwapchainImage> swapchainImages;
	std::vector<std::unique_ptr<ImageView>> swapchainImageViews;
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;

	UniqueSemaphore acquireToCopySemaphore;
	UniqueSemaphore copyToPresentSemaphore;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	// std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;

	std::vector<UniqueFence> imageAcquisitionFences;
	std::vector<UniqueSemaphore> imageAcquisitionSemaphores;

	std::vector<VkSemaphore> presentWaitSemaphores;

	SwapChainSupportDetails swapChainSupport;

	std::vector<std::shared_ptr<VkFramebuffer>> screenSizedFramebuffers;
	std::vector<std::shared_ptr<VkPipeline>> screenSizedPipelines;
	std::vector<std::shared_ptr<VkRenderPass>> screenFormatedRenderPasses;

	std::vector<PFN_keyCallback> keyCallbacks;
	std::vector<PFN_mouseButtonCallback> mouseButtonCallbacks;
	std::vector<PFN_mousePosCallback> mousePosCallbacks;

	std::array<ButtonState, int(MouseButton::COUNT)> mouseButtonsStates;

	double swapchainCreationTime = 0.0;

	RenderWindowPrivate(int width, int height, bool layers);
	~RenderWindowPrivate();

	void createImageViews();
	void recreateSwapchain(int width, int height);

	static void keyCallback(GLFWwindow* window, int key, int scancode,
	                        int action, int mods);

	static void mouseButtonCallback(GLFWwindow* window, int button, int action,
	                                int mods);

	static void mousePosCallback(GLFWwindow* window, double x, double y);

	static void framebufferSizeCallback(GLFWwindow* window, int width,
	                                    int height);
	void framebufferSizeCallback(int width, int height);

	static void iconifyCallback(GLFWwindow* window, int iconified);
	void iconifyCallback(bool iconified);

	static void maximizeCallback(GLFWwindow* window, int maximized);
	void maximizeCallback(bool maximized);
};

#pragma region helpers
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

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice,
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

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice,
                                              VkSurfaceKHR surface,
                                              const VulkanDevice& vk)
{
	SwapChainSupportDetails details;

	vk::PhysicalDeviceSurfaceInfo2KHR surfaceInfo;
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

VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
		    availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		// if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
		//    availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_IMMEDIATE_KHR;
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities,
                            uint32_t width, uint32_t height)
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

void check_vk_result(VkResult err)
{
	if (err == 0)
		return;
	printf("VkResult %d\n", err);
	if (err < 0)
		abort();
}

VkFormat findSupportedFormat(const VulkanDevice& vk,
                             const std::vector<VkFormat>& candidates,
                             VkImageTiling tiling,
                             VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props =
		    vk.getPhysicalDeviceFormatProperties(format);

		if (tiling == VK_IMAGE_TILING_LINEAR &&
		    (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
		         (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format");
}

VkFormat findDepthFormat(const VulkanDevice& vk)
{
	return findSupportedFormat(
	    vk,
	    { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
	      VK_FORMAT_D24_UNORM_S8_UINT },
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
	       format == VK_FORMAT_D24_UNORM_S8_UINT;
}
#pragma endregion

RenderWindowPrivate::RenderWindowPrivate(int width, int height, bool layers)
    : vulkanDevice(layers)
{
	auto& vk = vulkanDevice;

#pragma region window
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
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetCursorPosCallback(window, mousePosCallback);
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
#pragma endregion

#pragma region vulkan
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

	QueueFamilyIndices queueFamilyIndices =
	    findQueueFamilies(vk.physicalDevice(), surface, vk);

	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolInfo.flags = 0;  // Optional

	poolInfo.flags = VkCommandPoolCreateFlagBits::
	    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vk.create(poolInfo, commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("error: failed to create command pool");
	}
	vk.debugMarkerSetObjectName(commandPool, "RenderWindow::commandPool");

	if (vk.create(poolInfo, oneTimeCommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error(
		    "error: failed to create one time command pool");
	}
	vk.debugMarkerSetObjectName(commandPool,
	                            "RenderWindow::oneTimeCommandPool");

	recreateSwapchain(width, height);

	acquireToCopySemaphore = vk.createSemaphore();
	copyToPresentSemaphore = vk.createSemaphore();

	for (size_t i = 0; i < swapchainImages.size(); i++)
	{
		imageAcquisitionFences.emplace_back(
		    vk.createFence(VK_FENCE_CREATE_SIGNALED_BIT));
		imageAcquisitionSemaphores.emplace_back(vk.createSemaphore());
	}

#pragma endregion

#pragma region imguirenderPass
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = VkFormat::VK_FORMAT_B8G8R8A8_UNORM;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// colorAttachment.initialLayout =
	// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; colorAttachment.finalLayout =
	// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

	// VkAttachmentDescription colorHDRAttachment = {};
	// colorHDRAttachment.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
	// colorHDRAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// colorHDRAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	// colorHDRAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	// colorHDRAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	// colorHDRAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// colorHDRAttachment.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	// colorHDRAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

	// std::array colorAttachments{ colorAttachment, colorHDRAttachment };
	std::array colorAttachments{ colorAttachment };

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// VkAttachmentReference colorHDRAttachmentRef = {};
	// colorHDRAttachmentRef.attachment = 1;
	// colorHDRAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::array colorAttachmentRefs{

		colorAttachmentRef  //, colorHDRAttachmentRef
	};

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = uint32_t(colorAttachmentRefs.size());
	subpass.pColorAttachments = colorAttachmentRefs.data();
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.pResolveAttachments = nullptr;
	subpass.pDepthStencilAttachment = nullptr;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;

	VkSubpassDependency dependency = {};
	dependency.dependencyFlags = 0;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
	                           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.attachmentCount = uint32_t(colorAttachments.size());
	renderPassInfo.pAttachments = colorAttachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	imguiRenderPass = vk.create(renderPassInfo);
	if (!imguiRenderPass)
	{
		std::cerr << "error: failed to create imgui render pass" << std::endl;
		abort();
	}
#pragma endregion

#pragma region imgui
	VkDescriptorPoolSize pool_sizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};
	vk::DescriptorPoolCreateInfo pool_info;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
	pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;
	imguiDescriptorPool = vk.create(pool_info);

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	ImGui::StyleColorsDark();
	// ImGui::StyleColorsClassic();

	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = vk.instance();
	init_info.PhysicalDevice = vk.physicalDevice();
	init_info.Device = vk.vkDevice();
	init_info.QueueFamily = queueFamilyIndices.graphicsFamily.value();
	init_info.Queue = vk.graphicsQueue();
	init_info.PipelineCache = nullptr;
	init_info.DescriptorPool = imguiDescriptorPool.get();
	init_info.Allocator = nullptr;
	init_info.MinImageCount = 3;
	init_info.ImageCount = uint32_t(swapchainImages.size());
	init_info.CheckVkResultFn = check_vk_result;
	init_info.vk = &vulkanDevice;
	ImGui_ImplVulkan_Init(&init_info, imguiRenderPass.get());

	{
		// Use any command queue
		VkCommandPool command_pool = commandPool;
		CommandBuffer command_buffer(vk, command_pool);
		// VkCommandBuffer command_buffer =
		// wd->Frames[wd->FrameIndex].CommandBuffer;

		vk.ResetCommandPool(vk.vkDevice(), command_pool, 0);
		command_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		// check_vk_result(err);
		// vk::CommandBufferBeginInfo begin_info;
		// begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		// begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		// err = vkBeginCommandBuffer(command_buffer, &begin_info);
		// check_vk_result(err);

		ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

		command_buffer.end();

		if (vk.queueSubmit(vk.graphicsQueue(), command_buffer.get()) !=
		    VK_SUCCESS)
		{
			std::cerr << "error: failed to submit compute command buffer"
			          << std::endl;
			abort();
		}
		vk.wait();

		// VkSubmitInfo end_info = {};
		// end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		// end_info.commandBufferCount = 1;
		// end_info.pCommandBuffers = &command_buffer;
		// err = vkEndCommandBuffer(command_buffer);
		// check_vk_result(err);
		// err = vkQueueSubmit(g_Queue, 1, &end_info, VK_NULL_HANDLE);
		// check_vk_result(err);

		// err = vkDeviceWaitIdle(g_Device);
		// check_vk_result(err);

		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}
#pragma endregion
}

RenderWindowPrivate::~RenderWindowPrivate()
{
	auto& vk = vulkanDevice;

	for (auto& imageAcquisitionFence : imageAcquisitionFences)
		vk.wait(imageAcquisitionFence);
	vk.wait();

	auto instance = vk.instance();

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	for (auto imageAvailableSemaphore : imageAvailableSemaphores)
	{
		vk.destroy(imageAvailableSemaphore);
	}
	imageAvailableSemaphores.clear();

	for (auto inFlightFence : inFlightFences)
	{
		vk.destroy(inFlightFence);
	}

	swapchainImageViews.clear();

	vk.destroy(swapchain);
	vk.destroy(surface);

	frameCommandBuffers.clear();

	vk.destroy(oneTimeCommandPool);
	vk.destroy(commandPool);

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
	auto& vk = vulkanDevice;

	vk.wait();

	if (swapchainImageViews.size() != swapchainImages.size())
	{
		swapchainImageViews.resize(
		    std::max(swapchainImages.size(), swapchainImageViews.size()));

		for (size_t i = 0; i < swapchainImages.size(); i++)
		{
			if (swapchainImageViews[i] == nullptr)
			{
				swapchainImageViews[i] = std::make_unique<ImageView>(
				    vk, swapchainImages[i], swapchainImageFormat);
			}
			// else
			//{
			//	swapchainImageViews[i]->setImage(swapchainImages[i],
			//	                                 swapchainImageFormat);
			//}
		}
	}
}

void RenderWindowPrivate::recreateSwapchain(int width, int height)
{
	auto& vk = vulkanDevice;

	vk.wait();

	for (auto imageAvailableSemaphore : imageAvailableSemaphores)
	{
		vk.destroy(imageAvailableSemaphore);
	}
	imageAvailableSemaphores.clear();

	// for (auto renderFinishedSemaphore : renderFinishedSemaphores)
	//{
	//	vk.destroy(renderFinishedSemaphore);
	//}

	for (auto inFlightFence : inFlightFences)
	{
		vk.destroy(inFlightFence);
	}
	inFlightFences.clear();

	QueueFamilyIndices indices =
	    findQueueFamilies(vk.physicalDevice(), surface, vk);

	glfwGetFramebufferSize(window, &width, &height);

	vk::PhysicalDeviceSurfaceInfo2KHR surfaceInfo;
	surfaceInfo.surface = surface;

	vk.GetPhysicalDeviceSurfaceCapabilities2KHR(
	    vk.physicalDevice(), &surfaceInfo, &swapChainSupport.capabilities);

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
	swapchainCreateInfo.imageUsage =
	    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	std::array queueFamilyIndices = { indices.graphicsFamily.value(),
		                              indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily)
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount =
		    uint32_t(queueFamilyIndices.size());
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
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
	swapchainCreateInfo.oldSwapchain = swapchain;

	auto oldSwapchain = swapchain;

	if (vk.create(swapchainCreateInfo, swapchain) != VK_SUCCESS)
	{
		std::cerr << "error: failed to create swapchain" << std::endl;
		exit(1);
	}

	vk.destroy(oldSwapchain);

	vk.GetSwapchainImagesKHR(vk.vkDevice(), swapchain, &imageCount, nullptr);

	if (swapchainImages.empty())
	{
		std::vector<VkImage> vkImages(imageCount);
		vk.GetSwapchainImagesKHR(vk.vkDevice(), swapchain, &imageCount,
		                         vkImages.data());
		for (uint32_t i = 0; i < imageCount; i++)
			swapchainImages.emplace_back(
			    SwapchainImage(vk, vkImages[i], surfaceFormat.format));
	}
	else
	{
		if (swapchainImages.size() == imageCount)
		{
			std::vector<VkImage> vkImages(imageCount);
			vk.GetSwapchainImagesKHR(vk.vkDevice(), swapchain, &imageCount,
			                         vkImages.data());
			for (uint32_t i = 0; i < imageCount; i++)
			{
				swapchainImages[i].setImage(vkImages[i]);
				swapchainImages[i].setFormat(surfaceFormat.format);
			}
		}
		else
		{
			std::cerr << "error: unhandled case, imageCount has changed"
			          << std::endl;
			exit(1);
		}
	}

	CommandBuffer transitionCB(vk, commandPool);

	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	transitionCB.begin(beginInfo);
	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

	for (auto& image : swapchainImages)
	{
		barrier.image = image.image();
		transitionCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		                             VK_DEPENDENCY_DEVICE_GROUP_BIT, barrier);
	}
	if (transitionCB.end() != VK_SUCCESS)
	{
		std::cerr << "error: failed to record command buffer" << std::endl;
		abort();
	}

	vk::SubmitInfo submitInfo;

	// VkSemaphore waitSemaphores[] = {
	//	rw.get().currentImageAvailableSemaphore()
	//};
	// VkPipelineStageFlags waitStages[] = {
	//	VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	//};
	// submitInfo.waitSemaphoreCount = 1;
	// submitInfo.pWaitSemaphores = waitSemaphores;
	// submitInfo.pWaitDstStageMask = waitStages;

	// auto cb = m_commandBuffers[rw.get().imageIndex()].commandBuffer();
	auto cb = transitionCB.commandBuffer();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cb;
	// VkSemaphore signalSemaphores[] = {
	//	m_renderFinishedSemaphores[rw.get().currentFrame()]
	//};
	// submitInfo.signalSemaphoreCount = 1;
	// submitInfo.pSignalSemaphores = signalSemaphores;

	// VkFence inFlightFence = rw.get().currentInFlightFences();

	// vk.ResetFences(vk.vkDevice(), 1, &inFlightFence);

	if (vk.queueSubmit(vk.graphicsQueue(), submitInfo) != VK_SUCCESS)
	{
		std::cerr << "error: failed to submit draw command buffer"
		          << std::endl;
		abort();
	}

	vk.wait(vk.graphicsQueue());

	createImageViews();

	imagesInFlight.clear();
	imagesInFlight.resize(swapchainImageViews.size());

	for (auto& view : swapchainImageViews)
	{
		VkSemaphore imageAvailableSemaphore;
		VkFence inFlightFence;
		vk::SemaphoreCreateInfo semaphoreInfo;
		vk::FenceCreateInfo fenceInfo;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		if (vk.create(semaphoreInfo, imageAvailableSemaphore) != VK_SUCCESS ||
		    vk.create(fenceInfo, inFlightFence) != VK_SUCCESS)
		{
			throw std::runtime_error(
			    "error: failed to create semaphores or fence");
		}

		imageAvailableSemaphores.push_back(imageAvailableSemaphore);
		inFlightFences.push_back(inFlightFence);
	}

	swapchainCreationTime = glfwGetTime();
}

void RenderWindowPrivate::keyCallback(GLFWwindow* window, int key,
                                      int scancode, int action, int mods)
{
	RenderWindowPrivate* rwp =
	    (RenderWindowPrivate*)glfwGetWindowUserPointer(window);

	if (rwp)
		for (auto cb : rwp->keyCallbacks)
			cb(Key(key), scancode, Action(action), mods);
}

void RenderWindowPrivate::mouseButtonCallback(GLFWwindow* window, int button,
                                              int action, int mods)
{
	RenderWindowPrivate* rwp =
	    (RenderWindowPrivate*)glfwGetWindowUserPointer(window);

	if (rwp)
	{
		if (ButtonState(action) == ButtonState::Released)
			rwp->mouseButtonsStates[button] = ButtonState::Released;
		else if (ButtonState(action) == ButtonState::Pressed)
			rwp->mouseButtonsStates[button] = ButtonState::Pressed;

		for (auto cb : rwp->mouseButtonCallbacks)
			cb(MouseButton(button), Action(action), mods);
	}
}

void RenderWindowPrivate::mousePosCallback(GLFWwindow* window, double x,
                                           double y)
{
	RenderWindowPrivate* rwp =
	    (RenderWindowPrivate*)glfwGetWindowUserPointer(window);

	if (rwp)
		for (auto cb : rwp->mousePosCallbacks)
			cb(x, y);
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
}

RenderWindow::~RenderWindow() {}

void RenderWindow::pollEvents()
{
	for (auto& btn : p->mouseButtonsStates)
	{
		if (btn == ButtonState::Released)
			btn = ButtonState::Idle;
		else if (btn == ButtonState::Pressed)
			btn = ButtonState::Pressing;
	}

	glfwPollEvents();
}

// `semaphore` will be signaled
uint32_t RenderWindow::acquireNextImage(VkSemaphore semaphore, VkFence fence)
{
	const auto& vk = device();

	VkResult result =
	    vk.AcquireNextImageKHR(vk.vkDevice(), swapchain(), UINT64_MAX,
	                           semaphore, fence, &m_imageIndex);

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error("could not aquire next image");

	return m_imageIndex;
}

// `semaphore` will be signaled
uint32_t RenderWindow::acquireNextImage(VkSemaphore semaphore)
{
	return acquireNextImage(semaphore, nullptr);
}

uint32_t RenderWindow::acquireNextImage(VkFence fence)
{
	return acquireNextImage(nullptr, fence);
}

void RenderWindow::present()
{
	bool _;
	present(_);
}

void RenderWindow::present(bool& outSwapchainRecreated)
{
	outSwapchainRecreated = false;
	const auto& vk = device();

	/* m_imageIndex = */ acquireNextImage(p->acquireToCopySemaphore);
	// p->imageAvailableSemaphores[m_currentFrame]);

	VkResult result = vk.queuePresent(vk.presentQueue(), swapchain(),
	                                  m_imageIndex, p->acquireToCopySemaphore);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		int width, height;
		glfwGetFramebufferSize(p->window, &width, &height);

		p->recreateSwapchain(width, height);

		outSwapchainRecreated = true;
		return;
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("error: failed to present");
	}

	m_currentFrame = (m_currentFrame + 1) % p->swapchainImages.size();
}

// `image` will be copied to the swapchain. It must be in
// VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
void RenderWindow::present(const Texture2D& image, VkImageLayout currentLayout,
                           VkImageLayout outputLayout,
                           VkSemaphore additionalSemaphore)
{
	bool _;
	present(image, currentLayout, outputLayout, additionalSemaphore, _);
}

// `image` will be copied to the swapchain. It must be in
// VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
void RenderWindow::present(const Texture2D& image, VkImageLayout currentLayout,
                           VkImageLayout outputLayout,
                           VkSemaphore additionalSemaphore,
                           bool& outSwapchainRecreated)
{
	{
		int width, height;
		glfwGetFramebufferSize(p->window, &width, &height);

		if (width == 0 || height == 0)
			return;
	}

	outSwapchainRecreated = false;
	const auto& vk = device();

	auto& frame = getAvailableCommandBuffer();
	frame.reset();

	// vk.resetFence(frame.fence);
	// frame.commandBuffer.reset();
	frame.commandBuffer.begin();

	// vk.wait(p->imageAcquisitionFences[m_imageIndex]);
	// vk.resetFence(p->imageAcquisitionFences[m_imageIndex]);

	// waitForAllCommandBuffers();

	// acquireNextImage(p->imageAcquisitionFences[m_imageIndex]);
	size_t semaphoreIndex = m_imageIndex;
	acquireNextImage(p->imageAcquisitionSemaphores[semaphoreIndex]);

#pragma region transition and blit
	if (currentLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
	    outputLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		vk::ImageMemoryBarrier swapBarrier;
		swapBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		swapBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		swapBarrier.image = image;
		swapBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		swapBarrier.subresourceRange.baseMipLevel = 0;
		swapBarrier.subresourceRange.levelCount = 1;
		swapBarrier.subresourceRange.baseArrayLayer = 0;
		swapBarrier.subresourceRange.layerCount = 1;
		swapBarrier.oldLayout = currentLayout;
		swapBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		swapBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		swapBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		frame.commandBuffer.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
		                                    VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
		                                    swapBarrier);
	}

	vk::ImageMemoryBarrier swapBarrier;
	swapBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	swapBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	swapBarrier.image = p->swapchainImages[m_imageIndex];
	swapBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	swapBarrier.subresourceRange.baseMipLevel = 0;
	swapBarrier.subresourceRange.levelCount = 1;
	swapBarrier.subresourceRange.baseArrayLayer = 0;
	swapBarrier.subresourceRange.layerCount = 1;
	swapBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	swapBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	swapBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	swapBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	frame.commandBuffer.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
	                                    VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
	                                    swapBarrier);

	VkImageBlit blit{};
	blit.srcOffsets[1].x = image.extent3D().width;
	blit.srcOffsets[1].y = image.extent3D().height;
	blit.srcOffsets[1].z = image.extent3D().depth;
	blit.dstOffsets[1].x = swapchainExtent().width;
	blit.dstOffsets[1].y = swapchainExtent().height;
	blit.dstOffsets[1].z = 1;
	blit.srcSubresource.aspectMask =
	    VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	blit.srcSubresource.baseArrayLayer = 0;
	blit.srcSubresource.layerCount = 1;
	blit.srcSubresource.mipLevel = 0;
	blit.dstSubresource.aspectMask =
	    VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	blit.dstSubresource.baseArrayLayer = 0;
	blit.dstSubresource.layerCount = 1;
	blit.dstSubresource.mipLevel = 0;

	frame.commandBuffer.blitImage(image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	                              p->swapchainImages[m_imageIndex],
	                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, blit,
	                              VkFilter::VK_FILTER_LINEAR);

	swapBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	swapBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	swapBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	swapBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

	frame.commandBuffer.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
	                                    VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
	                                    swapBarrier);

	if (currentLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
	    outputLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		vk::ImageMemoryBarrier swapBarrier;
		swapBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		swapBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		swapBarrier.image = image;
		swapBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		swapBarrier.subresourceRange.baseMipLevel = 0;
		swapBarrier.subresourceRange.levelCount = 1;
		swapBarrier.subresourceRange.baseArrayLayer = 0;
		swapBarrier.subresourceRange.layerCount = 1;
		swapBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		swapBarrier.newLayout = outputLayout;
		swapBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		swapBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		frame.commandBuffer.pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
		                                    VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
		                                    swapBarrier);
	}
#pragma endregion

	frame.commandBuffer.end();

	vk::SubmitInfo submit;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &frame.commandBuffer.get();
	stack_vector<VkSemaphore, 3> waitSemaphores;
	stack_vector<VkPipelineStageFlags, 3> waitStages;
	waitSemaphores.push_back(p->imageAcquisitionSemaphores[semaphoreIndex]);
	waitStages.push_back(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &p->copyToPresentSemaphore.get();

	if (additionalSemaphore != nullptr)
	{
		waitSemaphores.push_back(additionalSemaphore);
		waitStages.push_back(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
	}

	submit.waitSemaphoreCount = uint32_t(waitSemaphores.size());
	submit.pWaitSemaphores = waitSemaphores.data();
	submit.pWaitDstStageMask = waitStages.data();
	vk.queueSubmit(vk.graphicsQueue(), submit, frame.fence);

	frame.submitted = true;

	VkResult result = vk.queuePresent(vk.presentQueue(), swapchain(),
	                                  m_imageIndex, p->copyToPresentSemaphore);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		int width, height;
		glfwGetFramebufferSize(p->window, &width, &height);

		p->recreateSwapchain(width, height);

		outSwapchainRecreated = true;
		return;
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("error: failed to present");
	}

	m_currentFrame = (m_currentFrame + 1) % p->swapchainImages.size();

	// vk.wait(frame.fence);
	// std::cout << "present: after wait " << uint32_t(frame.fence.get()) <<
	// "\t" << vk::to_string(vk.getStatus(frame.fence)) << std::endl;
	// frame.reset();
}

uint32_t RenderWindow::imageIndex() const { return m_imageIndex; }

size_t RenderWindow::currentFrame() const { return m_currentFrame; }

const VkSemaphore& RenderWindow::currentImageAvailableSemaphore() const
{
	return p->imageAvailableSemaphores[m_currentFrame];
}

const VkFence& RenderWindow::currentInFlightFences() const
{
	return p->inFlightFences[m_currentFrame];
}

void RenderWindow::pushPresentWaitSemaphore(VkSemaphore semaphore)
{
	p->presentWaitSemaphores.push_back(semaphore);
}

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
	// if (keyCallback)
	//{
	//	auto found = std::find(p->keyCallbacks.begin(), p->keyCallbacks.end(),
	//	                       keyCallback);
	//	if (found != p->keyCallbacks.end())
	//		p->keyCallbacks.erase(found);
	//}
}

void RenderWindow::registerMouseButtonCallback(
    PFN_mouseButtonCallback mouseButtonCallback)
{
	if (mouseButtonCallback)
		p->mouseButtonCallbacks.push_back(mouseButtonCallback);
}

void RenderWindow::unregisterMouseButtonCallback(
    PFN_mouseButtonCallback mouseButtonCallback)
{
	// if (mouseButtonCallback)
	//{
	//	auto found =
	//	    std::find(p->mouseButtonCallbacks.begin(),
	//	              p->mouseButtonCallbacks.end(), mouseButtonCallback);
	//	if (found != p->mouseButtonCallbacks.end())
	//		p->mouseButtonCallbacks.erase(found);
	//}
}

void RenderWindow::registerMousePosCallback(
    PFN_mousePosCallback mousePosCallback)
{
	if (mousePosCallback)
		p->mousePosCallbacks.push_back(mousePosCallback);
}

void RenderWindow::unregisterMousePosCallback(
    PFN_mousePosCallback mousePosCallback)
{
	// if (mousePosCallback)
	//{
	//	auto found = std::find(p->mousePosCallbacks.begin(),
	//	                       p->mousePosCallbacks.end(), mousePosCallback);
	//	if (found != p->mousePosCallbacks.end())
	//		p->mousePosCallbacks.erase(found);
	//}
}

const VulkanDevice& RenderWindow::device() const { return p->vulkanDevice; }

VkSwapchainKHR RenderWindow::swapchain() const { return p->swapchain; }

VkExtent2D RenderWindow::swapchainExtent() { return p->swapchainExtent; }

VkFormat RenderWindow::swapchainImageFormat()
{
	return p->swapchainImageFormat;
}

VkFormat RenderWindow::depthImageFormat() { return findDepthFormat(device()); }

std::vector<VkImage> RenderWindow::swapchainImages() const
{
	std::vector<VkImage> res;

	for (size_t i = 0; i < p->swapchainImages.size(); i++)
	{
		auto& image = p->swapchainImages[i];
		res.emplace_back(image.image());
	}

	return res;
}

std::vector<std::reference_wrapper<ImageView>>
RenderWindow::swapchainImageViews() const
{
	std::vector<std::reference_wrapper<ImageView>> res;

	// for (auto& iv : p->swapchainImageViews)
	for (size_t i = 0; i < p->swapchainImages.size(); i++)
	{
		auto& iv = p->swapchainImageViews[i];
		res.emplace_back(*iv.get());
	}

	return res;
}

VkCommandPool RenderWindow::commandPool() const { return p->commandPool; }

FrameCommandBuffer& RenderWindow::getAvailableCommandBuffer()
{
	const auto& vk = device();

	// std::cout << "\ngetAvailableCommandBuffer" << std::endl;
	// for (auto& frame : p->frameCommandBuffers)
	//{
	//	VkResult res = vk.getStatus(frame.fence);
	//	std::cout << "    fence " << uint32_t(frame.fence.get())
	//	          << ": \t" << (res == VK_SUCCESS ? "signaled" : "unsignaled")
	//<< " and " << (frame.submitted ? "submitted" : "unsubmitted")
	//	          << std::endl;
	//}
	// std::cout << std::endl;

	int outCommandBufferIndex = 0;
	for (auto& frame : p->frameCommandBuffers)
	{
		// VkResult res = vk.getStatus(frame.fence);
		// if (res == VK_SUCCESS)
		if (frame.isAvailable())
		{
			// std::cout << "    fence "
			//          << uint32_t(frame.fence.get()) << " is available!"
			//          << std::endl;
			return frame;
		}

		/// AAAAAAAAAAAAH
		outCommandBufferIndex++;
		if (outCommandBufferIndex >= 256)
			abort();
	}

	p->frameCommandBuffers.emplace_front(FrameCommandBuffer{
	    CommandBuffer(vk, commandPool()),
	    vk.createFence(VK_FENCE_CREATE_SIGNALED_BIT), vk.createSemaphore() });

#pragma region marker names
	vk.debugMarkerSetObjectName(
	    p->frameCommandBuffers.front().commandBuffer.get(),
	    "RenderWindow::frameCommandBuffers[" +
	        std::to_string(outCommandBufferIndex) + "].commandBuffer");
	vk.debugMarkerSetObjectName(p->frameCommandBuffers.front().fence.get(),
	                            "RenderWindow::frameCommandBuffers[" +
	                                std::to_string(outCommandBufferIndex) +
	                                "].fence");
	vk.debugMarkerSetObjectName(p->frameCommandBuffers.front().semaphore.get(),
	                            "RenderWindow::frameCommandBuffers[" +
	                                std::to_string(outCommandBufferIndex) +
	                                "].semaphore");
#pragma endregion

	// std::cout << "    fence "
	//          << uint32_t(p->frameCommandBuffers.front().fence.get())
	//          << " has been created..." << std::endl;

	// std::cout << outCommandBufferIndex << std::endl;

	return p->frameCommandBuffers.front();
}

void RenderWindow::waitForAllCommandBuffers()
{
	const auto& vk = device();

	for (auto& frame : p->frameCommandBuffers)
	{
		if (frame.submitted)
		{
			vk.wait(frame.fence);
			frame.reset();
		}
	}
}

VkCommandPool RenderWindow::oneTimeCommandPool() const
{
	return p->oneTimeCommandPool;
}

VkRenderPass RenderWindow::imguiRenderPass() const
{
	return p->imguiRenderPass.get();
}

double RenderWindow::swapchainCreationTime() const
{
	return p->swapchainCreationTime;
}

double RenderWindow::getTime() const { return glfwGetTime(); }

ButtonState RenderWindow::mouseState(MouseButton button) const
{
	return p->mouseButtonsStates[int(button)];
}

bool RenderWindow::mouseState(MouseButton button, ButtonState state) const
{
	return p->mouseButtonsStates[int(button)] == state;
}

std::pair<double, double> RenderWindow::mousePos() const
{
	std::pair<double, double> res;
	glfwGetCursorPos(p->window, &res.first, &res.second);
	return res;
}
}  // namespace cdm
