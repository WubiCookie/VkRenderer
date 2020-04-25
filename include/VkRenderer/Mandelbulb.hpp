#pragma once

#include "VulkanDevice.hpp"

#include "CommandBuffer.hpp"
#include "RenderWindow.hpp"

#include <random>

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

	Moveable<VmaAllocation> m_vertexBufferAllocation;
	Moveable<VkBuffer> m_vertexBuffer;

	Moveable<VmaAllocation> m_outputImageAllocation;
	Moveable<VkImage> m_outputImage;
	Moveable<VkImageView> m_outputImageView;

	Moveable<VmaAllocation> m_outputImageHDRAllocation;
	Moveable<VkImage> m_outputImageHDR;
	Moveable<VkImageView> m_outputImageHDRView;

	std::random_device rd;
	std::mt19937 gen;
	std::normal_distribution<float> dis;

public:
	Mandelbulb(RenderWindow& rw);
	~Mandelbulb();

	void render(CommandBuffer& cb);

	void randomizePoints();

	VkImage outputImage() const { return m_outputImage.get(); }
	VkImage outputImageHDR() const { return m_outputImageHDR.get(); }
};
}  // namespace cdm
