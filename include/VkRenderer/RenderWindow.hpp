#pragma once

#define NOCOMM
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#define VK_NO_PROTOTYPES
#include "cdm_vulkan.hpp"

#include <memory>
#include <vector>

class VulkanDevice;
class ImageView;

struct RenderWindowPrivate;

typedef void (*PFN_keyCallback)(int key, int scancode, int action, int mods);

class RenderWindow
{
	std::unique_ptr<RenderWindowPrivate> p;

	VkCommandPool m_commandPool = nullptr;
	VkCommandPool m_oneTimeCommandPool = nullptr;

public:
	RenderWindow(int width, int height, bool layers = false);
	RenderWindow(const RenderWindow&) = delete;
	RenderWindow(RenderWindow&&) = delete;
	~RenderWindow();

	RenderWindow& operator=(const RenderWindow&) = delete;
	RenderWindow& operator=(RenderWindow&&) = delete;

	void pollEvents();

	void show();
	void hide();
	bool visible();

	void size(int& width, int& height);
	void pos(int& xpos, int& ypos);
	void framebufferSize(int& width, int& height);

	void setShouldClose(bool close);
	bool shouldClose();

	void iconify();
	void maximize();
	void restore();
	bool iconified();
	bool maximized();

	void focus();
	bool focused();

	void registerKeyCallback(PFN_keyCallback keyCallback);
	void unregisterKeyCallback(PFN_keyCallback keyCallback);

	const VulkanDevice& device() const;

	VkSwapchainKHR swapchain() const;
	VkExtent2D swapchainExtent();
	VkFormat swapchainImageFormat();
	std::vector<std::reference_wrapper<ImageView>> swapchainImageViews() const;

	VkCommandPool commandPool() const;
	// VkCommandPool oneTimeCommandPool() const;
};
