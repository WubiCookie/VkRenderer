#ifndef VKRENDERER_DEBUGBOX_HPP
#define VKRENDERER_DEBUGBOX_HPP 1

#include "VulkanDevice.hpp"
#include "cdm_maths.hpp"

namespace cdm
{
class Camera;
class CommandBuffer;

class DebugBox
{
	std::reference_wrapper<const VulkanDevice> m_vk;

	UniquePipelineLayout m_pipelineLayout;
	UniqueGraphicsPipeline m_pipeline;

public:
	DebugBox(const VulkanDevice& device, VkRenderPass rp,
	         const vk::Viewport& viewport, const vk::Rect2D& scissor);
	DebugBox(const DebugBox&) = delete;
	DebugBox(DebugBox&&) = default;
	~DebugBox() = default;

	DebugBox& operator=(const DebugBox&) = delete;
	DebugBox& operator=(DebugBox&&) = default;

	void draw(CommandBuffer& cb, vector3 min, vector3 max, vector3 color,
	          const Camera& camera);
};
}  // namespace cdm

#endif  // VKRENDERER_DEBUGBOX_HPP
