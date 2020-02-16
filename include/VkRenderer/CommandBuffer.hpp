#pragma once

#include "VulkanDevice.hpp"

#include <array>

class CommandBuffer final : public VulkanDeviceObject
{
	cdm::Moveable<VkCommandPool> m_parentCommandPool;
	cdm::Moveable<VkCommandBuffer> m_commandBuffer;

public:
	CommandBuffer(
	    const VulkanDevice& device, VkCommandPool parentCommanPool,
	    VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	CommandBuffer(const CommandBuffer&) = delete;
	CommandBuffer(CommandBuffer&& cb) noexcept;
	~CommandBuffer();

	CommandBuffer& operator=(const CommandBuffer&) = delete;
	CommandBuffer& operator=(CommandBuffer&& cb) noexcept;

	VkCommandBuffer commandBuffer();
	// const VkCommandBuffer& commandBuffer();
	operator VkCommandBuffer() { return commandBuffer(); }

	// clang-format off
	inline VkResult begin(const cdm::vk::CommandBufferBeginInfo& beginInfo);
	inline void beginQuery(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags);
	inline void beginRenderPass(const cdm::vk::RenderPassBeginInfo& renderPassInfo, VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);
	inline void beginRenderPass2(const cdm::vk::RenderPassBeginInfo & renderPassInfo, const cdm::vk::SubpassBeginInfo& subpassInfo);
	inline void bindDescriptorSets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount = 0, const uint32_t* pDynamicOffsets = nullptr);
	inline void bindDescriptorSet(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, VkDescriptorSet descriptorSet, uint32_t dynamicOffsetCount = 0, const uint32_t* pDynamicOffsets = nullptr);
	inline void bindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);
	inline void bindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);
	inline void bindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets);
	inline void bindVertexBuffer(uint32_t firstBinding, VkBuffer buffer, VkDeviceSize offset);
	inline void blitImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter);
	inline void blitImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, const VkImageBlit& region, VkFilter filter);
	inline void clearAttachments(uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects);
	inline void clearAttachments(const VkClearAttachment& attachment, uint32_t rectCount, const VkClearRect* pRects);
	inline void clearAttachments(uint32_t attachmentCount, const VkClearAttachment* pAttachments, const VkClearRect& rect);
	inline void clearAttachments(const VkClearAttachment& attachment, const VkClearRect& rect);
	inline void clearColorImage(VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
	inline void clearColorImage(VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, const VkImageSubresourceRange& range);
	inline void clearDepthStencilImage(VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
	inline void clearDepthStencilImage(VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, const VkImageSubresourceRange& range);
	inline void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions);
	inline void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, const VkBufferCopy& region);
	inline void copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions);
	inline void copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, const VkBufferImageCopy& region);
	inline void copyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions);
	inline void copyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, const VkImageCopy& region);
	inline void copyImageToBuffer(VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions);
	inline void copyImageToBuffer(VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, const VkBufferImageCopy& region);
	inline void copyQueryPoolResults(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags);
	inline void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
	inline void dispatchBase(uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
	inline void dispatchIndirect(VkBuffer buffer, VkDeviceSize offset);
	inline void draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0);
	inline void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0);
	inline void drawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
	inline void drawIndexedIndirectCount(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride);
	inline void drawIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
	inline void drawIndirectCount(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride);
	inline void endQuery(VkQueryPool queryPool, uint32_t query);
	inline void endRenderPass();
	inline void endRenderPass2(const cdm::vk::SubpassEndInfo& subpassEndInfo);
	inline void executeCommands(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers);
	inline void executeCommand(VkCommandBuffer commandBuffer);
	inline void fillBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data);
	inline void nextSubpass(VkSubpassContents contents);
	inline void nextSubpass2(const cdm::vk::SubpassBeginInfo& subpassBeginInfo, const cdm::vk::SubpassEndInfo& subpassEndInfo);
	inline void pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
	inline void pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers);
	inline void pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, const VkMemoryBarrier& memoryBarrier);
	inline void pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers);
	inline void pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, const VkBufferMemoryBarrier& bufferMemoryBarrier);
	inline void pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
	inline void pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, const VkImageMemoryBarrier& imageMemoryBarrier);
	inline void pushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues);
	template<typename T>
	inline void pushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, const T* pValues);
	inline void resetEvent(VkEvent event, VkPipelineStageFlags stageMask);
	inline void resetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount);
	inline void resolveImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions);
	inline void resolveImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, const VkImageResolve& region);
	inline void setBlendConstants(const float blendConstants[4]);
	inline void setBlendConstants(float blendConstants0, float blendConstants1, float blendConstants2, float blendConstants3);
	inline void setDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor);
	inline void setDepthBounds(float minDepthBounds, float maxDepthBounds);
	inline void setDeviceMask(uint32_t deviceMask);
	inline void setEvent(VkEvent event, VkPipelineStageFlags stageMask);
	inline void setLineWidth(float lineWidth);
	inline void setScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors);
	inline void setScissor(uint32_t firstScissor, const VkRect2D& scissor);
	inline void setStencilCompareMask(VkStencilFaceFlags faceMask, uint32_t compareMask);
	inline void setStencilReference(VkStencilFaceFlags faceMask, uint32_t reference);
	inline void setStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t writeMask);
	inline void setViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports);
	inline void setViewport(uint32_t firstViewport, const VkViewport& viewport);
	inline void updateBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData);
	inline void waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
	inline void waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers);
	inline void waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkMemoryBarrier& memoryBarrier);
	inline void waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers);
	inline void waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkBufferMemoryBarrier& bufferMemoryBarrier);
	inline void waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
	inline void waitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkImageMemoryBarrier& imageMemoryBarrier);
	inline void waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers);
	inline void waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkMemoryBarrier& memoryBarrier);
	inline void waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers);
	inline void waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkBufferMemoryBarrier& bufferMemoryBarrier);
	inline void waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
	inline void waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkImageMemoryBarrier& imageMemoryBarrier);
	inline void writeTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query);
	inline VkResult end();
	inline VkResult reset(VkCommandBufferResetFlags flags);
	// clang-format on

private:
	bool outdated() const;
	void recreate();
};

#include "CommandBuffer.inl"
