#pragma once

#include "VulkanDevice.hpp"

#define VK_NO_PROTOTYPES
#include "cdm_vulkan.hpp"

#include <vector>

class RenderWindow;
class RenderPass;

class Material final : public VulkanDeviceObject
{
	std::reference_wrapper<RenderWindow> rw;

	std::vector<uint32_t> m_vertexShaderBytecode;
	std::vector<uint32_t> m_fragmentShaderBytecode;

	cdm::Moveable<VkShaderModule> m_vertexModule;
	cdm::Moveable<VkShaderModule> m_fragmentModule;

	cdm::Moveable<VkPipelineLayout> m_pipelineLayout;
	cdm::Moveable<VkPipeline> m_pipeline;

	std::reference_wrapper<RenderPass> m_renderPass;

public:
	Material(RenderWindow& rw, RenderPass& renderPass);
	~Material();

	bool setVertexShaderBytecode(std::vector<uint32_t> bytecode);
	bool setFragmentShaderBytecode(std::vector<uint32_t> bytecode);

	const std::vector<uint32_t>& vertexShaderBytecode() const;
	const std::vector<uint32_t>& fragmentShaderBytecode() const;

	void setRenderPass(RenderPass& renderPass);
	bool buildPipeline();

	VkPipelineLayout pipelineLayout() const;
	VkPipeline pipeline();

private:
	bool outdated() const;
	void recreate();
};
