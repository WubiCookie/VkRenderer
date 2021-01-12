#ifndef VKRENDERER_RENDERAPPLICATION_HPP
#define VKRENDERER_RENDERAPPLICATION_HPP 1

#include "RenderWindow.hpp"

#include <array>

namespace cdm
{
class VulkanDevice;

class RenderApplication
{
	std::array<ButtonState, int(MouseButton::COUNT)> m_mouseButtonStates;
	std::array<ButtonState, int(Key::COUNT)> m_keyStates;

	double m_mousePosX;
	double m_mousePosY;

	double m_mouseDeltaX;
	double m_mouseDeltaY;

protected:
	RenderWindow& renderWindow;
	const VulkanDevice& vk;

	// double deltaTime;

public:
	RenderApplication(RenderWindow& rw);
	RenderApplication(RenderWindow& rw, int argc, const char** argv);
	~RenderApplication() = default;

	virtual int run();

	ButtonState getMouseButtonState(MouseButton button) const
	{
		return m_mouseButtonStates[int(button)];
	}
	ButtonState getKeyState(Key key) const { return m_keyStates[int(key)]; }

	void getMousePos(double& x, double& y)
	{
		x = m_mousePosX;
		y = m_mousePosY;
	}
	void getMouseDelta(double& x, double& y)
	{
		x = m_mouseDeltaX;
		y = m_mouseDeltaY;
	}

protected:
	virtual void pollEvents();
	virtual void update(){};
	virtual void draw(){};

	virtual void onMouseDown(MouseButton button){};
	virtual void onMouseUp(MouseButton button){};
	virtual void onMouseMove(double x, double y, double deltaX,
	                         double deltaY){};
	virtual void onKeyDown(Key key){};
	virtual void onKeyUp(Key key){};
};
}  // namespace cdm

#endif  // !VKRENDERER_RENDERAPPLICATION_HPP
