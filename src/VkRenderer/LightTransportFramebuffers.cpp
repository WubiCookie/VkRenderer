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
void LightTransport::createFramebuffers()
{
	auto& vk = rw.get().device();

#pragma region framebuffer
	{
		vk::FramebufferCreateInfo framebufferInfo;
		framebufferInfo.renderPass = m_renderPass.get();
		std::array attachments = {
			m_outputImageHDR.view()  //, m_outputImageHDR.view()
		};
		framebufferInfo.attachmentCount = uint32_t(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = width;
		framebufferInfo.height = height;
		framebufferInfo.layers = 1;

		m_framebuffer = vk.create(framebufferInfo);
		if (!m_framebuffer)
		{
			std::cerr << "error: failed to create framebuffer" << std::endl;
			abort();
		}
	}
#pragma endregion

#pragma region blit framebuffer
	{
		vk::FramebufferCreateInfo framebufferInfo;
		framebufferInfo.renderPass = m_blitRenderPass.get();
		std::array attachments = {
			m_outputImage.view()  //, m_outputImageHDR.view()
		};
		framebufferInfo.attachmentCount = uint32_t(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = width;
		framebufferInfo.height = height;
		framebufferInfo.layers = 1;

		m_blitFramebuffer = vk.create(framebufferInfo);
		if (!m_blitFramebuffer)
		{
			std::cerr << "error: failed to create blit framebuffer"
			          << std::endl;
			abort();
		}
	}
#pragma endregion
}
}  // namespace cdm
