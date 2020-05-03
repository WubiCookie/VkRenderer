#pragma once

#include "VulkanDevice.hpp"

#include "CommandBuffer.hpp"
#include "RenderWindow.hpp"
#include "Buffer.hpp"
#include "Texture.hpp"

#include <random>
#include <memory>

namespace cdm
{
class Mandelbulb final
{
	std::reference_wrapper<RenderWindow> rw;

	UniqueRenderPass m_renderPass;

	UniqueFramebuffer m_framebuffer;

	UniqueShaderModule m_vertexModule;
	UniqueShaderModule m_fragmentModule;
	UniqueShaderModule m_computeModule;

	UniquePipelineLayout m_pipelineLayout;
	UniquePipeline m_pipeline;

	UniqueDescriptorPool m_computePool;
	UniqueDescriptorSetLayout m_computeSetLayout;
	Moveable<VkDescriptorSet> m_computeSet;
	UniquePipelineLayout m_computePipelineLayout;
	UniquePipeline m_computePipeline;

	Buffer m_vertexBuffer;
	Buffer m_computeUbo;
	Texture2D m_outputImage;
	Texture2D m_outputImageHDR;

	//Moveable<VmaAllocation> m_vertexBufferAllocation;
	//Moveable<VkBuffer> m_vertexBuffer;

	//Moveable<VmaAllocation> m_computeUboAllocation;
	//Moveable<VkBuffer> m_computeUbo;

	//Moveable<VmaAllocation> m_outputImageAllocation;
	//Moveable<VkImage> m_outputImage;
	//UniqueImageView m_outputImageView;

	//Moveable<VmaAllocation> m_outputImageHDRAllocation;
	//Moveable<VkImage> m_outputImageHDR;
	//UniqueImageView m_outputImageHDRView;

	std::random_device rd;
	std::mt19937 gen;
	std::normal_distribution<float> dis;

public:
	Mandelbulb(RenderWindow& renderWindow);
	~Mandelbulb();

	void render(CommandBuffer& cb);

	void compute(CommandBuffer& cb);

	void randomizePoints();

	VkImage outputImage() const { return m_outputImage.get(); }
	VkImage outputImageHDR() const { return m_outputImageHDR.get(); }
};
}  // namespace cdm
