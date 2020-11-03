#pragma once

#include "Buffer.hpp"
#include "VulkanDevice.hpp"
#include "VulkanHelperStructs.hpp"

#include "ShaderWriter/Intrinsics/Intrinsics.hpp"
#include "ShaderWriter/Source.hpp"

#include "cdm_maths.hpp"

namespace cdm
{
class CommandBuffer;
class RenderWindow;

class StandardMesh final
{
public:
	struct Vertex
	{
		vector3 position;
		vector3 normal;
		vector2 uv;
		vector3 tangent;
	};

	struct ShaderVertexInput
	{
		sdw::Vec3 inPosition;
		sdw::Vec3 inNormal;
		sdw::Vec2 inUV;
		sdw::Vec3 inTangent;
	};

private:
	Movable<RenderWindow*> rw;

	std::vector<Vertex> m_vertices;
	std::vector<vector4> m_positions;
	std::vector<uint32_t> m_indices;

	Buffer m_vertexBuffer;
	Buffer m_positionBuffer;
	Buffer m_indexBuffer;

public:
	StandardMesh() = default;
	StandardMesh(RenderWindow& rw, std::vector<Vertex> vertices,
	             std::vector<uint32_t> indices);
	StandardMesh(const StandardMesh&) = delete;
	StandardMesh(StandardMesh&&) = default;
	~StandardMesh() = default;

	StandardMesh& operator=(const StandardMesh&) = delete;
	StandardMesh& operator=(StandardMesh&&) = default;

	void draw(CommandBuffer& cb);

	const std::vector<Vertex>& vertices() const noexcept { return m_vertices; }
	const std::vector<vector4>& positions() const noexcept
	{
		return m_positions;
	}
	const std::vector<uint32_t>& indices() const noexcept { return m_indices; }

	const Buffer& vertexBuffer() const noexcept { return m_vertexBuffer; }
	const Buffer& positionBuffer() const noexcept { return m_positionBuffer; }
	const Buffer& indexBuffer() const noexcept { return m_indexBuffer; }

	static VertexInputState vertexInputState();
	static VertexInputState positionOnlyVertexInputState();
	static ShaderVertexInput shaderVertexInput(sdw::VertexWriter& writer);
};
}  // namespace cdm
