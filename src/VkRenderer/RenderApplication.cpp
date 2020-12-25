#include "RenderApplication.hpp"

#include "VulkanDevice.hpp"

namespace cdm
{
RenderApplication::RenderApplication(RenderWindow& rw)
    : RenderApplication(rw, 0, nullptr)
{
}

RenderApplication::RenderApplication(RenderWindow& rw, int argc,
                                     const char** argv)
    : renderWindow(rw),
      vk(rw.device())
{
	for (auto& state : m_mouseButtonStates)
		state = ButtonState::Idle;
	for (auto& state : m_keyStates)
		state = ButtonState::Idle;

	rw.registerMouseButtonCallback(
	    [this](MouseButton button, Action action, int mods) {
			m_mouseButtonStates[int(button)] = ButtonState(action);

			if (action == Action::Press)
			    this->onMouseDown(button);
		    else if (action == Action::Release)
			    this->onMouseUp(button);
	    });

	rw.registerMousePosCallback([this](double x, double y) { 
		m_mouseDeltaX = m_mousePosX - x;
		m_mouseDeltaY = m_mousePosY - y;
		m_mousePosX = x;
		m_mousePosY = y;

		this->onMouseMove(m_mousePosX, m_mousePosY, m_mouseDeltaX,
		                  m_mouseDeltaY);
	});

	rw.registerKeyCallback(
	    [this](Key key, int scancode, Action action, int mods) {
		    m_keyStates[int(key)] = ButtonState(action);

		    if (action == Action::Press)
			    this->onKeyDown(key);
		    else if (action == Action::Release)
			    this->onKeyUp(key);		    
	    });
}

int RenderApplication::run()
{
	//auto start = std::chrono::steady_clock::now();
	//auto end = std::chrono::steady_clock::now();
	//std::chrono::duration<double> elapsed_seconds = end - start;

	while (!renderWindow.shouldClose())
	{
		//end = std::chrono::steady_clock::now();
		//elapsed_seconds = end - start;
		//deltaTime = elapsed_seconds.count();
		//start = std::chrono::steady_clock::now();

		pollEvents();

		draw();
	}

	vk.wait(vk.graphicsQueue());
	vk.wait();

	return 0;
}

void RenderApplication::pollEvents()
{
	for (auto& state : m_mouseButtonStates)
	{
		if (state == ButtonState::Pressed)
			state = ButtonState::Pressing;
		else if (state == ButtonState::Released)
			state = ButtonState::Idle;
	}

	for (auto& state : m_keyStates)
	{
		if (state == ButtonState::Pressed)
			state = ButtonState::Pressing;
		else if (state == ButtonState::Released)
			state = ButtonState::Idle;
	}

	m_mouseDeltaX = 0.0;
	m_mouseDeltaY = 0.0;

	renderWindow.pollEvents();
}
}  // namespace cdm
