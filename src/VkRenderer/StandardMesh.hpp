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
	Movable<const VulkanDevice*> device;

	uint32_t m_verticesCount = 0;
	uint32_t m_indicesCount = 0;

	Buffer m_vertexBuffer;
	Buffer m_positionBuffer;
	Buffer m_indexBuffer;

public:
	StandardMesh() = default;
	StandardMesh(const VulkanDevice& vulkanDevice,
	             const std::vector<Vertex>& vertices,
	             const std::vector<uint32_t>& indices);
	StandardMesh(const StandardMesh&) = delete;
	StandardMesh(StandardMesh&&) = default;
	~StandardMesh() = default;

	StandardMesh& operator=(const StandardMesh&) = delete;
	StandardMesh& operator=(StandardMesh&&) = default;

	void draw(CommandBuffer& cb);
	void drawPositions(CommandBuffer& cb);

	// const std::vector<Vertex>& vertices() const noexcept { return
	// m_vertices; } const std::vector<vector4>& positions() const noexcept
	//{
	//	return m_positions;
	//}
	// const std::vector<uint32_t>& indices() const noexcept { return
	// m_indices; }

	// uint32_t verticesCount() const noexcept { return m_verticesCount; }
	// uint32_t indicesCount() const noexcept { return m_indicesCount; }

	// const Buffer& vertexBuffer() const noexcept { return m_vertexBuffer; }
	// const Buffer& positionBuffer() const noexcept { return m_positionBuffer;
	// } const Buffer& indexBuffer() const noexcept { return m_indexBuffer; }

	static VertexInputState vertexInputState();
	static VertexInputState positionOnlyVertexInputState();
	static ShaderVertexInput shaderVertexInput(sdw::VertexWriter& writer);
};
}  // namespace cdm
