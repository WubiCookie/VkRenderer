#include "CommandBuffer.hpp"

#include <stdexcept>

namespace cdm
{
CommandBuffer::CommandBuffer(const VulkanDevice& device_,
                             VkCommandPool parentCommandPool,
                             VkCommandBufferLevel level)
    : VulkanDeviceObject(device_),
      m_parentCommandPool(parentCommandPool)
{
	auto& vk = this->device();

	if (m_parentCommandPool.get() == nullptr)
	{
		throw std::runtime_error("error: parentCommandPool is nullptr");
	}

	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.commandPool = m_parentCommandPool.get();
	allocInfo.level = level;
	allocInfo.commandBufferCount = 1;

	if (vk.allocate(allocInfo, &m_commandBuffer.get()) != VK_SUCCESS)
	{
		throw std::runtime_error("error: failed to allocate command buffers");
	}
}

CommandBuffer::CommandBuffer(CommandBuffer&& cb) noexcept
    : VulkanDeviceObject(std::move(cb)),
      m_parentCommandPool(std::exchange(cb.m_parentCommandPool, nullptr)),
      m_commandBuffer(std::exchange(cb.m_commandBuffer, nullptr))
{
}

CommandBuffer::~CommandBuffer()
{
	auto& vk = this->device();

	if (m_parentCommandPool.get())
	{
		vk.free(m_parentCommandPool.get(), m_commandBuffer.get());
	}
}

CommandBuffer& CommandBuffer::operator=(CommandBuffer&& cb) noexcept
{
	VulkanDeviceObject::operator=(std::move(cb));

	m_parentCommandPool = std::exchange(cb.m_parentCommandPool, nullptr);
	m_commandBuffer = std::exchange(cb.m_commandBuffer, nullptr);

	return *this;
}

VkCommandBuffer& CommandBuffer::get() { return m_commandBuffer.get(); }

VkCommandBuffer& CommandBuffer::commandBuffer()
{
	return m_commandBuffer.get();
}

VkResult CommandBuffer::begin()
{
	vk::CommandBufferBeginInfo beginInfo;
	return begin(beginInfo);
}

VkResult CommandBuffer::begin(const vk::CommandBufferBeginInfo& beginInfo)
{
	return device().BeginCommandBuffer(m_commandBuffer.get(), &beginInfo);
}

VkResult CommandBuffer::begin(VkCommandBufferUsageFlags usage)
{
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = usage;
	return device().BeginCommandBuffer(m_commandBuffer.get(), &beginInfo);
}

void CommandBuffer::beginQuery(VkQueryPool queryPool, uint32_t query,
                               VkQueryControlFlags flags)
{
	device().CmdBeginQuery(m_commandBuffer.get(), queryPool, query, flags);
}

void CommandBuffer::beginRenderPass(
    const vk::RenderPassBeginInfo& renderPassInfo, VkSubpassContents contents)
{
	device().CmdBeginRenderPass(m_commandBuffer.get(), &renderPassInfo,
	                            contents);
}

void CommandBuffer::beginRenderPass2(
    const vk::RenderPassBeginInfo& renderPassInfo,
    const vk::SubpassBeginInfo& subpassInfo)
{
	device().CmdBeginRenderPass2KHR(m_commandBuffer.get(), &renderPassInfo,
	                                &subpassInfo);
}

void CommandBuffer::bindDescriptorSets(VkPipelineBindPoint pipelineBindPoint,
                                       VkPipelineLayout layout,
                                       uint32_t firstSet,
                                       uint32_t descriptorSetCount,
                                       const VkDescriptorSet* pDescriptorSets,
                                       uint32_t dynamicOffsetCount,
                                       const uint32_t* pDynamicOffsets)
{
	device().CmdBindDescriptorSets(m_commandBuffer.get(), pipelineBindPoint,
	                               layout, firstSet, descriptorSetCount,
	                               pDescriptorSets, dynamicOffsetCount,
	                               pDynamicOffsets);
}

void CommandBuffer::bindDescriptorSet(VkPipelineBindPoint pipelineBindPoint,
                                      VkPipelineLayout layout,
                                      uint32_t firstSet,
                                      VkDescriptorSet descriptorSet,
                                      uint32_t dynamicOffsetCount,
                                      const uint32_t* pDynamicOffsets)
{
	bindDescriptorSets(pipelineBindPoint, layout, firstSet, 1, &descriptorSet,
	                   dynamicOffsetCount, pDynamicOffsets);
}

void CommandBuffer::bindIndexBuffer(VkBuffer buffer, VkDeviceSize offset,
                                    VkIndexType indexType)
{
	device().CmdBindIndexBuffer(m_commandBuffer.get(), buffer, offset,
	                            indexType);
}

void CommandBuffer::bindPipeline(VkPipelineBindPoint pipelineBindPoint,
                                 VkPipeline pipeline)
{
	device().CmdBindPipeline(m_commandBuffer.get(), pipelineBindPoint,
	                         pipeline);
}

void CommandBuffer::bindVertexBuffers(uint32_t firstBinding,
                                      uint32_t bindingCount,
                                      const VkBuffer* pBuffers,
                                      const VkDeviceSize* pOffsets)
{
	device().CmdBindVertexBuffers(m_commandBuffer.get(), firstBinding,
	                              bindingCount, pBuffers, pOffsets);
}

void CommandBuffer::bindVertexBuffer(uint32_t firstBinding, VkBuffer buffer,
                                     VkDeviceSize offset)
{
	bindVertexBuffers(firstBinding, 1, &buffer, &offset);
}

void CommandBuffer::bindVertexBuffer(VkBuffer buffer)
{
	bindVertexBuffer(0, buffer, 0);
}

void CommandBuffer::blitImage(VkImage srcImage, VkImageLayout srcImageLayout,
                              VkImage dstImage, VkImageLayout dstImageLayout,
                              uint32_t regionCount,
                              const VkImageBlit* pRegions, VkFilter filter)
{
	device().CmdBlitImage(m_commandBuffer.get(), srcImage, srcImageLayout,
	                      dstImage, dstImageLayout, regionCount, pRegions,
	                      filter);
}

void CommandBuffer::blitImage(VkImage srcImage, VkImageLayout srcImageLayout,
                              VkImage dstImage, VkImageLayout dstImageLayout,
                              const VkImageBlit& region, VkFilter filter)
{
	blitImage(srcImage, srcImageLayout, dstImage, dstImageLayout, 1, &region,
	          filter);
}

void CommandBuffer::clearAttachments(uint32_t attachmentCount,
                                     const VkClearAttachment* pAttachments,
                                     uint32_t rectCount,
                                     const VkClearRect* pRects)
{
	device().CmdClearAttachments(m_commandBuffer.get(), attachmentCount,
	                             pAttachments, rectCount, pRects);
}

void CommandBuffer::clearAttachments(const VkClearAttachment& attachment,
                                     uint32_t rectCount,
                                     const VkClearRect* pRects)
{
	clearAttachments(1, &attachment, rectCount, pRects);
}

void CommandBuffer::clearAttachments(uint32_t attachmentCount,
                                     const VkClearAttachment* pAttachments,
                                     const VkClearRect& rect)
{
	clearAttachments(attachmentCount, pAttachments, 1, &rect);
}

void CommandBuffer::clearAttachments(const VkClearAttachment& attachment,
                                     const VkClearRect& rect)
{
	clearAttachments(1, &attachment, 1, &rect);
}

void CommandBuffer::clearColorImage(VkImage image, VkImageLayout imageLayout,
                                    const VkClearColorValue* pColor,
                                    uint32_t rangeCount,
                                    const VkImageSubresourceRange* pRanges)
{
	device().CmdClearColorImage(m_commandBuffer.get(), image, imageLayout,
	                            pColor, rangeCount, pRanges);
}

void CommandBuffer::clearColorImage(VkImage image, VkImageLayout imageLayout,
                                    const VkClearColorValue* pColor,
                                    const VkImageSubresourceRange& range)
{
	clearColorImage(image, imageLayout, pColor, 1, &range);
}

void CommandBuffer::clearDepthStencilImage(
    VkImage image, VkImageLayout imageLayout,
    const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount,
    const VkImageSubresourceRange* pRanges)
{
	device().CmdClearDepthStencilImage(m_commandBuffer.get(), image,
	                                   imageLayout, pDepthStencil, rangeCount,
	                                   pRanges);
}

void CommandBuffer::clearDepthStencilImage(
    VkImage image, VkImageLayout imageLayout,
    const VkClearDepthStencilValue* pDepthStencil,
    const VkImageSubresourceRange& range)
{
	clearDepthStencilImage(image, imageLayout, pDepthStencil, 1, &range);
}

void CommandBuffer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                               uint32_t regionCount,
                               const VkBufferCopy* pRegions)
{
	device().CmdCopyBuffer(m_commandBuffer.get(), srcBuffer, dstBuffer,
	                       regionCount, pRegions);
}

void CommandBuffer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                               const VkBufferCopy& region)
{
	copyBuffer(srcBuffer, dstBuffer, 1, &region);
}

void CommandBuffer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                               VkDeviceSize size, VkDeviceSize srcOffset,
                               VkDeviceSize dstOffset)
{
	VkBufferCopy region{};
	region.size = size;
	region.srcOffset = srcOffset;
	region.dstOffset = dstOffset;

	copyBuffer(srcBuffer, dstBuffer, region);
}

void CommandBuffer::copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage,
                                      VkImageLayout dstImageLayout,
                                      uint32_t regionCount,
                                      const VkBufferImageCopy* pRegions)
{
	device().CmdCopyBufferToImage(m_commandBuffer.get(), srcBuffer, dstImage,
	                              dstImageLayout, regionCount, pRegions);
}

void CommandBuffer::copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage,
                                      VkImageLayout dstImageLayout,
                                      const VkBufferImageCopy& region)
{
	copyBufferToImage(srcBuffer, dstImage, dstImageLayout, 1, &region);
}

void CommandBuffer::copyImage(VkImage srcImage, VkImageLayout srcImageLayout,
                              VkImage dstImage, VkImageLayout dstImageLayout,
                              uint32_t regionCount,
                              const VkImageCopy* pRegions)
{
	device().CmdCopyImage(m_commandBuffer.get(), srcImage, srcImageLayout,
	                      dstImage, dstImageLayout, regionCount, pRegions);
}

void CommandBuffer::copyImage(VkImage srcImage, VkImageLayout srcImageLayout,
                              VkImage dstImage, VkImageLayout dstImageLayout,
                              const VkImageCopy& region)
{
	copyImage(srcImage, srcImageLayout, dstImage, dstImageLayout, 1, &region);
}

void CommandBuffer::copyImageToBuffer(VkImage srcImage,
                                      VkImageLayout srcImageLayout,
                                      VkBuffer dstBuffer, uint32_t regionCount,
                                      const VkBufferImageCopy* pRegions)
{
	device().CmdCopyImageToBuffer(m_commandBuffer.get(), srcImage,
	                              srcImageLayout, dstBuffer, regionCount,
	                              pRegions);
}

void CommandBuffer::copyImageToBuffer(VkImage srcImage,
                                      VkImageLayout srcImageLayout,
                                      VkBuffer dstBuffer,
                                      const VkBufferImageCopy& region)
{
	copyImageToBuffer(srcImage, srcImageLayout, dstBuffer, 1, &region);
}

void CommandBuffer::copyQueryPoolResults(
    VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount,
    VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride,
    VkQueryResultFlags flags)
{
	device().CmdCopyQueryPoolResults(m_commandBuffer.get(), queryPool,
	                                 firstQuery, queryCount, dstBuffer,
	                                 dstOffset, stride, flags);
}

void CommandBuffer::debugMarkerBegin(
    const vk::DebugMarkerMarkerInfoEXT& markerInfo)
{
	if (device().CmdDebugMarkerBeginEXT)
		device().CmdDebugMarkerBeginEXT(m_commandBuffer.get(), &markerInfo);
}

void CommandBuffer::debugMarkerBegin(std::string_view markerName,
                                     std::array<float, 4> color)
{
	vk::DebugMarkerMarkerInfoEXT markerInfo;
	markerInfo.pMarkerName = markerName.data();
	markerInfo.color[0] = color[0];
	markerInfo.color[1] = color[1];
	markerInfo.color[2] = color[2];
	markerInfo.color[3] = color[3];

	debugMarkerBegin(markerInfo);
}

void CommandBuffer::debugMarkerBegin(std::string_view markerName, float colorR,
                                     float colorG, float colorB, float colorA)
{
	vk::DebugMarkerMarkerInfoEXT markerInfo;
	markerInfo.pMarkerName = markerName.data();
	markerInfo.color[0] = colorR;
	markerInfo.color[1] = colorG;
	markerInfo.color[2] = colorB;
	markerInfo.color[3] = colorA;

	debugMarkerBegin(markerInfo);
}

void CommandBuffer::debugMarkerEnd()
{
	if (device().CmdDebugMarkerEndEXT)
		device().CmdDebugMarkerEndEXT(m_commandBuffer.get());
}

void CommandBuffer::debugMarkerInsert(
    const vk::DebugMarkerMarkerInfoEXT& markerInfo)
{
	if (device().CmdDebugMarkerInsertEXT)
		device().CmdDebugMarkerInsertEXT(m_commandBuffer.get(), &markerInfo);
}

void CommandBuffer::debugMarkerInsert(std::string_view markerName,
                                      std::array<float, 4> color)
{
	vk::DebugMarkerMarkerInfoEXT markerInfo;
	markerInfo.pMarkerName = markerName.data();
	markerInfo.color[0] = color[0];
	markerInfo.color[1] = color[1];
	markerInfo.color[2] = color[2];
	markerInfo.color[3] = color[3];

	debugMarkerInsert(markerInfo);
}

void CommandBuffer::debugMarkerInsert(std::string_view markerName,
                                      float colorR, float colorG, float colorB,
                                      float colorA)
{
	vk::DebugMarkerMarkerInfoEXT markerInfo;
	markerInfo.pMarkerName = markerName.data();
	markerInfo.color[0] = colorR;
	markerInfo.color[1] = colorG;
	markerInfo.color[2] = colorB;
	markerInfo.color[3] = colorA;

	debugMarkerInsert(markerInfo);
}

void CommandBuffer::dispatch(uint32_t groupCountX, uint32_t groupCountY,
                             uint32_t groupCountZ)
{
	device().CmdDispatch(m_commandBuffer.get(), groupCountX, groupCountY,
	                     groupCountZ);
}

void CommandBuffer::dispatchBase(uint32_t baseGroupX, uint32_t baseGroupY,
                                 uint32_t baseGroupZ, uint32_t groupCountX,
                                 uint32_t groupCountY, uint32_t groupCountZ)
{
	device().CmdDispatchBase(m_commandBuffer.get(), baseGroupX, baseGroupY,
	                         baseGroupZ, groupCountX, groupCountY,
	                         groupCountZ);
}

void CommandBuffer::dispatchIndirect(VkBuffer buffer, VkDeviceSize offset)
{
	device().CmdDispatchIndirect(m_commandBuffer.get(), buffer, offset);
}

void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount,
                         uint32_t firstVertex, uint32_t firstInstance)
{
	device().CmdDraw(m_commandBuffer.get(), vertexCount, instanceCount,
	                 firstVertex, firstInstance);
}

void CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount,
                                uint32_t firstIndex, int32_t vertexOffset,
                                uint32_t firstInstance)
{
	device().CmdDrawIndexed(m_commandBuffer.get(), indexCount, instanceCount,
	                        firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::drawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset,
                                        uint32_t drawCount, uint32_t stride)
{
	device().CmdDrawIndexedIndirect(m_commandBuffer.get(), buffer, offset,
	                                drawCount, stride);
}

void CommandBuffer::drawIndexedIndirectCount(
    VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer,
    VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
	device().CmdDrawIndexedIndirectCount(m_commandBuffer.get(), buffer, offset,
	                                     countBuffer, countBufferOffset,
	                                     maxDrawCount, stride);
}

void CommandBuffer::drawIndirect(VkBuffer buffer, VkDeviceSize offset,
                                 uint32_t drawCount, uint32_t stride)
{
	device().CmdDrawIndirect(m_commandBuffer.get(), buffer, offset, drawCount,
	                         stride);
}

void CommandBuffer::drawIndirectCount(VkBuffer buffer, VkDeviceSize offset,
                                      VkBuffer countBuffer,
                                      VkDeviceSize countBufferOffset,
                                      uint32_t maxDrawCount, uint32_t stride)
{
	device().CmdDrawIndirectCount(m_commandBuffer.get(), buffer, offset,
	                              countBuffer, countBufferOffset, maxDrawCount,
	                              stride);
}

void CommandBuffer::endQuery(VkQueryPool queryPool, uint32_t query)
{
	device().CmdEndQuery(m_commandBuffer.get(), queryPool, query);
}

void CommandBuffer::endRenderPass()
{
	device().CmdEndRenderPass(m_commandBuffer.get());
}

void CommandBuffer::endRenderPass2(const vk::SubpassEndInfo& subpassEndInfo)
{
	device().CmdEndRenderPass2KHR(m_commandBuffer.get(), &subpassEndInfo);
}

void CommandBuffer::executeCommands(uint32_t commandBufferCount,
                                    const VkCommandBuffer* pCommandBuffers)
{
	device().CmdExecuteCommands(m_commandBuffer.get(), commandBufferCount,
	                            pCommandBuffers);
}

void CommandBuffer::executeCommand(VkCommandBuffer commandBuffer)
{
	executeCommands(1, &commandBuffer);
}

void CommandBuffer::fillBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset,
                               VkDeviceSize size, uint32_t data)
{
	device().CmdFillBuffer(m_commandBuffer.get(), dstBuffer, dstOffset, size,
	                       data);
}

void CommandBuffer::nextSubpass(VkSubpassContents contents)
{
	device().CmdNextSubpass(m_commandBuffer.get(), contents);
}

void CommandBuffer::nextSubpass2(const vk::SubpassBeginInfo& subpassBeginInfo,
                                 const vk::SubpassEndInfo& subpassEndInfo)
{
	device().CmdNextSubpass2(m_commandBuffer.get(), &subpassBeginInfo,
	                         &subpassEndInfo);
}

void CommandBuffer::pipelineBarrier(
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

void CommandBuffer::pipelineBarrier(VkPipelineStageFlags srcStageMask,
                                    VkPipelineStageFlags dstStageMask,
                                    VkDependencyFlags dependencyFlags,
                                    uint32_t memoryBarrierCount,
                                    const VkMemoryBarrier* pMemoryBarriers)
{
	pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags,
	                memoryBarrierCount, pMemoryBarriers, 0, nullptr, 0,
	                nullptr);
}
void CommandBuffer::pipelineBarrier(VkPipelineStageFlags srcStageMask,
                                    VkPipelineStageFlags dstStageMask,
                                    VkDependencyFlags dependencyFlags,
                                    const VkMemoryBarrier& memoryBarrier)
{
	pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, 1,
	                &memoryBarrier);
}
void CommandBuffer::pipelineBarrier(
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    VkDependencyFlags dependencyFlags, uint32_t bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier* pBufferMemoryBarriers)
{
	pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, 0, nullptr,
	                bufferMemoryBarrierCount, pBufferMemoryBarriers, 0,
	                nullptr);
}
void CommandBuffer::pipelineBarrier(
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    VkDependencyFlags dependencyFlags,
    const VkBufferMemoryBarrier& bufferMemoryBarrier)
{
	pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, 1,
	                &bufferMemoryBarrier);
}
void CommandBuffer::pipelineBarrier(
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    VkDependencyFlags dependencyFlags, uint32_t imageMemoryBarrierCount,
    const VkImageMemoryBarrier* pImageMemoryBarriers)
{
	pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, 0, nullptr, 0,
	                nullptr, imageMemoryBarrierCount, pImageMemoryBarriers);
}
void CommandBuffer::pipelineBarrier(
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    VkDependencyFlags dependencyFlags,
    const VkImageMemoryBarrier& imageMemoryBarrier)
{
	pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, 1,
	                &imageMemoryBarrier);
}

void CommandBuffer::pushConstants(VkPipelineLayout layout,
                                  VkShaderStageFlags stageFlags,
                                  uint32_t offset, uint32_t size,
                                  const void* pValues)
{
	device().CmdPushConstants(m_commandBuffer.get(), layout, stageFlags,
	                          offset, size, pValues);
}

void CommandBuffer::resetEvent(VkEvent event, VkPipelineStageFlags stageMask)
{
	device().CmdResetEvent(m_commandBuffer.get(), event, stageMask);
}

void CommandBuffer::resetQueryPool(VkQueryPool queryPool, uint32_t firstQuery,
                                   uint32_t queryCount)
{
	device().CmdResetQueryPool(m_commandBuffer.get(), queryPool, firstQuery,
	                           queryCount);
}

void CommandBuffer::resolveImage(VkImage srcImage,
                                 VkImageLayout srcImageLayout,
                                 VkImage dstImage,
                                 VkImageLayout dstImageLayout,
                                 uint32_t regionCount,
                                 const VkImageResolve* pRegions)
{
	device().CmdResolveImage(m_commandBuffer.get(), srcImage, srcImageLayout,
	                         dstImage, dstImageLayout, regionCount, pRegions);
}

void CommandBuffer::resolveImage(VkImage srcImage,
                                 VkImageLayout srcImageLayout,
                                 VkImage dstImage,
                                 VkImageLayout dstImageLayout,
                                 const VkImageResolve& region)
{
	resolveImage(srcImage, srcImageLayout, dstImage, dstImageLayout, 1,
	             &region);
}

void CommandBuffer::setBlendConstants(const float blendConstants[4])
{
	device().CmdSetBlendConstants(m_commandBuffer.get(), blendConstants);
}

void CommandBuffer::setBlendConstants(float blendConstants0,
                                      float blendConstants1,
                                      float blendConstants2,
                                      float blendConstants3)
{
	std::array blendConstants = { blendConstants0, blendConstants1,
		                          blendConstants2, blendConstants3 };
	setBlendConstants(blendConstants.data());
}

void CommandBuffer::setDepthBias(float depthBiasConstantFactor,
                                 float depthBiasClamp,
                                 float depthBiasSlopeFactor)
{
	device().CmdSetDepthBias(m_commandBuffer.get(), depthBiasConstantFactor,
	                         depthBiasClamp, depthBiasSlopeFactor);
}

void CommandBuffer::setDepthBounds(float minDepthBounds, float maxDepthBounds)
{
	device().CmdSetDepthBounds(m_commandBuffer.get(), minDepthBounds,
	                           maxDepthBounds);
}

void CommandBuffer::setDeviceMask(uint32_t deviceMask)
{
	device().CmdSetDeviceMask(m_commandBuffer.get(), deviceMask);
}

void CommandBuffer::setEvent(VkEvent event, VkPipelineStageFlags stageMask)
{
	device().CmdSetEvent(m_commandBuffer.get(), event, stageMask);
}

void CommandBuffer::setLineWidth(float lineWidth)
{
	device().CmdSetLineWidth(m_commandBuffer.get(), lineWidth);
}

void CommandBuffer::setScissor(uint32_t firstScissor, uint32_t scissorCount,
                               const VkRect2D* pScissors)
{
	device().CmdSetScissor(m_commandBuffer.get(), firstScissor, scissorCount,
	                       pScissors);
}

void CommandBuffer::setScissor(uint32_t firstScissor, const VkRect2D& scissor)
{
	setScissor(firstScissor, 1, &scissor);
}

void CommandBuffer::setStencilCompareMask(VkStencilFaceFlags faceMask,
                                          uint32_t compareMask)
{
	device().CmdSetStencilCompareMask(m_commandBuffer.get(), faceMask,
	                                  compareMask);
}

void CommandBuffer::setStencilReference(VkStencilFaceFlags faceMask,
                                        uint32_t reference)
{
	device().CmdSetStencilReference(m_commandBuffer.get(), faceMask,
	                                reference);
}

void CommandBuffer::setStencilWriteMask(VkStencilFaceFlags faceMask,
                                        uint32_t writeMask)
{
	device().CmdSetStencilWriteMask(m_commandBuffer.get(), faceMask,
	                                writeMask);
}

void CommandBuffer::setViewport(uint32_t firstViewport, uint32_t viewportCount,
                                const VkViewport* pViewports)
{
	device().CmdSetViewport(m_commandBuffer.get(), firstViewport,
	                        viewportCount, pViewports);
}

void CommandBuffer::setViewport(uint32_t firstViewport,
                                const VkViewport& viewport)
{
	setViewport(firstViewport, 1, &viewport);
}

void CommandBuffer::updateBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset,
                                 VkDeviceSize dataSize, const void* pData)
{
	device().CmdUpdateBuffer(m_commandBuffer.get(), dstBuffer, dstOffset,
	                         dataSize, pData);
}

void CommandBuffer::waitEvents(
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

void CommandBuffer::waitEvents(uint32_t eventCount, const VkEvent* pEvents,
                               VkPipelineStageFlags srcStageMask,
                               VkPipelineStageFlags dstStageMask,
                               uint32_t memoryBarrierCount,
                               const VkMemoryBarrier* pMemoryBarriers)
{
	waitEvents(eventCount, pEvents, srcStageMask, dstStageMask,
	           memoryBarrierCount, pMemoryBarriers, 0, nullptr, 0, nullptr);
}
void CommandBuffer::waitEvents(uint32_t eventCount, const VkEvent* pEvents,
                               VkPipelineStageFlags srcStageMask,
                               VkPipelineStageFlags dstStageMask,
                               const VkMemoryBarrier& memoryBarrier)
{
	waitEvents(eventCount, pEvents, srcStageMask, dstStageMask, 1,
	           &memoryBarrier);
}
void CommandBuffer::waitEvents(
    uint32_t eventCount, const VkEvent* pEvents,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    uint32_t bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier* pBufferMemoryBarriers)
{
	waitEvents(eventCount, pEvents, srcStageMask, dstStageMask, 0, nullptr,
	           bufferMemoryBarrierCount, pBufferMemoryBarriers, 0, nullptr);
}
void CommandBuffer::waitEvents(
    uint32_t eventCount, const VkEvent* pEvents,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    const VkBufferMemoryBarrier& bufferMemoryBarrier)
{
	waitEvents(eventCount, pEvents, srcStageMask, dstStageMask, 1,
	           &bufferMemoryBarrier);
}
void CommandBuffer::waitEvents(
    uint32_t eventCount, const VkEvent* pEvents,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    uint32_t imageMemoryBarrierCount,
    const VkImageMemoryBarrier* pImageMemoryBarriers)
{
	waitEvents(eventCount, pEvents, srcStageMask, dstStageMask, 0, nullptr, 0,
	           nullptr, imageMemoryBarrierCount, pImageMemoryBarriers);
}
void CommandBuffer::waitEvents(uint32_t eventCount, const VkEvent* pEvents,
                               VkPipelineStageFlags srcStageMask,
                               VkPipelineStageFlags dstStageMask,
                               const VkImageMemoryBarrier& imageMemoryBarrier)
{
	waitEvents(eventCount, pEvents, srcStageMask, dstStageMask, 1,
	           &imageMemoryBarrier);
}
void CommandBuffer::waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask,
                              VkPipelineStageFlags dstStageMask,
                              uint32_t memoryBarrierCount,
                              const VkMemoryBarrier* pMemoryBarriers)
{
	waitEvents(1, &event, srcStageMask, dstStageMask, memoryBarrierCount,
	           pMemoryBarriers, 0, nullptr, 0, nullptr);
}
void CommandBuffer::waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask,
                              VkPipelineStageFlags dstStageMask,
                              const VkMemoryBarrier& memoryBarrier)
{
	waitEvents(1, &event, srcStageMask, dstStageMask, 1, &memoryBarrier);
}
void CommandBuffer::waitEvent(
    VkEvent event, VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask, uint32_t bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier* pBufferMemoryBarriers)
{
	waitEvents(1, &event, srcStageMask, dstStageMask, 0, nullptr,
	           bufferMemoryBarrierCount, pBufferMemoryBarriers, 0, nullptr);
}
void CommandBuffer::waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask,
                              VkPipelineStageFlags dstStageMask,
                              const VkBufferMemoryBarrier& bufferMemoryBarrier)
{
	waitEvents(1, &event, srcStageMask, dstStageMask, 1, &bufferMemoryBarrier);
}
void CommandBuffer::waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask,
                              VkPipelineStageFlags dstStageMask,
                              uint32_t imageMemoryBarrierCount,
                              const VkImageMemoryBarrier* pImageMemoryBarriers)
{
	waitEvents(1, &event, srcStageMask, dstStageMask, 0, nullptr, 0, nullptr,
	           imageMemoryBarrierCount, pImageMemoryBarriers);
}
void CommandBuffer::waitEvent(VkEvent event, VkPipelineStageFlags srcStageMask,
                              VkPipelineStageFlags dstStageMask,
                              const VkImageMemoryBarrier& imageMemoryBarrier)
{
	waitEvents(1, &event, srcStageMask, dstStageMask, 1, &imageMemoryBarrier);
}

void CommandBuffer::writeTimestamp(VkPipelineStageFlagBits pipelineStage,
                                   VkQueryPool queryPool, uint32_t query)
{
	device().CmdWriteTimestamp(m_commandBuffer.get(), pipelineStage, queryPool,
	                           query);
}

VkResult CommandBuffer::end()
{
	return device().EndCommandBuffer(m_commandBuffer.get());
}

VkResult CommandBuffer::reset(VkCommandBufferResetFlags flags)
{
	return device().ResetCommandBuffer(m_commandBuffer.get(), flags);
}

bool CommandBuffer::outdated() const { return false; }

void CommandBuffer::recreate() {}
}  // namespace cdm
