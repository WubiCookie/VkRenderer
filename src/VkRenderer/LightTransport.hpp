#pragma once

#include "VulkanDevice.hpp"

#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include "RenderWindow.hpp"
#include "Texture2D.hpp"

#include <memory>
#include <random>

constexpr size_t RAYS_COUNT = 1024;
constexpr size_t THREAD_COUNT = 16;

namespace cdm
{
class LightTransport final
{
	std::reference_wrapper<RenderWindow> rw;

	UniqueRenderPass m_renderPass;

	UniqueFramebuffer m_framebuffer;

	UniqueShaderModule m_vertexModule;
	UniqueShaderModule m_fragmentModule;
	UniqueShaderModule m_rayComputeModule;
	UniqueShaderModule m_colorComputeModule;

	UniquePipeline m_pipeline;

	UniqueDescriptorPool m_computePool;
	UniqueDescriptorSetLayout m_rayComputeSetLayout;
	UniqueDescriptorSetLayout m_colorComputeSetLayout;
	Movable<VkDescriptorSet> m_rayComputeSet;
	Movable<VkDescriptorSet> m_colorComputeSet;
	UniquePipelineLayout m_rayComputePipelineLayout;
	UniquePipelineLayout m_colorComputePipelineLayout;
	UniquePipeline m_rayComputePipeline;
	UniquePipeline m_colorComputePipeline;

	Buffer m_raysBuffer;
	Buffer m_vertexBuffer;
	Buffer m_computeUbo;
	Texture2D m_outputImage;
	Texture2D m_outputImageHDR;

	std::random_device rd;
	std::mt19937 gen;
	std::normal_distribution<float> dis;
	std::uniform_real_distribution<float> udis;

public:
	struct Config
	{
		struct vec2
		{
			float x = 0.0f;
			float y = 0.0f;
			//float _0 = 0.0f;
			//float _1 = 0.0f;
		};
		struct vec3
		{
			float x = 0.0f;
			float y = 0.0f;
			float z = 0.0f;
			float _ = 0.0f;
		};

		vec2 spherePos;

		float sphereRadius = 50.0f;
		float seed = 0.0f;
		float airRefraction = 1.0f;
		float sphereRefraction = 0.7f;
		float deltaT = 1.0f;
		float lightsSpeed = 1.0f;

		int32_t raysCount = RAYS_COUNT;

		void copyTo(void* ptr);
	};

private:
	Config m_config;

	void applyImguiParameters();

public:
	LightTransport(RenderWindow& renderWindow);
	~LightTransport();

	void render(CommandBuffer& cb);
	void compute(CommandBuffer& cb);
	void imgui(CommandBuffer& cb);

	void standaloneDraw();

	void randomizePoints();
	void setSampleAndRandomize(float s);

	VkImage outputImage() const { return m_outputImage.get(); }
	VkImage outputImageHDR() const { return m_outputImageHDR.get(); }
	Texture2D& outputTexture() { return m_outputImage; }
	Texture2D& outputTextureHDR() { return m_outputImageHDR; }
};
}  // namespace cdm
