#include "Texture.hpp"

#include "CommandBuffer.hpp"
#include "RenderWindow.hpp"

namespace cdm
{
Texture2D::Texture2D(RenderWindow& renderWindow, uint32_t imageWidth,
                     uint32_t imageHeight, VkFormat imageFormat,
                     VkImageTiling imageTiling, VkBufferUsageFlags usage,
                     VmaMemoryUsage memoryUsage,
                     VkMemoryPropertyFlags requiredFlags)
    : rw(&renderWindow)
{
	auto& vk = rw.get()->device();

	vk::ImageCreateInfo info;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.extent.width = imageWidth;
	info.extent.height = imageHeight;
	info.extent.depth = 1;
	info.mipLevels = 1;
	info.arrayLayers = 1;
	info.format = imageFormat;
	info.tiling = imageTiling;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.usage = usage;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.flags = 0;

	VmaAllocationCreateInfo imageAllocCreateInfo = {};
	imageAllocCreateInfo.usage = memoryUsage;
	imageAllocCreateInfo.requiredFlags = requiredFlags;

	VmaAllocationInfo allocInfo{};

	vmaCreateImage(vk.allocator(), &info, &imageAllocCreateInfo,
	               &m_image.get(), &m_allocation.get(), &allocInfo);

	if (m_image == false)
		throw std::runtime_error("could not create image");

	m_deviceMemory = allocInfo.deviceMemory;
	m_offset = allocInfo.offset;
	m_size = allocInfo.size;

	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = m_image.get();
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = imageFormat;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	m_imageView = vk.create(viewInfo);
	if (!m_imageView)
		throw std::runtime_error("could not create image view");
}

Texture2D::Texture2D(Texture2D&& texture) noexcept
{
	rw = std::exchange(texture.rw, nullptr);

	m_allocation = std::exchange(texture.m_allocation, nullptr);
	m_image = std::exchange(texture.m_image, nullptr);
	m_imageView = std::move(texture.m_imageView);

	m_deviceMemory = std::exchange(texture.m_deviceMemory, nullptr);
	m_offset = std::exchange(texture.m_offset, 0);
	m_size = std::exchange(texture.m_size, 0);

	m_width = std::exchange(texture.m_width, 0);
	m_height = std::exchange(texture.m_height, 0);
	m_format = std::exchange(texture.m_format, VK_FORMAT_UNDEFINED);
}

Texture2D::~Texture2D()
{
	if (rw.get())
	{
		auto& vk = rw.get()->device();

		if (m_imageView)
			m_imageView.reset();
		if (m_image)
			vmaDestroyImage(vk.allocator(), m_image.get(), m_allocation.get());
	}
}

Texture2D& Texture2D::operator=(Texture2D&& texture) noexcept
{
	rw = std::exchange(texture.rw, nullptr);

	m_allocation = std::exchange(texture.m_allocation, nullptr);
	m_image = std::exchange(texture.m_image, nullptr);
	m_imageView = std::move(texture.m_imageView);

	m_deviceMemory = std::exchange(texture.m_deviceMemory, nullptr);
	m_offset = std::exchange(texture.m_offset, 0);
	m_size = std::exchange(texture.m_size, 0);

	m_width = std::exchange(texture.m_width, 0);
	m_height = std::exchange(texture.m_height, 0);
	m_format = std::exchange(texture.m_format, VK_FORMAT_UNDEFINED);

	return *this;
}

void Texture2D::transitionLayoutImediate(VkImageLayout oldLayout,
                                         VkImageLayout newLayout)
{
	if (rw.get() == nullptr)
		return;

	auto& vk = rw.get()->device();

	CommandBuffer transitionCB(vk, rw.get()->commandPool());

	transitionCB.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image();
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;
	transitionCB.pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
	                             VK_DEPENDENCY_DEVICE_GROUP_BIT, barrier);
	if (transitionCB.end() != VK_SUCCESS)
		throw std::runtime_error("failed to record transition command buffer");

	if (vk.queueSubmit(vk.graphicsQueue(), transitionCB.get()) != VK_SUCCESS)
		throw std::runtime_error("failed to submit transition command buffer");

	vk.wait(vk.graphicsQueue());
}
}  // namespace cdm
