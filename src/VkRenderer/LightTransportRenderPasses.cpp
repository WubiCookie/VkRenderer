#include "LightTransport.hpp"

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
void LightTransport::createRenderPasses()
{
	auto& vk = rw.get().device();

#pragma region renderpass
	{
		vk::AttachmentDescription colorAttachment;
		colorAttachment.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		//colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		//colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		//colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

		// VkAttachmentDescription colorHDRAttachment = {};
		// colorHDRAttachment.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		// colorHDRAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		// colorHDRAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		// colorHDRAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		// colorHDRAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		// colorHDRAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		// colorHDRAttachment.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		// colorHDRAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		// std::array colorAttachments{ colorAttachment, colorHDRAttachment };
		std::array colorAttachments{ colorAttachment };

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// VkAttachmentReference colorHDRAttachmentRef = {};
		// colorHDRAttachmentRef.attachment = 1;
		// colorHDRAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		std::array colorAttachmentRefs{

			colorAttachmentRef  //, colorHDRAttachmentRef
		};

		vk::SubpassDescription subpass;
		//subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = uint32_t(colorAttachmentRefs.size());
		subpass.pColorAttachments = colorAttachmentRefs.data();
		//subpass.inputAttachmentCount = 0;
		//subpass.pInputAttachments = nullptr;
		//subpass.pResolveAttachments = nullptr;
		//subpass.pDepthStencilAttachment = nullptr;
		//subpass.preserveAttachmentCount = 0;
		//subpass.pPreserveAttachments = nullptr;

		VkSubpassDependency dependency = {};
		dependency.dependencyFlags = 0;
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		vk::RenderPassCreateInfo renderPassInfo;
		renderPassInfo.attachmentCount = uint32_t(colorAttachments.size());
		renderPassInfo.pAttachments = colorAttachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		m_renderPass = vk.create(renderPassInfo);
		if (!m_renderPass)
		{
			std::cerr << "error: failed to create render pass" << std::endl;
			abort();
		}
		vk.debugMarkerSetObjectName(m_renderPass.get(), "renderPass");
	}
#pragma endregion
	
#pragma region blit renderpass
	{
		vk::AttachmentDescription colorAttachment;
		colorAttachment.format = VkFormat::VK_FORMAT_B8G8R8A8_UNORM;
		//colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		//colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		//colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		//colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// VkAttachmentDescription colorHDRAttachment = {};
		// colorHDRAttachment.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		// colorHDRAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		// colorHDRAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		// colorHDRAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		// colorHDRAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		// colorHDRAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		// colorHDRAttachment.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		// colorHDRAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		// std::array colorAttachments{ colorAttachment, colorHDRAttachment };
		std::array colorAttachments{ colorAttachment };

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// VkAttachmentReference colorHDRAttachmentRef = {};
		// colorHDRAttachmentRef.attachment = 1;
		// colorHDRAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		std::array colorAttachmentRefs{

			colorAttachmentRef  //, colorHDRAttachmentRef
		};

		vk::SubpassDescription subpass;
		//subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = uint32_t(colorAttachmentRefs.size());
		subpass.pColorAttachments = colorAttachmentRefs.data();
		//subpass.inputAttachmentCount = 0;
		//subpass.pInputAttachments = nullptr;
		//subpass.pResolveAttachments = nullptr;
		//subpass.pDepthStencilAttachment = nullptr;
		//subpass.preserveAttachmentCount = 0;
		//subpass.pPreserveAttachments = nullptr;

		VkSubpassDependency dependency = {};
		dependency.dependencyFlags = 0;
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		vk::RenderPassCreateInfo renderPassInfo;
		renderPassInfo.attachmentCount = uint32_t(colorAttachments.size());
		renderPassInfo.pAttachments = colorAttachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		m_blitRenderPass = vk.create(renderPassInfo);
		if (!m_blitRenderPass)
		{
			std::cerr << "error: failed to create blit render pass" << std::endl;
			abort();
		}
		vk.debugMarkerSetObjectName(m_blitRenderPass.get(), "blitRenderPass");
	}
#pragma endregion
	
	/*
#pragma region ray renderpass
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

		// VkAttachmentDescription colorHDRAttachment = {};
		// colorHDRAttachment.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		// colorHDRAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		// colorHDRAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		// colorHDRAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		// colorHDRAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		// colorHDRAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		// colorHDRAttachment.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		// colorHDRAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		// std::array colorAttachments{ colorAttachment, colorHDRAttachment };
		std::array colorAttachments{ colorAttachment };

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// VkAttachmentReference colorHDRAttachmentRef = {};
		// colorHDRAttachmentRef.attachment = 1;
		// colorHDRAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		std::array colorAttachmentRefs{

			colorAttachmentRef  //, colorHDRAttachmentRef
		};

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = uint32_t(colorAttachmentRefs.size());
		subpass.pColorAttachments = colorAttachmentRefs.data();
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = nullptr;
		subpass.pResolveAttachments = nullptr;
		subpass.pDepthStencilAttachment = nullptr;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = nullptr;

		VkSubpassDependency dependency = {};
		dependency.dependencyFlags = 0;
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		vk::RenderPassCreateInfo renderPassInfo;
		renderPassInfo.attachmentCount = uint32_t(colorAttachments.size());
		renderPassInfo.pAttachments = colorAttachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		m_rayRenderPass = vk.create(renderPassInfo);
		if (!m_rayRenderPass)
		{
			std::cerr << "error: failed to create ray render pass" << std::endl;
			abort();
		}
	}
#pragma endregion
	//*/
}
}  // namespace cdm
