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

	std::uniform_real_distribution<float> urd(0.0f,
	                                          2.0f * constants<float>::Pi());
	std::uniform_real_distribution<float> urdcol(0.0f, 1.0f);

	std::vector<Line> lines(VERTEX_BUFFER_LINE_COUNT);
	for (auto& line : lines)
	{
		radian theta(urd(gen));
		vector2 dir{ cos(theta), sin(theta) };

		line.B.pos = dir * 2.0f;
		line.A.col.x = urdcol(gen);
		line.A.col.y = urdcol(gen);
		line.A.col.z = urdcol(gen);
		line.B.col = line.A.col;

		dir = line.B.pos - line.A.pos;
		dir.normalize();

		line.A.dir = dir;
		line.B.dir = dir;
	}

	m_vertexBuffer = Buffer(
	    rw, sizeof(Line) * VERTEX_BUFFER_LINE_COUNT,
	    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
	        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	    VMA_MEMORY_USAGE_CPU_TO_GPU, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	//fillVertexBuffer();
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
