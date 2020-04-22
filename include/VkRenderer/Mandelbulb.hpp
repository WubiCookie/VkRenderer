#pragma once

#include "VulkanDevice.hpp"

#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include "cdm_vulkan.hpp"
#include "vk_mem_alloc.h"

#include "CommandBuffer.hpp"
#include "RenderWindow.hpp"

namespace cdm
{
class Mandelbulb final
{
	std::reference_wrapper<RenderWindow> rw;

	Moveable<VkRenderPass> m_renderPass;

	Moveable<VkFramebuffer> m_framebuffer;

	Moveable<VkShaderModule> m_vertexModule;
	Moveable<VkShaderModule> m_fragmentModule;

	Moveable<VkPipelineLayout> m_pipelineLayout;
	Moveable<VkPipeline> m_pipeline;

	Moveable<VmaAllocator> m_allocator;

	Moveable<VmaAllocation> m_vertexBufferAllocation;
	Moveable<VkBuffer> m_vertexBuffer;

	Moveable<VmaAllocation> m_outputImageAllocation;
	Moveable<VkImage> m_outputImage;
	Moveable<VkImageView> m_outputImageView;

public:
	Mandelbulb(RenderWindow& rw);
	~Mandelbulb();

	void render(CommandBuffer& cb);

	VkImage outputImage() const { return m_outputImage.get(); }
};
}  // namespace cdm
