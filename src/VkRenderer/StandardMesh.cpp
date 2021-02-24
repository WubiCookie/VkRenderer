#include "StandardMesh.hpp"

#include "CommandBuffer.hpp"
#include "CommandBufferPool.hpp"

#include <stdexcept>

namespace cdm
{
StandardMesh::StandardMesh(const VulkanDevice& vulkanDevice,
                           const std::vector<Vertex>& vertices,
                           const std::vector<uint32_t>& indices)
    : device(&vulkanDevice)
{
	auto& vk = *device;

	std::vector<vector4> positions;

	positions.reserve(vertices.size());
	for (const auto& vertex : vertices)
		positions.push_back(vector4(vertex.position, 1.0f));

	ResettableCommandBufferPool pool(
	    vk, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	auto& frame = pool.getAvailableCommandBuffer();

	CommandBuffer& copyCB = frame.commandBuffer;

#pragma region vertexBuffer
	m_verticesCount = vertices.size();

	auto vertexStagingBuffer = Buffer(
	    vk, sizeof(Vertex) * vertices.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	    VMA_MEMORY_USAGE_CPU_TO_GPU, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	m_vertexBuffer = Buffer(
	    vk, sizeof(Vertex) * vertices.size(),
	    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	Vertex* vertexData = vertexStagingBuffer.map<Vertex>();
	std::copy(vertices.begin(), vertices.end(), vertexData);
	vertexStagingBuffer.unmap();

	copyCB.reset();
	copyCB.begin();
	copyCB.copyBuffer(vertexStagingBuffer.get(), m_vertexBuffer.get(),
	                  sizeof(Vertex) * vertices.size());
	copyCB.end();

	if (vk.queueSubmit(vk.graphicsQueue(), copyCB.get()) != VK_SUCCESS)
		throw std::runtime_error("could not copy vertex buffer");
#pragma endregion

#pragma region positionBuffer
	auto positionStagingBuffer =
	    Buffer(vk, sizeof(vector4) * positions.size(),
	           VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
	           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	m_positionBuffer = Buffer(
	    vk, sizeof(vector4) * positions.size(),
	    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vector4* positionData = positionStagingBuffer.map<vector4>();
	std::copy(positions.begin(), positions.end(), positionData);
	positionStagingBuffer.unmap();

	vk.wait(vk.graphicsQueue());
	copyCB.reset();
	copyCB.begin();
	copyCB.copyBuffer(positionStagingBuffer.get(), m_positionBuffer.get(),
	                  sizeof(vector4) * positions.size());
	copyCB.end();

	if (vk.queueSubmit(vk.graphicsQueue(), copyCB.get()) != VK_SUCCESS)
		throw std::runtime_error("could not copy position buffer");

#pragma endregion

#pragma region indexBuffer
	m_indicesCount = uint32_t(indices.size());

	auto indexStagingBuffer =
	    Buffer(vk, sizeof(uint32_t) * indices.size(),
	           VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
	           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	m_indexBuffer = Buffer(
	    vk, sizeof(uint32_t) * indices.size(),
	    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	uint32_t* indexData = indexStagingBuffer.map<uint32_t>();
	std::copy(indices.begin(), indices.end(), indexData);
	indexStagingBuffer.unmap();

	vk.wait(vk.graphicsQueue());
	copyCB.reset();
	copyCB.begin();
	copyCB.copyBuffer(indexStagingBuffer.get(), m_indexBuffer.get(),
	                  sizeof(uint32_t) * indices.size());
	copyCB.end();

	if (vk.queueSubmit(vk.graphicsQueue(), copyCB.get()) != VK_SUCCESS)
		throw std::runtime_error("could not copy index buffer");

	vk.wait(vk.graphicsQueue());
#pragma endregion
}

void StandardMesh::draw(CommandBuffer& cb)
{
	cb.bindVertexBuffer(m_vertexBuffer.get());
	if (m_indicesCount != 0)
	{
		cb.bindIndexBuffer(m_indexBuffer.get(), 0, VK_INDEX_TYPE_UINT32);
		cb.drawIndexed(m_indicesCount);
	}
	else
	{
		cb.draw(m_verticesCount);
	}
}

void StandardMesh::drawPositions(CommandBuffer& cb)
{
	cb.bindVertexBuffer(m_positionBuffer.get());
	if (m_indicesCount != 0)
	{
		cb.bindIndexBuffer(m_indexBuffer.get(), 0, VK_INDEX_TYPE_UINT32);
		cb.drawIndexed(m_indicesCount);
	}
	else
	{
		cb.draw(m_verticesCount);
	}
}

VertexInputState StandardMesh::vertexInputState()
{
	VertexInputState res;

	VkVertexInputBindingDescription binding = {};
	binding.binding = 0;
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	binding.stride = sizeof(Vertex);

	res.bindings.push_back(binding);

	VkVertexInputAttributeDescription positionAttribute = {};
	positionAttribute.binding = 0;
	positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	positionAttribute.location = 0;
	positionAttribute.offset = 0;

	VkVertexInputAttributeDescription normalAttribute = {};
	normalAttribute.binding = 0;
	normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	normalAttribute.location = 1;
	normalAttribute.offset = sizeof(vector3);

	VkVertexInputAttributeDescription uvAttribute = {};
	uvAttribute.binding = 0;
	uvAttribute.format = VK_FORMAT_R32G32_SFLOAT;
	uvAttribute.location = 2;
	uvAttribute.offset = sizeof(vector3) + sizeof(vector3);

	VkVertexInputAttributeDescription tangentAttribute = {};
	tangentAttribute.binding = 0;
	tangentAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	tangentAttribute.location = 3;
	tangentAttribute.offset =
	    sizeof(vector3) + sizeof(vector3) + sizeof(vector2);

	res.attributes.push_back(positionAttribute);
	res.attributes.push_back(normalAttribute);
	res.attributes.push_back(uvAttribute);
	res.attributes.push_back(tangentAttribute);

	res.vertexInputInfo.vertexBindingDescriptionCount =
	    uint32_t(res.bindings.size());
	res.vertexInputInfo.pVertexBindingDescriptions = res.bindings.data();
	res.vertexInputInfo.vertexAttributeDescriptionCount =
	    uint32_t(res.attributes.size());
	res.vertexInputInfo.pVertexAttributeDescriptions = res.attributes.data();

	return res;
}

VertexInputState StandardMesh::positionOnlyVertexInputState()
{
	VertexInputState res;

	VkVertexInputBindingDescription binding = {};
	binding.binding = 0;
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	binding.stride = sizeof(vector4);

	res.bindings.push_back(binding);

	VkVertexInputAttributeDescription positionAttribute = {};
	positionAttribute.binding = 0;
	positionAttribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	positionAttribute.location = 0;
	positionAttribute.offset = 0;

	res.attributes.push_back(positionAttribute);

	res.vertexInputInfo.vertexBindingDescriptionCount =
	    uint32_t(res.bindings.size());
	res.vertexInputInfo.pVertexBindingDescriptions = res.bindings.data();
	res.vertexInputInfo.vertexAttributeDescriptionCount =
	    uint32_t(res.attributes.size());
	res.vertexInputInfo.pVertexAttributeDescriptions = res.attributes.data();

	return res;
}

StandardMesh::ShaderVertexInput StandardMesh::shaderVertexInput(
    sdw::VertexWriter& writer)
{
	using namespace sdw;

	return {
		writer.declInput<Vec3>("inPosition", 0),
		writer.declInput<Vec3>("inNormal", 1),
		writer.declInput<Vec2>("inUV", 2),
		writer.declInput<Vec3>("inTangent", 3),
	};
}
}  // namespace cdm
