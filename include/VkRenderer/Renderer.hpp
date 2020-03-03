#pragma once

#include "Material.hpp"
#include "RenderPass.hpp"
#include "VulkanDevice.hpp"

#include <vector>

namespace cdm
{
class RenderWindow;
class CommandBuffer;
class Framebuffer;

class Renderer
{
	std::reference_wrapper<RenderWindow> rw;

	std::vector<CommandBuffer> m_commandBuffers;
	std::vector<Framebuffer> m_framebuffers;

	std::vector<VkSemaphore> m_renderFinishedSemaphores;

	RenderPass m_defaultRenderPass;
	Material m_defaultMaterial;

public:
	Renderer(RenderWindow& renderWindow);

	void render();

	std::vector<CommandBuffer>& commandBuffers();
	std::vector<Framebuffer>& framebuffers();

	RenderPass& defaultRenderPass();
	Material& defaultMaterial();
};
}  // namespace cdm
