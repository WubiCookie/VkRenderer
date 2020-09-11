#pragma once

#include "VulkanDevice.hpp"

#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include "RenderWindow.hpp"
#include "Texture2D.hpp"

#include <memory>
#include <random>

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

	UniquePipeline m_pipeline;

	UniqueDescriptorPool m_computePool;
	UniqueDescriptorSetLayout m_computeSetLayout;
	Movable<VkDescriptorSet> m_computeSet;
	UniquePipelineLayout m_computePipelineLayout;
	UniquePipeline m_computePipeline;

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
		struct vec3
		{
			float x;
			float y;
			float z;
		};

		float camAperture{ 0.1f };
		float camFocalDistance{ 14.2f };
		float camFocalLength{ 5.0f };
		float density{ 500.0f };
		float maxAbso{ 0.7f };
		float maxShadowAbso{ 0.7f };
		float power{ 8.0f };
		float samples{ 0.0f };
		float sceneRadius{ 1.5f };
		float seed{ 0.0f };

	private:
		float stepSize;

	public:
		float stepsSkipShadow{ 4.0f };
		float volumePrecision{ 0.35f };
		int32_t maxSteps{ 500 };

	private:
		float _00{0.0f};
		float _01{0.0f};

	public:
		vec3 camRot{ 0.7f, -2.4f, 0.0f };

	private:
		float _0{0.0f};

	public:
		vec3 lightColor{ 3.0f, 3.0f, 3.0f };

	private:
		float _1{0.0f};

	public:
		vec3 lightDir{ -1.0f, 0.0f, 0.0f };

	private:
		float _2{0.0f};

	public:
		float bloomAscale1{ 0.15f };
		float bloomAscale2{ 0.3f };
		float bloomBscale1{ 0.05f };
		float bloomBscale2{ 0.2f };
		
		int iterations{ 8 };

		Config()
		    : stepSize{ std::min(1.0f / (volumePrecision * density),
			                     sceneRadius / 2.0f) }
		{
		}

		void copyTo(void* ptr);
	};

private:
	Config m_config;

	float CamFocalDistance;
	float CamFocalLength;
	float CamAperture;

	CommandBuffer computeCB;// (vk, rw.oneTimeCommandPool());
	CommandBuffer imguiCB;// (vk, rw.oneTimeCommandPool());
	CommandBuffer copyHDRCB;// (vk, rw.oneTimeCommandPool());
	CommandBuffer cb;// (vk, rw.oneTimeCommandPool());

	void applyImguiParameters();

public:
	Mandelbulb(RenderWindow& renderWindow);
	~Mandelbulb();

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

	bool mustClear = false;
};
}  // namespace cdm
