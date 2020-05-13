#pragma once

#include "VulkanDevice.hpp"

#include <array>

namespace cdm
{
class CommandBuffer final : public VulkanDeviceObject
{
	Moveable<VkCommandPool> m_parentCommandPool;
	Moveable<VkCommandBuffer> m_commandBuffer;

public:
	CommandBuffer(
	    const VulkanDevice& device, VkCommandPool parentCommanPool,
	    VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	CommandBuffer(const CommandBuffer&) = delete;
	CommandBuffer(CommandBuffer&& cb) noexcept;
	~CommandBuffer();

	CommandBuffer& operator=(const CommandBuffer&) = delete;
	CommandBuffer& operator=(CommandBuffer&& cb) noexcept;

	VkCommandBuffer& get();
	VkCommandBuffer& commandBuffer();
	// const VkCommandBuffer& commandBuffer();
	operator VkCommandBuffer() { return commandBuffer(); }

	// clang-format off
	VkResult begin();
	VkResult begin(const cdm::vk::CommandBufferBeginInfo& beginInfo);
	VkResult begin(VkCommandBufferUsageFlags usage);
	void beginQuery(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags);
	void beginRenderPass(const cdm::vk::RenderPassBeginInfo& renderPassInfo, VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);
	void beginRenderPass2(const cdm::vk::RenderPassBeginInfo& renderPassInfo, const cdm::vk::SubpassBeginInfo& subpassInfo);
	void bindDescriptorSets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount = 0, const uint32_t* pDynamicOffsets = nullptr);
	void bindDescriptorSet(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, VkDescriptorSet descriptorSet, uint32_t dynamicOffsetCount = 0, const uint32_t* pDynamicOffsets = nullptr);
	void bindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);
	void bindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);
	void bindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets);
	void bindVertexBuffer(uint32_t firstBinding, VkBuffer buffer, VkDeviceSize offset = 0);
	void bindVertexBuffer(VkBuffer buffer);
	void blitImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter);
	void blitImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, const VkImageBlit& region, VkFilter filter);
	void clearAttachments(uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects);
	void clearAttachments(const VkClearAttachment& attachment, uint32_t rectCount, const VkClearRect* pRects);
	void clearAttachments(uint32_t attachmentCount, const VkClearAttachment* pAttachments, const VkClearRect& rect);
	void clearAttachments(const VkClearAttachment& attachment, const VkClearRect& rect);
	void clearColorImage(VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
	void clearColorImage(VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, const VkImageSubresourceRange& range);
	void clearDepthStencilImage(VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
	void clearDepthStencilImage(VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, const VkImageSubresourceRange& range);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, const VkBufferCopy& region);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);
	void copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions);
	void copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, const VkBufferImageCopy& region);
	void copyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions);
	void copyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, const VkImageCopy& region);
	void copyImageToBuffer(VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions);
	void copyImageToBuffer(VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, const VkBufferImageCopy& region);
	void copyQueryPoolResults(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags);
	void debugMarkerBegin(const vk::DebugMarkerMarkerInfoEXT& markerInfo);
	void debugMarkerBegin(std::string_view markerName, std::array<float, 4> color);
	void debugMarkerBegin(std::string_view markerName, float colorR = 1.0f, float colorG = 1.0f, float colorB = 1.0f, float colorA = 1.0f);
	void debugMarkerEnd();
	void debugMarkerInsert(const vk::DebugMarkerMarkerInfoEXT& markerInfo);
	void debugMarkerInsert(std::string_view markerName, std::array<float, 4> color);
	void debugMarkerInsert(std::string_view markerName, float colorR = 1.0f, float colorG = 1.0f, float colorB = 1.0f, float colorA = 1.0f);
	void dispatch(uint32_t groupCountX = 1, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);
	void dispatchBase(uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX = 1, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);
	void dispatchIndirect(VkBuffer buffer, VkDeviceSize offset);
	void draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0);
	void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0);
	void drawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
	void drawIndexedIndirectCount(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride);
	void drawIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
	void drawIndirectCount(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride);
	void endQuery(VkQueryPool queryPool, uint32_t query);
	void endRenderPass();
	void endRenderPass2(const cdm::vk::SubpassEndInfo& subpassEndInfo);
	void executeCommands(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers);
	void executeCommand(VkCommandBuffer commandBuffer);
	void fillBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data);
	void nextSubpass(VkSubpassContents contents);
	void nextSubpass2(const cdm::vk::SubpassBeginInfo& subpassBeginInfo, const cdm::vk::SubpassEndInfo& subpassEndInfo);
	void pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
	void pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers);
	void pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, const VkMemoryBarrier& memoryBarrier);
	void pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers);
	void pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, const VkBufferMemoryBarrier& bufferMemoryBarrier);
	void pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
	void pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, const VkImageMemoryBarrier& imageMemoryBarrier);
	void pushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues);
	template<typename T>
	void pushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, const T* pValues);
	void resetEvent(VkEvent event, VkPipelineStageFlags stageMask);
	void resetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount);
	void resolveImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions);
	void resolveImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, const VkImageResolve& region);
	void setBlendConstants(const float blendConstants[4]);
	void setBlendConstants(float blendConstants0, float blendConstants1, float blendConstants2, float blendConstants3);
	void setDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor);
	void setDepthBounds(float minDepthBounds, float maxDepthBounds);
	void setDeviceMask(uint32_t deviceMask);
	void setEvent(VkEvent event, VkPipelineStageFlags stageMask);
	void setLineWidth(float lineWidth);
	void setScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors);
	void setScissor(uint32_t firstScissor, const VkRect2D& scissor);
	void setStencilCompareMask(VkStencilFaceFlags faceMask, uint32_t compareMask);
	void setStencilReference(VkStencilFaceFlags faceMask, uint32_t reference);
	void setStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t writeMask);
	void setViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports);
	void setViewport(uint32_t firstViewport, const VkViewport& viewport);
	void updateBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData);
	void waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
	void waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers);
	void waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkMemoryBarrier& memoryBarrier);
	void waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers);
	void waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkBufferMemoryBarrier& bufferMemoryBarrier);
	void waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
	void waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkImageMemoryBarrier& imageMemoryBarrier);
	void waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers);
	void waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkMemoryBarrier& memoryBarrier);
	void waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers);
	void waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkBufferMemoryBarrier& bufferMemoryBarrier);
	void waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
	void waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkImageMemoryBarrier& imageMemoryBarrier);
	void writeTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query);
	VkResult end();
	VkResult reset(VkCommandBufferResetFlags flags = VkCommandBufferResetFlags());
	// clang-format on

private:
	bool outdated() const;
	void recreate();
};
}  // namespace cdm

#include "CommandBuffer.inl"
