#pragma once

#include "VulkanDevice.hpp"

#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include "RenderWindow.hpp"
#include "Texture1D.hpp"
#include "Texture2D.hpp"

#include "cdm_maths.hpp"

#include <memory>
#include <random>

//constexpr size_t RAYS_COUNT = 1024 * 2;
//constexpr size_t RAYS_COUNT = 8;
//constexpr size_t THREAD_COUNT = 16;
constexpr size_t THREAD_COUNT = 8;
constexpr size_t VERTEX_BUFFER_LINE_COUNT = 4096*4;
//constexpr size_t VERTEX_BUFFER_LINE_COUNT = 1024;
//constexpr size_t VERTEX_BATCH_COUNT = 2048*64;
//constexpr size_t VERTEX_BATCH_COUNT = 2048*1;
constexpr size_t VERTEX_BATCH_COUNT = 8;
constexpr float HDR_SCALE = 1.0f;

namespace cdm
{
struct RayIteration
{
	struct Vec2
	{
		float x = 0;
		float y = 0;
	};
	struct Vec3
	{
		float x = 0;
		float y = 0;
		float z = 0;
		float _ = 0;
	};

	Vec2 position;
	Vec2 direction;

	// Color		Wavelength	Frequency
	// Violet	380–450 nm	680–790 THz
	// Blue		450–485 nm	620–680 THz
	// Cyan		485–500 nm	600–620 THz
	// Green	500–565 nm	530–600 THz
	// Yellow	565–590 nm	510–530 THz
	// Orange	590–625 nm	480–510 THz
	// Red		625–740 nm	405–480 THz
	float waveLength = 625.0f;
	//float amplitude = 1.0f;
	//float phase = 0.0f;
	//float currentRefraction = 1.0f;

	float frequency = 4.8e14f;
	//float speed = 3e8f;
	float rng = 0.0f;
	float _0;
};

class LightTransport final
{
	std::reference_wrapper<RenderWindow> rw;

	UniqueRenderPass m_renderPass;
	UniqueRenderPass m_blitRenderPass;

	UniqueFramebuffer m_framebuffer;
	UniqueFramebuffer m_blitFramebuffer;

	UniqueDescriptorPool m_descriptorPool;

	std::vector<UniqueDescriptorSetLayout> m_setLayouts;
	Movable<VkDescriptorSet> m_descriptorSet;
	UniquePipelineLayout m_pipelineLayout;
	UniqueGraphicsPipeline m_pipeline;

	std::vector<UniqueDescriptorSetLayout> m_blitSetLayouts;
	Movable<VkDescriptorSet> m_blitDescriptorSet;
	UniquePipelineLayout m_blitPipelineLayout;
	UniqueGraphicsPipeline m_blitPipeline;

	std::vector<UniqueDescriptorSetLayout> m_traceSetLayouts;
	Movable<VkDescriptorSet> m_traceDescriptorSet;
	UniquePipelineLayout m_tracePipelineLayout;
	UniqueComputePipeline m_tracePipeline;

	Buffer m_raysBuffer;
	Buffer m_vertexBuffer;
	//Buffer m_computeUbo;

	Texture2D m_outputImage;
	Texture2D m_outputImageHDR;
	//Texture1D m_waveLengthTransferFunction;

	std::random_device rd;
	std::mt19937 gen;

	struct Vertex
	{
		vector2 pos;
		vector2 dir;
		vector4 col{ 1.0f, 1.0f, 1.0f, 1.0f };
	};
	struct Line
	{
		Vertex A, B;
	};

public:
	struct Config
	{
		struct vec2
		{
			float x = 0.0f;
			float y = 0.0f;
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
		int32_t raysCount = VERTEX_BUFFER_LINE_COUNT;
		float raySize = 1.5f;

		float airRefractionScale = 0.0f;
		float sphereRefractionScale = 0.5f;

		void copyTo(void* ptr);
	};

private:
	Config m_config;

	void applyImguiParameters();

	void createRenderPasses();
	void createShaderModules();
	void createDescriptorsObjects();
	void createPipelines();
	void createBuffers();
	void createImages();
	void createFramebuffers();
	void updateDescriptorSets();

	void fillVertexBuffer();
	void fillRaysBuffer();

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
