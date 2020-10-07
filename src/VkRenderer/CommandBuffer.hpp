#pragma once

#include "VulkanDevice.hpp"

#include "cdm_vulkan.hpp"

#include <array>

namespace cdm
{
class CommandPool;
class UniqueComputePipeline;
class UniqueGraphicsPipeline;

class CommandBuffer final : public VulkanDeviceObject
{
	Movable<VkCommandPool> m_parentCommandPool;
	Movable<VkCommandBuffer> m_commandBuffer;

public:
	CommandBuffer(
	    const VulkanDevice& device, VkCommandPool parentCommandPool,
	    VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	CommandBuffer(
	    const CommandPool& parentCommandPool,
	    VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	CommandBuffer(const CommandBuffer&) = delete;
	CommandBuffer(CommandBuffer&& cb) = default;
	~CommandBuffer();

	CommandBuffer& operator=(const CommandBuffer&) = delete;
	CommandBuffer& operator=(CommandBuffer&& cb) = default;

	VkCommandBuffer& get();
	const VkCommandBuffer& get() const;
	VkCommandBuffer& commandBuffer();
	const VkCommandBuffer& commandBuffer() const;
	operator VkCommandBuffer&() { return commandBuffer(); }
	operator const VkCommandBuffer&() const { return commandBuffer(); }

	// clang-format off
	VkResult begin();
	VkResult begin(const cdm::vk::CommandBufferBeginInfo& beginInfo);
	VkResult begin(VkCommandBufferUsageFlags usage);
	CommandBuffer& beginQuery(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags);
	CommandBuffer& beginRenderPass(const cdm::vk::RenderPassBeginInfo& renderPassInfo, VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);
	CommandBuffer& beginRenderPass2(const cdm::vk::RenderPassBeginInfo& renderPassInfo, const cdm::vk::SubpassBeginInfo& subpassInfo);
	CommandBuffer& bindDescriptorSets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount = 0, const uint32_t* pDynamicOffsets = nullptr);
	CommandBuffer& bindDescriptorSet(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, VkDescriptorSet descriptorSet);
	CommandBuffer& bindDescriptorSet(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, VkDescriptorSet descriptorSet, uint32_t dynamicOffset);
	CommandBuffer& bindIndexBuffer(VkBuffer buffer, VkDeviceSize offset = 0, VkIndexType indexType = VK_INDEX_TYPE_UINT32);
	CommandBuffer& bindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);
	CommandBuffer& bindPipeline(const UniqueComputePipeline& pipeline);
	CommandBuffer& bindPipeline(const UniqueGraphicsPipeline& pipeline);
	CommandBuffer& bindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets);
	CommandBuffer& bindVertexBuffer(uint32_t firstBinding, VkBuffer buffer, VkDeviceSize offset = 0);
	CommandBuffer& bindVertexBuffer(VkBuffer buffer);
	CommandBuffer& blitImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter);
	CommandBuffer& blitImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, const VkImageBlit& region, VkFilter filter);
	CommandBuffer& clearAttachments(uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects);
	CommandBuffer& clearAttachments(const VkClearAttachment& attachment, uint32_t rectCount, const VkClearRect* pRects);
	CommandBuffer& clearAttachments(uint32_t attachmentCount, const VkClearAttachment* pAttachments, const VkClearRect& rect);
	CommandBuffer& clearAttachments(const VkClearAttachment& attachment, const VkClearRect& rect);
	CommandBuffer& clearColorImage(VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
	CommandBuffer& clearColorImage(VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, const VkImageSubresourceRange& range);
	CommandBuffer& clearDepthStencilImage(VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
	CommandBuffer& clearDepthStencilImage(VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, const VkImageSubresourceRange& range);
	CommandBuffer& copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions);
	CommandBuffer& copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, const VkBufferCopy& region);
	CommandBuffer& copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);
	CommandBuffer& copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions);
	CommandBuffer& copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, const VkBufferImageCopy& region);
	CommandBuffer& copyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions);
	CommandBuffer& copyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, const VkImageCopy& region);
	CommandBuffer& copyImageToBuffer(VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions);
	CommandBuffer& copyImageToBuffer(VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, const VkBufferImageCopy& region);
	CommandBuffer& copyQueryPoolResults(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags);
	CommandBuffer& debugMarkerBegin(const vk::DebugMarkerMarkerInfoEXT& markerInfo);
	CommandBuffer& debugMarkerBegin(std::string_view markerName, std::array<float, 4> color);
	CommandBuffer& debugMarkerBegin(std::string_view markerName, float colorR = 1.0f, float colorG = 1.0f, float colorB = 1.0f, float colorA = 1.0f);
	CommandBuffer& debugMarkerEnd();
	CommandBuffer& debugMarkerInsert(const vk::DebugMarkerMarkerInfoEXT& markerInfo);
	CommandBuffer& debugMarkerInsert(std::string_view markerName, std::array<float, 4> color);
	CommandBuffer& debugMarkerInsert(std::string_view markerName, float colorR = 1.0f, float colorG = 1.0f, float colorB = 1.0f, float colorA = 1.0f);
	CommandBuffer& dispatch(uint32_t groupCountX = 1, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);
	CommandBuffer& dispatchBase(uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX = 1, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);
	CommandBuffer& dispatchIndirect(VkBuffer buffer, VkDeviceSize offset);
	CommandBuffer& draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0);
	CommandBuffer& drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0);
	CommandBuffer& drawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
	CommandBuffer& drawIndexedIndirectCount(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride);
	CommandBuffer& drawIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
	CommandBuffer& drawIndirectCount(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride);
	CommandBuffer& endQuery(VkQueryPool queryPool, uint32_t query);
	CommandBuffer& endRenderPass();
	CommandBuffer& endRenderPass2(const cdm::vk::SubpassEndInfo& subpassEndInfo);
	CommandBuffer& executeCommands(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers);
	CommandBuffer& executeCommand(VkCommandBuffer commandBuffer);
	CommandBuffer& fillBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data);
	CommandBuffer& nextSubpass(VkSubpassContents contents);
	CommandBuffer& nextSubpass2(const cdm::vk::SubpassBeginInfo& subpassBeginInfo, const cdm::vk::SubpassEndInfo& subpassEndInfo);
	CommandBuffer& pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
	CommandBuffer& pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers);
	CommandBuffer& pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, const VkMemoryBarrier& memoryBarrier);
	CommandBuffer& pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers);
	CommandBuffer& pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, const VkBufferMemoryBarrier& bufferMemoryBarrier);
	CommandBuffer& pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
	CommandBuffer& pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, const VkImageMemoryBarrier& imageMemoryBarrier);
	CommandBuffer& pushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues);
	template<typename T>
	CommandBuffer& pushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, const T* pValues);
	CommandBuffer& resetEvent(VkEvent event, VkPipelineStageFlags stageMask);
	CommandBuffer& resetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount);
	CommandBuffer& resolveImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions);
	CommandBuffer& resolveImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, const VkImageResolve& region);
	CommandBuffer& setBlendConstants(const float blendConstants[4]);
	CommandBuffer& setBlendConstants(float blendConstants0, float blendConstants1, float blendConstants2, float blendConstants3);
	CommandBuffer& setDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor);
	CommandBuffer& setDepthBounds(float minDepthBounds, float maxDepthBounds);
	CommandBuffer& setDeviceMask(uint32_t deviceMask);
	CommandBuffer& setEvent(VkEvent event, VkPipelineStageFlags stageMask);
	CommandBuffer& setLineWidth(float lineWidth);
	CommandBuffer& setScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors);
	CommandBuffer& setScissor(uint32_t firstScissor, const VkRect2D& scissor);
	CommandBuffer& setScissor(const VkRect2D& scissor);
	CommandBuffer& setStencilCompareMask(VkStencilFaceFlags faceMask, uint32_t compareMask);
	CommandBuffer& setStencilReference(VkStencilFaceFlags faceMask, uint32_t reference);
	CommandBuffer& setStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t writeMask);
	CommandBuffer& setViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports);
	CommandBuffer& setViewport(uint32_t firstViewport, const VkViewport& viewport);
	CommandBuffer& setViewport(const VkViewport& viewport);
	CommandBuffer& updateBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData);
	CommandBuffer& waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
	CommandBuffer& waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers);
	CommandBuffer& waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkMemoryBarrier& memoryBarrier);
	CommandBuffer& waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers);
	CommandBuffer& waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkBufferMemoryBarrier& bufferMemoryBarrier);
	CommandBuffer& waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
	CommandBuffer& waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkImageMemoryBarrier& imageMemoryBarrier);
	CommandBuffer& waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers);
	CommandBuffer& waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkMemoryBarrier& memoryBarrier);
	CommandBuffer& waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers);
	CommandBuffer& waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkBufferMemoryBarrier& bufferMemoryBarrier);
	CommandBuffer& waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
	CommandBuffer& waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkImageMemoryBarrier& imageMemoryBarrier);
	CommandBuffer& writeTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query);
	VkResult end();
	VkResult reset(VkCommandBufferResetFlags flags = VkCommandBufferResetFlags());
	// clang-format on

private:
	bool outdated() const;
	void recreate();
};

struct FrameCommandBuffer
{
	CommandBuffer commandBuffer;
	UniqueFence fence;
	UniqueSemaphore semaphore;
	bool submitted = false;

	void reset();
	bool isAvailable();
};
}  // namespace cdm

#include "CommandBuffer.inl"
