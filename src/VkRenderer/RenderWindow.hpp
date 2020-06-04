#pragma once

#define NOCOMM
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#define VK_NO_PROTOTYPES
#include "cdm_vulkan.hpp"

#include <functional>
#include <memory>
#include <vector>

namespace cdm
{
class VulkanDevice;
class ImageView;

struct RenderWindowPrivate;

enum class Key : int
{
	Unknown = -1,
	Space = 32,
	Apostrophe = 39 /* ' */,
	Comma = 44 /* , */,
	Minus = 45 /* - */,
	Period = 46 /* . */,
	Slash = 47 /* / */,
	_0 = 48,
	_1 = 49,
	_2 = 50,
	_3 = 51,
	_4 = 52,
	_5 = 53,
	_6 = 54,
	_7 = 55,
	_8 = 56,
	_9 = 57,
	Semicolon = 59 /* ; */,
	Equal = 61 /* = */,
	A = 65,
	B = 66,
	C = 67,
	D = 68,
	E = 69,
	F = 70,
	G = 71,
	H = 72,
	I = 73,
	J = 74,
	K = 75,
	L = 76,
	M = 77,
	N = 78,
	O = 79,
	P = 80,
	Q = 81,
	R = 82,
	S = 83,
	T = 84,
	U = 85,
	V = 86,
	W = 87,
	X = 88,
	Y = 89,
	Z = 90,
	LeftBracket = 91 /* [ */,
	Backslash = 92 /* \ */,
	RightBracket = 93 /* ] */,
	GraveAccent = 96 /* ` */,
	World_1 = 161 /* non-us #1 */,
	World_2 = 162 /* non-us #2 */,
	Escape = 256,
	Enter = 257,
	Tab = 258,
	Backspace = 259,
	Insert = 260,
	Delete = 261,
	Right = 262,
	Left = 263,
	Down = 264,
	Up = 265,
	PageUp = 266,
	PageDown = 267,
	Home = 268,
	End = 269,
	CapsLock = 280,
	ScrollLock = 281,
	NumLock = 282,
	PrintScreen = 283,
	Pause = 284,
	F1 = 290,
	F2 = 291,
	F3 = 292,
	F4 = 293,
	F5 = 294,
	F6 = 295,
	F7 = 296,
	F8 = 297,
	F9 = 298,
	F10 = 299,
	F11 = 300,
	F12 = 301,
	F13 = 302,
	F14 = 303,
	F15 = 304,
	F16 = 305,
	F17 = 306,
	F18 = 307,
	F19 = 308,
	F20 = 309,
	F21 = 310,
	F22 = 311,
	F23 = 312,
	F24 = 313,
	F25 = 314,
	Kp0 = 320,
	Kp1 = 321,
	Kp2 = 322,
	Kp3 = 323,
	Kp4 = 324,
	Kp5 = 325,
	Kp6 = 326,
	Kp7 = 327,
	Kp8 = 328,
	Kp9 = 329,
	KpDecimal = 330,
	KpDivide = 331,
	KpMultiply = 332,
	KpSubtract = 333,
	KpAdd = 334,
	KpEnter = 335,
	KpEqual = 336,
	LeftShift = 340,
	LeftControl = 341,
	LeftAlt = 342,
	LeftSuper = 343,
	RightShift = 344,
	RightControl = 345,
	RightAlt = 346,
	RightSuper = 347,
	Menu = 348,
	COUNT
};

enum class MouseButton : int
{
	_1,
	_2,
	_3,
	_4,
	_5,
	_6,
	_7,
	_8,
	Left = _1,
	Right,
	Middle,
	COUNT
};

enum class Action : int
{
	Release,
	Press,
	Repeat
};

enum class ButtonState : int
{
	Released = int(Action::Release),
	Pressed = int(Action::Press),
	Idle,
	Pressing,
};

using PFN_keyCallback = std::function<void(Key, int, Action, int)>;
using PFN_mouseButtonCallback = std::function<void(MouseButton, Action, int)>;
using PFN_mousePosCallback = std::function<void(double, double)>;

class RenderWindow
{
	std::unique_ptr<RenderWindowPrivate> p;

	uint32_t m_imageIndex = -1;
	size_t m_currentFrame = 0;

public:
	RenderWindow(int width, int height, bool layers = false);
	RenderWindow(const RenderWindow&) = delete;
	RenderWindow(RenderWindow&&) = delete;
	~RenderWindow();

	RenderWindow& operator=(const RenderWindow&) = delete;
	RenderWindow& operator=(RenderWindow&&) = delete;

	void pollEvents();
	uint32_t acquireNextImage(VkSemaphore semaphore, VkFence fence);
	uint32_t acquireNextImage(VkSemaphore semaphore);
	uint32_t acquireNextImage(VkFence fence);
	void prerender();
	void present();
	void present(bool& outSwapchainRecreated);
	// void presentImage(VkImage image);

	uint32_t imageIndex() const;
	size_t currentFrame() const;

	VkSemaphore currentImageAvailableSemaphore() const;
	VkFence currentInFlightFences() const;

	void pushPresentWaitSemaphore(VkSemaphore semaphore);

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

	void registerMouseButtonCallback(
	    PFN_mouseButtonCallback mouseButtonCallback);
	void unregisterMouseButtonCallback(
	    PFN_mouseButtonCallback mouseButtonCallback);

	void registerMousePosCallback(PFN_mousePosCallback mousePosCallback);
	void unregisterMousePosCallback(PFN_mousePosCallback mousePosCallback);

	const VulkanDevice& device() const;

	VkSwapchainKHR swapchain() const;
	VkExtent2D swapchainExtent();
	VkFormat swapchainImageFormat();
	VkFormat depthImageFormat();
	std::vector<VkImage> swapchainImages() const;
	std::vector<std::reference_wrapper<ImageView>> swapchainImageViews() const;

	VkCommandPool commandPool() const;
	VkCommandPool oneTimeCommandPool() const;

	VkRenderPass imguiRenderPass() const;

	double swapchainCreationTime() const;

	double getTime() const;

	ButtonState mouseState(MouseButton button) const;
	bool mouseState(MouseButton button, ButtonState state) const;

	std::pair<double, double> mousePos() const;
};
}  // namespace cdm
