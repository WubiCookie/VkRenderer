#pragma once

#include "Buffer.hpp"
#include "Camera.hpp"
#include "CommandBuffer.hpp"
#include "DebugBox.hpp"
#include "DepthTexture.hpp"
#include "RenderApplication.hpp"
#include "Texture2D.hpp"
#include "VulkanDevice.hpp"
#include "cdm_maths.hpp"
#include "kd_tree.hpp"
#include "octree.hpp"

#include <optional>
#include <random>
#include <vector>

namespace cdm
{
class OrbitCamera : public Camera
{
	quaternion m_rotation = quaternion::identity();
	float m_distance;
	vector3 m_center;

	void Update();

public:
	void Rotate(radian pitch, radian yaw);
	void AddDistance(float offset);
	void SetCenter(vector3 center);
};

class SpatialPartitionning final : public RenderApplication
{
	struct Vertex
	{
		vector4 position;
		vector3 normal;
	};

	LogRRID log;

	ResettableCommandBufferPool m_cbPool;

	UniqueRenderPass m_renderPass;

	Texture2D m_outputImage;
	DepthTexture m_depthTexture;

	UniqueFramebuffer m_framebuffer;
	UniqueFramebuffer m_imguiFramebuffer;

	UniqueShaderModule m_vertexModule;
	UniqueShaderModule m_fragmentModule;

	UniqueDescriptorPool m_dPool;
	UniqueDescriptorSetLayout m_dSetLayout;
	Movable<VkDescriptorSet> m_dSet;
	UniquePipelineLayout m_pipelineLayout;

	UniquePipeline m_meshPipeline;
	UniquePipeline m_debogBoxPipeline;

	Buffer m_uniformBuffer;
	Buffer m_vertexBuffer;
	Buffer m_indexBuffer;

	Box m_box;
	std::optional<DebugBox> m_debugBox;

	std::random_device rd;
	std::mt19937 gen;
	std::normal_distribution<float> dis;
	std::uniform_real_distribution<float> udis;

	std::vector<vector3> m_positions;
	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;
	uint32_t m_indicesCount = 0;

	bool m_swapchainRecreated = false;

	OrbitCamera m_camera;

public:
	SpatialPartitionning(RenderWindow& renderWindow);
	~SpatialPartitionning();

	void render(CommandBuffer& cb);
	void imgui(CommandBuffer& cb);

	void resize(uint32_t width, uint32_t height) override;
	void update() override;
	void draw() override;

protected:
	void onMouseWheel(double xoffset, double yoffset) override;
};
}  // namespace cdm
