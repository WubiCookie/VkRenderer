#include "Framebuffer.hpp"
#include "ImageView.hpp"
#include "RenderPass.hpp"
#include "RenderWindow.hpp"

#include <stdexcept>

namespace cdm
{
Framebuffer::Framebuffer(
    RenderWindow& renderWindow, RenderPass& renderPass,
    std::vector<std::reference_wrapper<ImageView>> imageViews)
    : VulkanDeviceObject(renderWindow.device()),
      rw(renderWindow),
      m_renderPass(renderPass),
      m_imageViews(std::move(imageViews))
{
}

Framebuffer::~Framebuffer()
{
	const auto& vk = device();

	vk.destroy(m_framebuffer.get());
}

VkFramebuffer Framebuffer::framebuffer()
{
	if (outdated())
		recreate();

	return m_framebuffer.get();
}

Framebuffer::operator VkFramebuffer() { return framebuffer(); }

bool Framebuffer::outdated() const
{
	if (m_renderPass.get().creationTime() > creationTime())
		return true;

	for (const ImageView& iv : m_imageViews)
		if (iv.creationTime() > creationTime())
			return true;

	return false;
}

void Framebuffer::recreate()
{
	auto& vk = device();
	auto& rw = this->rw.get();

	vk.destroy(m_framebuffer.get());

	int width, height;
	rw.framebufferSize(width, height);

	std::vector<VkImageView> vkImageViews;
	vkImageViews.reserve(m_imageViews.size());
	for (ImageView& i : m_imageViews)
	{
		vkImageViews.push_back(i);
	}

	vk::FramebufferCreateInfo framebufferInfo;
	framebufferInfo.renderPass = m_renderPass.get().renderPass();
	framebufferInfo.attachmentCount = uint32_t(vkImageViews.size());
	framebufferInfo.pAttachments = vkImageViews.data();
	framebufferInfo.width = width;
	framebufferInfo.height = height;
	framebufferInfo.layers = 1;

	if (vk.create(framebufferInfo, m_framebuffer.get()) != VK_SUCCESS)
	{
		throw std::runtime_error("error: failed to create framebuffer");
	}

	setCreationTime();
}
}  // namespace cdm
