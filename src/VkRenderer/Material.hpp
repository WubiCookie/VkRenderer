#pragma once

#include "VulkanDevice.hpp"

#define VK_NO_PROTOTYPES
#include "cdm_vulkan.hpp"

#ifdef USE_SHADERC
#include <shaderc/shaderc.hpp>
#endif

#include <vector>

namespace cdm
{
class RenderWindow;
class RenderPass;
class CommandBuffer;

class Material final : public VulkanDeviceObject
{
	std::reference_wrapper<RenderWindow> rw;

	std::vector<uint32_t> m_vertexShaderBytecode;
	std::vector<uint32_t> m_fragmentShaderBytecode;

	std::string m_vertexShaderGLSL;
	std::string m_fragmentShaderGLSL;

	Moveable<VkShaderModule> m_vertexModule;
	Moveable<VkShaderModule> m_fragmentModule;

	Moveable<VkPipelineLayout> m_pipelineLayout;
	Moveable<VkPipeline> m_pipeline;

	std::reference_wrapper<RenderPass> m_renderPass;

#ifdef USE_SHADERC
	static inline shaderc::Compiler m_compiler;
#endif

public:
	Material(RenderWindow& rw, RenderPass& renderPass);
	~Material();

	bool setVertexShaderBytecode(std::vector<uint32_t> bytecode);
	bool setFragmentShaderBytecode(std::vector<uint32_t> bytecode);

	bool setVertexShaderGLSL(std::string code);
	bool setFragmentShaderGLSL(std::string code);

	const std::vector<uint32_t>& vertexShaderBytecode() const;
	const std::vector<uint32_t>& fragmentShaderBytecode() const;

	std::string vertexShaderGLSL() const;
	std::string fragmentShaderGLSL() const;

	void setRenderPass(RenderPass& renderPass);
	bool buildPipeline();

	void bind(CommandBuffer& commandBuffer);

	VkPipelineLayout pipelineLayout() const;
	VkPipeline pipeline();

private:
	bool outdated() const;
	void recreate();
};
}  // namespace cdm
