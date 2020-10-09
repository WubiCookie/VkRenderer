#include "LightTransport.hpp"
#include "cdm_maths.hpp"

#include <array>
#include <iostream>
#include <stdexcept>

constexpr size_t POINT_COUNT = 2000;

#define TEX_SCALE 1

constexpr uint32_t width = 1280 * TEX_SCALE;
constexpr uint32_t height = 720 * TEX_SCALE;
constexpr float widthf = 1280.0f * TEX_SCALE;
constexpr float heightf = 720.0f * TEX_SCALE;

namespace cdm
{
void LightTransport::createBuffers()
{
	auto& vk = rw.get().device();

#pragma region vertexBuffer
	m_vertexBuffer = Buffer(
	    rw, sizeof(Line) * VERTEX_BUFFER_LINE_COUNT * BUMPS,
	    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
#pragma endregion

#pragma region ray compute UBO
	m_raysBuffer =
	    Buffer(rw, sizeof(RayIteration) * VERTEX_BUFFER_LINE_COUNT,
	           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
	           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	fillRaysBuffer();
#pragma endregion

	/*
#pragma region color compute UBO
	m_computeUbo = Buffer(
	    rw, sizeof(m_config), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	    VMA_MEMORY_USAGE_CPU_TO_GPU, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	m_config.copyTo(m_computeUbo.map());
	m_computeUbo.unmap();
#pragma endregion
	//*/
}
}  // namespace cdm
