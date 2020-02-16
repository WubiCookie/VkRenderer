#pragma once

#include "CommandBuffer.hpp"

inline VkResult CommandBuffer::begin(
    const cdm::vk::CommandBufferBeginInfo& beginInfo)
{
	return device().BeginCommandBuffer(m_commandBuffer.get(), &beginInfo);
}

inline void CommandBuffer::beginQuery(VkQueryPool queryPool, uint32_t query,
                                      VkQueryControlFlags flags)
{
	device().CmdBeginQuery(m_commandBuffer.get(), queryPool, query, flags);
}

inline void CommandBuffer::beginRenderPass(
    const cdm::vk::RenderPassBeginInfo& renderPassInfo,
    VkSubpassContents contents)
{
	device().CmdBeginRenderPass(m_commandBuffer.get(), &renderPassInfo,
	                            contents);
}

inline void CommandBuffer::beginRenderPass2(
    const cdm::vk::RenderPassBeginInfo& renderPassInfo,
    const cdm::vk::SubpassBeginInfo& subpassInfo)
{
	device().CmdBeginRenderPass2(m_commandBuffer.get(), &renderPassInfo,
	                             &subpassInfo);
}

inline void CommandBuffer::bindDescriptorSets(
    VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout,
    uint32_t firstSet, uint32_t descriptorSetCount,
    const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount,
    const uint32_t* pDynamicOffsets)
{
	device().CmdBindDescriptorSets(m_commandBuffer.get(), pipelineBindPoint,
	                               layout, firstSet, descriptorSetCount,
	                               pDescriptorSets, dynamicOffsetCount,
	                               pDynamicOffsets);
}

inline void CommandBuffer::bindDescriptorSet(
    VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout,
    uint32_t firstSet, VkDescriptorSet descriptorSet,
    uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
{
	bindDescriptorSets(pipelineBindPoint, layout, firstSet, 1, &descriptorSet,
	                   dynamicOffsetCount, pDynamicOffsets);
}

inline void CommandBuffer::bindIndexBuffer(VkBuffer buffer,
                                           VkDeviceSize offset,
                                           VkIndexType indexType)
{
	device().CmdBindIndexBuffer(m_commandBuffer.get(), buffer, offset,
	                            indexType);
}

inline void CommandBuffer::bindPipeline(VkPipelineBindPoint pipelineBindPoint,
                                        VkPipeline pipeline)
{
	device().CmdBindPipeline(m_commandBuffer.get(), pipelineBindPoint,
	                         pipeline);
}

inline void CommandBuffer::bindVertexBuffers(uint32_t firstBinding,
                                             uint32_t bindingCount,
                                             const VkBuffer* pBuffers,
                                             const VkDeviceSize* pOffsets)
{
	device().CmdBindVertexBuffers(m_commandBuffer.get(), firstBinding,
	                              bindingCount, pBuffers, pOffsets);
}

inline void CommandBuffer::bindVertexBuffer(uint32_t firstBinding,
                                            VkBuffer buffer,
                                            VkDeviceSize offset)
{
	bindVertexBuffers(firstBinding, 1, &buffer, &offset);
}

inline void CommandBuffer::blitImage(
    VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
    VkImageLayout dstImageLayout, uint32_t regionCount,
    const VkImageBlit* pRegions, VkFilter filter)
{
	device().CmdBlitImage(m_commandBuffer.get(), srcImage, srcImageLayout,
	                      dstImage, dstImageLayout, regionCount, pRegions,
	                      filter);
}

inline void CommandBuffer::blitImage(
    VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
    VkImageLayout dstImageLayout, const VkImageBlit& region, VkFilter filter)
{
	blitImage(srcImage, srcImageLayout, dstImage, dstImageLayout, 1, &region,
	          filter);
}

inline void CommandBuffer::clearAttachments(
    uint32_t attachmentCount, const VkClearAttachment* pAttachments,
    uint32_t rectCount, const VkClearRect* pRects)
{
	device().CmdClearAttachments(m_commandBuffer.get(), attachmentCount,
	                             pAttachments, rectCount, pRects);
}

inline void CommandBuffer::clearAttachments(
    const VkClearAttachment& attachment, uint32_t rectCount,
    const VkClearRect* pRects)
{
	clearAttachments(1, &attachment, rectCount, pRects);
}

inline void CommandBuffer::clearAttachments(
    uint32_t attachmentCount, const VkClearAttachment* pAttachments,
    const VkClearRect& rect)
{
	clearAttachments(attachmentCount, pAttachments, 1, &rect);
}

inline void CommandBuffer::clearAttachments(
    const VkClearAttachment& attachment, const VkClearRect& rect)
{
	clearAttachments(1, &attachment, 1, &rect);
}

inline void CommandBuffer::clearColorImage(
    VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor,
    uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
	device().CmdClearColorImage(m_commandBuffer.get(), image, imageLayout,
	                            pColor, rangeCount, pRanges);
}

inline void CommandBuffer::clearColorImage(
    VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor,
    const VkImageSubresourceRange& range)
{
	clearColorImage(image, imageLayout, pColor, 1, &range);
}

inline void CommandBuffer::clearDepthStencilImage(
    VkImage image, VkImageLayout imageLayout,
    const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount,
    const VkImageSubresourceRange* pRanges)
{
	device().CmdClearDepthStencilImage(m_commandBuffer.get(), image,
	                                   imageLayout, pDepthStencil, rangeCount,
	                                   pRanges);
}

inline void CommandBuffer::clearDepthStencilImage(
    VkImage image, VkImageLayout imageLayout,
    const VkClearDepthStencilValue* pDepthStencil,
    const VkImageSubresourceRange& range)
{
	clearDepthStencilImage(image, imageLayout, pDepthStencil, 1, &range);
}

inline void CommandBuffer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                                      uint32_t regionCount,
                                      const VkBufferCopy* pRegions)
{
	device().CmdCopyBuffer(m_commandBuffer.get(), srcBuffer, dstBuffer,
	                       regionCount, pRegions);
}

inline void CommandBuffer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                                      const VkBufferCopy& region)
{
	copyBuffer(srcBuffer, dstBuffer, 1, &region);
}

inline void CommandBuffer::copyBufferToImage(VkBuffer srcBuffer,
                                             VkImage dstImage,
                                             VkImageLayout dstImageLayout,
                                             uint32_t regionCount,
                                             const VkBufferImageCopy* pRegions)
{
	device().CmdCopyBufferToImage(m_commandBuffer.get(), srcBuffer, dstImage,
	                              dstImageLayout, regionCount, pRegions);
}

inline void CommandBuffer::copyBufferToImage(VkBuffer srcBuffer,
                                             VkImage dstImage,
                                             VkImageLayout dstImageLayout,
                                             const VkBufferImageCopy& region)
{
	copyBufferToImage(srcBuffer, dstImage, dstImageLayout, 1, &region);
}

inline void CommandBuffer::copyImage(VkImage srcImage,
                                     VkImageLayout srcImageLayout,
                                     VkImage dstImage,
                                     VkImageLayout dstImageLayout,
                                     uint32_t regionCount,
                                     const VkImageCopy* pRegions)
{
	device().CmdCopyImage(m_commandBuffer.get(), srcImage, srcImageLayout,
	                      dstImage, dstImageLayout, regionCount, pRegions);
}

inline void CommandBuffer::copyImage(VkImage srcImage,
                                     VkImageLayout srcImageLayout,
                                     VkImage dstImage,
                                     VkImageLayout dstImageLayout,
                                     const VkImageCopy& region)
{
	copyImage(srcImage, srcImageLayout, dstImage, dstImageLayout, 1, &region);
}

inline void CommandBuffer::copyImageToBuffer(VkImage srcImage,
                                             VkImageLayout srcImageLayout,
                                             VkBuffer dstBuffer,
                                             uint32_t regionCount,
                                             const VkBufferImageCopy* pRegions)
{
	device().CmdCopyImageToBuffer(m_commandBuffer.get(), srcImage,
	                              srcImageLayout, dstBuffer, regionCount,
	                              pRegions);
}

inline void CommandBuffer::copyImageToBuffer(VkImage srcImage,
                                             VkImageLayout srcImageLayout,
                                             VkBuffer dstBuffer,
                                             const VkBufferImageCopy& region)
{
	copyImageToBuffer(srcImage, srcImageLayout, dstBuffer, 1, &region);
}

inline void CommandBuffer::copyQueryPoolResults(
    VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount,
    VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride,
    VkQueryResultFlags flags)
{
	device().CmdCopyQueryPoolResults(m_commandBuffer.get(), queryPool,
	                                 firstQuery, queryCount, dstBuffer,
	                                 dstOffset, stride, flags);
}

inline void CommandBuffer::dispatch(uint32_t groupCountX, uint32_t groupCountY,
                                    uint32_t groupCountZ)
{
	device().CmdDispatch(m_commandBuffer.get(), groupCountX, groupCountY,
	                     groupCountZ);
}

inline void CommandBuffer::dispatchBase(
    uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ,
    uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
	device().CmdDispatchBase(m_commandBuffer.get(), baseGroupX, baseGroupY,
	                         baseGroupZ, groupCountX, groupCountY,
	                         groupCountZ);
}

inline void CommandBuffer::dispatchIndirect(VkBuffer buffer,
                                            VkDeviceSize offset)
{
	device().CmdDispatchIndirect(m_commandBuffer.get(), buffer, offset);
}

inline void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount,
                                uint32_t firstVertex, uint32_t firstInstance)
{
	device().CmdDraw(m_commandBuffer.get(), vertexCount, instanceCount,
	                 firstVertex, firstInstance);
}

inline void CommandBuffer::drawIndexed(uint32_t indexCount,
                                       uint32_t instanceCount,
                                       uint32_t firstIndex,
                                       int32_t vertexOffset,
                                       uint32_t firstInstance)
{
	device().CmdDrawIndexed(m_commandBuffer.get(), indexCount, instanceCount,
	                        firstIndex, vertexOffset, firstInstance);
}

inline void CommandBuffer::drawIndexedIndirect(VkBuffer buffer,
                                               VkDeviceSize offset,
                                               uint32_t drawCount,
                                               uint32_t stride)
{
	device().CmdDrawIndexedIndirect(m_commandBuffer.get(), buffer, offset,
	                                drawCount, stride);
}

inline void CommandBuffer::drawIndexedIndirectCount(
    VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer,
    VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
	device().CmdDrawIndexedIndirectCount(m_commandBuffer.get(), buffer, offset,
	                                     countBuffer, countBufferOffset,
	                                     maxDrawCount, stride);
}

inline void CommandBuffer::drawIndirect(VkBuffer buffer, VkDeviceSize offset,
                                        uint32_t drawCount, uint32_t stride)
{
	device().CmdDrawIndirect(m_commandBuffer.get(), buffer, offset, drawCount,
	                         stride);
}

inline void CommandBuffer::drawIndirectCount(
    VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer,
    VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
	device().CmdDrawIndirectCount(m_commandBuffer.get(), buffer, offset,
	                              countBuffer, countBufferOffset, maxDrawCount,
	                              stride);
}

inline void CommandBuffer::endQuery(VkQueryPool queryPool, uint32_t query)
{
	device().CmdEndQuery(m_commandBuffer.get(), queryPool, query);
}

inline void CommandBuffer::endRenderPass()
{
	device().CmdEndRenderPass(m_commandBuffer.get());
}

inline void CommandBuffer::endRenderPass2(
    const cdm::vk::SubpassEndInfo& subpassEndInfo)
{
	device().CmdEndRenderPass2(m_commandBuffer.get(), &subpassEndInfo);
}

inline void CommandBuffer::executeCommands(
    uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers)
{
	device().CmdExecuteCommands(m_commandBuffer.get(), commandBufferCount,
	                            pCommandBuffers);
}

inline void CommandBuffer::executeCommand(VkCommandBuffer commandBuffer)
{
	executeCommands(1, &commandBuffer);
}

inline void CommandBuffer::fillBuffer(VkBuffer dstBuffer,
                                      VkDeviceSize dstOffset,
                                      VkDeviceSize size, uint32_t data)
{
	device().CmdFillBuffer(m_commandBuffer.get(), dstBuffer, dstOffset, size,
	                       data);
}

inline void CommandBuffer::nextSubpass(VkSubpassContents contents)
{
	device().CmdNextSubpass(m_commandBuffer.get(), contents);
}

inline void CommandBuffer::nextSubpass2(
    const cdm::vk::SubpassBeginInfo& subpassBeginInfo,
    const cdm::vk::SubpassEndInfo& subpassEndInfo)
{
	device().CmdNextSubpass2(m_commandBuffer.get(), &subpassBeginInfo,
	                         &subpassEndInfo);
}

inline void CommandBuffer::pipelineBarrier(
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount,
    const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier* pBufferMemoryBarriers,
    uint32_t imageMemoryBarrierCount,
    const VkImageMemoryBarrier* pImageMemoryBarriers)
{
	device().CmdPipelineBarrier(
	    m_commandBuffer.get(), srcStageMask, dstStageMask, dependencyFlags,
	    memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount,
	    pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

inline void CommandBuffer::pipelineBarrier(
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount,
    const VkMemoryBarrier* pMemoryBarriers)
{
	pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags,
	                memoryBarrierCount, pMemoryBarriers, 0, nullptr, 0,
	                nullptr);
}
inline void CommandBuffer::pipelineBarrier(
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    VkDependencyFlags dependencyFlags, const VkMemoryBarrier& memoryBarrier)
{
	pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, 1,
	                &memoryBarrier);
}
inline void CommandBuffer::pipelineBarrier(
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    VkDependencyFlags dependencyFlags, uint32_t bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier* pBufferMemoryBarriers)
{
	pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, 0, nullptr,
	                bufferMemoryBarrierCount, pBufferMemoryBarriers, 0,
	                nullptr);
}
inline void CommandBuffer::pipelineBarrier(
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    VkDependencyFlags dependencyFlags,
    const VkBufferMemoryBarrier& bufferMemoryBarrier)
{
	pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, 1,
	                &bufferMemoryBarrier);
}
inline void CommandBuffer::pipelineBarrier(
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    VkDependencyFlags dependencyFlags, uint32_t imageMemoryBarrierCount,
    const VkImageMemoryBarrier* pImageMemoryBarriers)
{
	pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, 0, nullptr, 0,
	                nullptr, imageMemoryBarrierCount, pImageMemoryBarriers);
}
inline void CommandBuffer::pipelineBarrier(
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    VkDependencyFlags dependencyFlags,
    const VkImageMemoryBarrier& imageMemoryBarrier)
{
	pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, 1,
	                &imageMemoryBarrier);
}

inline void CommandBuffer::pushConstants(VkPipelineLayout layout,
                                         VkShaderStageFlags stageFlags,
                                         uint32_t offset, uint32_t size,
                                         const void* pValues)
{
	device().CmdPushConstants(m_commandBuffer.get(), layout, stageFlags,
	                          offset, size, pValues);
}

template <typename T>
inline void CommandBuffer::pushConstants(VkPipelineLayout layout,
                                         VkShaderStageFlags stageFlags,
                                         uint32_t offset, const T* pValues)
{
	pushConstants(
	    layout, stageFlags, offset, uint32_t(sizeof(T)),
	    inline reinterpret_cast<const void * CommandBuffer::>(pValues))
}

inline void CommandBuffer::resetEvent(VkEvent event,
                                      VkPipelineStageFlags stageMask)
{
	device().CmdResetEvent(m_commandBuffer.get(), event, stageMask);
}

inline void CommandBuffer::resetQueryPool(VkQueryPool queryPool,
                                          uint32_t firstQuery,
                                          uint32_t queryCount)
{
	device().CmdResetQueryPool(m_commandBuffer.get(), queryPool, firstQuery,
	                           queryCount);
}

inline void CommandBuffer::resolveImage(VkImage srcImage,
                                        VkImageLayout srcImageLayout,
                                        VkImage dstImage,
                                        VkImageLayout dstImageLayout,
                                        uint32_t regionCount,
                                        const VkImageResolve* pRegions)
{
	device().CmdResolveImage(m_commandBuffer.get(), srcImage, srcImageLayout,
	                         dstImage, dstImageLayout, regionCount, pRegions);
}

inline void CommandBuffer::resolveImage(VkImage srcImage,
                                        VkImageLayout srcImageLayout,
                                        VkImage dstImage,
                                        VkImageLayout dstImageLayout,
                                        const VkImageResolve& region)
{
	resolveImage(srcImage, srcImageLayout, dstImage, dstImageLayout, 1,
	             &region);
}

inline void CommandBuffer::setBlendConstants(const float blendConstants[4])
{
	device().CmdSetBlendConstants(m_commandBuffer.get(), blendConstants);
}

inline void CommandBuffer::setBlendConstants(float blendConstants0,
                                             float blendConstants1,
                                             float blendConstants2,
                                             float blendConstants3)
{
	std::array blendConstants = { blendConstants0, blendConstants1,
		                          blendConstants2, blendConstants3 };
	setBlendConstants(blendConstants.data());
}

inline void CommandBuffer::setDepthBias(float depthBiasConstantFactor,
                                        float depthBiasClamp,
                                        float depthBiasSlopeFactor)
{
	device().CmdSetDepthBias(m_commandBuffer.get(), depthBiasConstantFactor,
	                         depthBiasClamp, depthBiasSlopeFactor);
}

inline void CommandBuffer::setDepthBounds(float minDepthBounds,
                                          float maxDepthBounds)
{
	device().CmdSetDepthBounds(m_commandBuffer.get(), minDepthBounds,
	                           maxDepthBounds);
}

inline void CommandBuffer::setDeviceMask(uint32_t deviceMask)
{
	device().CmdSetDeviceMask(m_commandBuffer.get(), deviceMask);
}

inline void CommandBuffer::setEvent(VkEvent event,
                                    VkPipelineStageFlags stageMask)
{
	device().CmdSetEvent(m_commandBuffer.get(), event, stageMask);
}

inline void CommandBuffer::setLineWidth(float lineWidth)
{
	device().CmdSetLineWidth(m_commandBuffer.get(), lineWidth);
}

inline void CommandBuffer::setScissor(uint32_t firstScissor,
                                      uint32_t scissorCount,
                                      const VkRect2D* pScissors)
{
	device().CmdSetScissor(m_commandBuffer.get(), firstScissor, scissorCount,
	                       pScissors);
}

inline void CommandBuffer::setScissor(uint32_t firstScissor,
                                      const VkRect2D& scissor)
{
	setScissor(firstScissor, 1, &scissor);
}

inline void CommandBuffer::setStencilCompareMask(VkStencilFaceFlags faceMask,
                                                 uint32_t compareMask)
{
	device().CmdSetStencilCompareMask(m_commandBuffer.get(), faceMask,
	                                  compareMask);
}

inline void CommandBuffer::setStencilReference(VkStencilFaceFlags faceMask,
                                               uint32_t reference)
{
	device().CmdSetStencilReference(m_commandBuffer.get(), faceMask,
	                                reference);
}

inline void CommandBuffer::setStencilWriteMask(VkStencilFaceFlags faceMask,
                                               uint32_t writeMask)
{
	device().CmdSetStencilWriteMask(m_commandBuffer.get(), faceMask,
	                                writeMask);
}

inline void CommandBuffer::setViewport(uint32_t firstViewport,
                                       uint32_t viewportCount,
                                       const VkViewport* pViewports)
{
	device().CmdSetViewport(m_commandBuffer.get(), firstViewport,
	                        viewportCount, pViewports);
}

inline void CommandBuffer::setViewport(uint32_t firstViewport,
                                       const VkViewport& viewport)
{
	setViewport(firstViewport, 1, &viewport);
}

inline void CommandBuffer::updateBuffer(VkBuffer dstBuffer,
                                        VkDeviceSize dstOffset,
                                        VkDeviceSize dataSize,
                                        const void* pData)
{
	device().CmdUpdateBuffer(m_commandBuffer.get(), dstBuffer, dstOffset,
	                         dataSize, pData);
}

inline void CommandBuffer::waitEvents(
    uint32_t eventCount, const VkEvent* pEvents,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
    uint32_t bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier* pBufferMemoryBarriers,
    uint32_t imageMemoryBarrierCount,
    const VkImageMemoryBarrier* pImageMemoryBarriers)
{
	device().CmdWaitEvents(
	    m_commandBuffer.get(), eventCount, pEvents, srcStageMask, dstStageMask,
	    memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount,
	    pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

inline void CommandBuffer::waitEvents(uint32_t eventCount,
                                      const VkEvent* pEvents,
                                      VkPipelineStageFlags srcStageMask,
                                      VkPipelineStageFlags dstStageMask,
                                      uint32_t memoryBarrierCount,
                                      const VkMemoryBarrier* pMemoryBarriers)
{
	waitEvents(eventCount, pEvents, srcStageMask, dstStageMask,
	           memoryBarrierCount, pMemoryBarriers, 0, nullptr, 0, nullptr);
}
inline void CommandBuffer::waitEvents(uint32_t eventCount,
                                      const VkEvent* pEvents,
                                      VkPipelineStageFlags srcStageMask,
                                      VkPipelineStageFlags dstStageMask,
                                      const VkMemoryBarrier& memoryBarrier)
{
	waitEvents(eventCount, pEvents, srcStageMask, dstStageMask, 1,
	           &memoryBarrier);
}
inline void CommandBuffer::waitEvents(
    uint32_t eventCount, const VkEvent* pEvents,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    uint32_t bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier* pBufferMemoryBarriers)
{
	waitEvents(eventCount, pEvents, srcStageMask, dstStageMask, 0, nullptr,
	           bufferMemoryBarrierCount, pBufferMemoryBarriers, 0, nullptr);
}
inline void CommandBuffer::waitEvents(
    uint32_t eventCount, const VkEvent* pEvents,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    const VkBufferMemoryBarrier& bufferMemoryBarrier)
{
	waitEvents(eventCount, pEvents, srcStageMask, dstStageMask, 1,
	           &bufferMemoryBarrier);
}
inline void CommandBuffer::waitEvents(
    uint32_t eventCount, const VkEvent* pEvents,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    uint32_t imageMemoryBarrierCount,
    const VkImageMemoryBarrier* pImageMemoryBarriers)
{
	waitEvents(eventCount, pEvents, srcStageMask, dstStageMask, 0, nullptr, 0,
	           nullptr, imageMemoryBarrierCount, pImageMemoryBarriers);
}
inline void CommandBuffer::waitEvents(
    uint32_t eventCount, const VkEvent* pEvents,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    const VkImageMemoryBarrier& imageMemoryBarrier)
{
	waitEvents(eventCount, pEvents, srcStageMask, dstStageMask, 1,
	           &imageMemoryBarrier);
}
inline void CommandBuffer::waitEvent(VkEvent event,
                                     VkPipelineStageFlags srcStageMask,
                                     VkPipelineStageFlags dstStageMask,
                                     uint32_t memoryBarrierCount,
                                     const VkMemoryBarrier* pMemoryBarriers)
{
	waitEvents(1, &event, srcStageMask, dstStageMask, memoryBarrierCount,
	           pMemoryBarriers, 0, nullptr, 0, nullptr);
}
inline void CommandBuffer::waitEvent(VkEvent event,
                                     VkPipelineStageFlags srcStageMask,
                                     VkPipelineStageFlags dstStageMask,
                                     const VkMemoryBarrier& memoryBarrier)
{
	waitEvents(1, &event, srcStageMask, dstStageMask, 1, &memoryBarrier);
}
inline void CommandBuffer::waitEvent(
    VkEvent event, VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask, uint32_t bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier* pBufferMemoryBarriers)
{
	waitEvents(1, &event, srcStageMask, dstStageMask, 0, nullptr,
	           bufferMemoryBarrierCount, pBufferMemoryBarriers, 0, nullptr);
}
inline void CommandBuffer::waitEvent(
    VkEvent event, VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    const VkBufferMemoryBarrier& bufferMemoryBarrier)
{
	waitEvents(1, &event, srcStageMask, dstStageMask, 1, &bufferMemoryBarrier);
}
inline void CommandBuffer::waitEvent(
    VkEvent event, VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask, uint32_t imageMemoryBarrierCount,
    const VkImageMemoryBarrier* pImageMemoryBarriers)
{
	waitEvents(1, &event, srcStageMask, dstStageMask, 0, nullptr, 0, nullptr,
	           imageMemoryBarrierCount, pImageMemoryBarriers);
}
inline void CommandBuffer::waitEvent(
    VkEvent event, VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    const VkImageMemoryBarrier& imageMemoryBarrier)
{
	waitEvents(1, &event, srcStageMask, dstStageMask, 1, &imageMemoryBarrier);
}

inline void CommandBuffer::writeTimestamp(
    VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool,
    uint32_t query)
{
	device().CmdWriteTimestamp(m_commandBuffer.get(), pipelineStage, queryPool,
	                           query);
}

inline VkResult CommandBuffer::end()
{
	return device().EndCommandBuffer(m_commandBuffer.get());
}

inline VkResult CommandBuffer::reset(VkCommandBufferResetFlags flags)
{
	device().ResetCommandBuffer(m_commandBuffer.get(), flags);
}
