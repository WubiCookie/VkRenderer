#pragma once

#include "VulkanDevice.hpp"

namespace cdm
{
inline void VulkanDeviceBase::destroySurface(VkSurfaceKHR surface) const
{
	DestroySurfaceKHR(instance(), surface, nullptr);
}

inline void VulkanDeviceBase::destroy(VkSurfaceKHR surface) const
{
	destroySurface(surface);
}

inline VkResult VulkanDeviceBase::createSurface(
    const cdm::vk::Win32SurfaceCreateInfoKHR& createInfo,
    VkSurfaceKHR& outSurface) const
{
	return CreateWin32SurfaceKHR(instance(), &createInfo, nullptr,
	                             &outSurface);
}

inline VkResult VulkanDeviceBase::create(
    const cdm::vk::Win32SurfaceCreateInfoKHR& createInfo,
    VkSurfaceKHR& outSurface) const
{
	return createSurface(createInfo, outSurface);
}

inline VkResult VulkanDevice::allocateCommandBuffers(
    const cdm::vk::CommandBufferAllocateInfo& allocateInfo,
    VkCommandBuffer* pCommandBuffers) const
{
	return AllocateCommandBuffers(vkDevice(), &allocateInfo, pCommandBuffers);
}

inline VkResult VulkanDevice::allocate(
    const cdm::vk::CommandBufferAllocateInfo& allocateInfo,
    VkCommandBuffer* pCommandBuffers) const
{
	return allocateCommandBuffers(allocateInfo, pCommandBuffers);
}

inline VkResult VulkanDevice::allocateDescriptorSets(
    const cdm::vk::DescriptorSetAllocateInfo& allocateInfo,
    VkDescriptorSet* pDescriptorSets) const
{
	return AllocateDescriptorSets(vkDevice(), &allocateInfo, pDescriptorSets);
}

inline VkResult VulkanDevice::allocate(
    const cdm::vk::DescriptorSetAllocateInfo& allocateInfo,
    VkDescriptorSet* pDescriptorSets) const
{
	return allocateDescriptorSets(allocateInfo, pDescriptorSets);
}

inline VkResult VulkanDevice::allocateMemory(
    const cdm::vk::MemoryAllocateInfo& allocateInfo,
    VkDeviceMemory& outDeviceMemory) const
{
	return AllocateMemory(vkDevice(), &allocateInfo, nullptr,
	                      &outDeviceMemory);
}

inline VkResult VulkanDevice::allocate(
    const cdm::vk::MemoryAllocateInfo& allocateInfo,
    VkDeviceMemory& outDeviceMemory) const
{
	return allocateMemory(allocateInfo, outDeviceMemory);
}

inline VkResult VulkanDevice::createComputePipelines(
    uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos,
    VkPipeline* pPipelines, VkPipelineCache pipelineCache) const
{
	return CreateComputePipelines(vkDevice(), pipelineCache, createInfoCount,
	                              pCreateInfos, nullptr, pPipelines);
}

inline VkResult VulkanDevice::create(
    uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos,
    VkPipeline* pPipelines, VkPipelineCache pipelineCache) const
{
	return createComputePipelines(createInfoCount, pCreateInfos, pPipelines,
	                              pipelineCache);
}

inline VkResult VulkanDevice::createComputePipelines(
    uint32_t createInfoCount,
    const cdm::vk::ComputePipelineCreateInfo* pCreateInfos,
    VkPipeline* pPipelines, VkPipelineCache pipelineCache) const
{
	return CreateComputePipelines(vkDevice(), pipelineCache, createInfoCount,
	                              pCreateInfos, nullptr, pPipelines);
}

inline VkResult VulkanDevice::create(
    uint32_t createInfoCount,
    const cdm::vk::ComputePipelineCreateInfo* pCreateInfos,
    VkPipeline* pPipelines, VkPipelineCache pipelineCache) const
{
	return createComputePipelines(createInfoCount, pCreateInfos, pPipelines,
	                              pipelineCache);
}

inline VkResult VulkanDevice::createComputePipeline(
    const cdm::vk::ComputePipelineCreateInfo& createInfo,
    VkPipeline& outPipeline, VkPipelineCache pipelineCache) const
{
	return CreateComputePipelines(vkDevice(), pipelineCache, 1, &createInfo,
	                              nullptr, &outPipeline);
}

inline VkResult VulkanDevice::create(
    const cdm::vk::ComputePipelineCreateInfo& createInfo,
    VkPipeline& outPipeline, VkPipelineCache pipelineCache) const
{
	return createComputePipeline(createInfo, outPipeline, pipelineCache);
}

inline VkResult VulkanDevice::createGraphicsPipelines(
    uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos,
    VkPipeline* pPipelines, VkPipelineCache pipelineCache) const
{
	return CreateGraphicsPipelines(vkDevice(), pipelineCache, createInfoCount,
	                               pCreateInfos, nullptr, pPipelines);
}

inline VkResult VulkanDevice::create(
    uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos,
    VkPipeline* pPipelines, VkPipelineCache pipelineCache) const
{
	return createGraphicsPipelines(createInfoCount, pCreateInfos, pPipelines,
	                               pipelineCache);
}

inline VkResult VulkanDevice::createGraphicsPipelines(
    uint32_t createInfoCount,
    const cdm::vk::GraphicsPipelineCreateInfo* pCreateInfos,
    VkPipeline* pPipelines, VkPipelineCache pipelineCache) const
{
	return CreateGraphicsPipelines(vkDevice(), pipelineCache, createInfoCount,
	                               pCreateInfos, nullptr, pPipelines);
}

inline VkResult VulkanDevice::create(
    uint32_t createInfoCount,
    const cdm::vk::GraphicsPipelineCreateInfo* pCreateInfos,
    VkPipeline* pPipelines, VkPipelineCache pipelineCache) const
{
	return createGraphicsPipelines(createInfoCount, pCreateInfos, pPipelines,
	                               pipelineCache);
}

inline VkResult VulkanDevice::createGraphicsPipeline(
    const cdm::vk::GraphicsPipelineCreateInfo& createInfo,
    VkPipeline& outPipeline, VkPipelineCache pipelineCache) const
{
	return CreateGraphicsPipelines(vkDevice(), pipelineCache, 1, &createInfo,
	                               nullptr, &outPipeline);
}

inline VkResult VulkanDevice::create(
    const cdm::vk::GraphicsPipelineCreateInfo& createInfo,
    VkPipeline& outPipeline, VkPipelineCache pipelineCache) const
{
	return createGraphicsPipeline(createInfo, outPipeline, pipelineCache);
}

inline VkResult VulkanDevice::createRenderPass2(
    const cdm::vk::RenderPassCreateInfo2& createInfo,
    VkRenderPass& outRenderPass) const
{
	return CreateRenderPass2(vkDevice(), &createInfo, nullptr, &outRenderPass);
}

inline VkResult VulkanDevice::create(
    const cdm::vk::RenderPassCreateInfo2& createInfo,
    VkRenderPass& outRenderPass) const
{
	return createRenderPass2(createInfo, outRenderPass);
}

inline void VulkanDevice::destroyDevice() const
{
	DestroyDevice(vkDevice(), nullptr);
}

inline void VulkanDevice::destroy() const { destroyDevice(); }

inline VkResult VulkanDevice::waitIdle() const
{
	return DeviceWaitIdle(vkDevice());
}
inline VkResult VulkanDevice::wait() const { return waitIdle(); }

inline void VulkanDevice::freeCommandBuffers(
    VkCommandPool commandPool, uint32_t commandBufferCount,
    const VkCommandBuffer* pCommandBuffers) const
{
	FreeCommandBuffers(vkDevice(), commandPool, commandBufferCount,
	                   pCommandBuffers);
}

inline void VulkanDevice::free(VkCommandPool commandPool,
                               uint32_t commandBufferCount,
                               const VkCommandBuffer* pCommandBuffers) const
{
	freeCommandBuffers(commandPool, commandBufferCount, pCommandBuffers);
}

inline void VulkanDevice::freeCommandBuffer(
    VkCommandPool commandPool, VkCommandBuffer CommandBuffer) const
{
	FreeCommandBuffers(vkDevice(), commandPool, 1, &CommandBuffer);
}

inline void VulkanDevice::free(VkCommandPool commandPool,
                               VkCommandBuffer CommandBuffer) const
{
	freeCommandBuffers(commandPool, 1, &CommandBuffer);
}

inline VkResult VulkanDevice::freeDescriptorSets(
    VkDescriptorPool descriptorPool, uint32_t descriptorSetCount,
    const VkDescriptorSet* pDescriptorSets) const
{
	return FreeDescriptorSets(vkDevice(), descriptorPool, descriptorSetCount,
	                          pDescriptorSets);
}

inline VkResult VulkanDevice::free(
    VkDescriptorPool descriptorPool, uint32_t descriptorSetCount,
    const VkDescriptorSet* pDescriptorSets) const
{
	return freeDescriptorSets(descriptorPool, descriptorSetCount,
	                          pDescriptorSets);
}
inline VkResult VulkanDevice::freeDescriptorSets(
    VkDescriptorPool descriptorPool, VkDescriptorSet DescriptorSet) const
{
	return FreeDescriptorSets(vkDevice(), descriptorPool, 1, &DescriptorSet);
}

inline VkResult VulkanDevice::free(VkDescriptorPool descriptorPool,
                                   VkDescriptorSet DescriptorSet) const
{
	return freeDescriptorSets(descriptorPool, 1, &DescriptorSet);
}

inline void VulkanDevice::freeMemory(VkDeviceMemory aDeviceMemory) const
{
	FreeMemory(vkDevice(), aDeviceMemory, nullptr);
}

inline void VulkanDevice::free(VkDeviceMemory aDeviceMemory) const
{
	freeMemory(aDeviceMemory);
}

inline VkResult VulkanDevice::queueWaitIdle(VkQueue queue) const
{
	return QueueWaitIdle(queue);
}

inline VkResult VulkanDevice::waitIdle(VkQueue queue) const
{
	return QueueWaitIdle(queue);
}

inline VkResult VulkanDevice::wait(VkQueue queue) const
{
	return QueueWaitIdle(queue);
}

inline VkResult VulkanDevice::waitForFences(uint32_t fenceCount,
                                            const VkFence* pFences,
                                            bool waitAll,
                                            uint64_t timeout) const
{
	return WaitForFences(vkDevice(), fenceCount, pFences, waitAll, timeout);
}

inline VkResult VulkanDevice::wait(uint32_t fenceCount, const VkFence* pFences,
                                   bool waitAll, uint64_t timeout) const
{
	return waitForFences(fenceCount, pFences, waitAll, timeout);
}

inline VkResult VulkanDevice::waitForFence(VkFence fence,
                                           uint64_t timeout) const
{
	return WaitForFences(vkDevice(), 1, &fence, true, timeout);
}

inline VkResult VulkanDevice::wait(VkFence fence, uint64_t timeout) const
{
	return waitForFence(fence, timeout);
}

inline VkResult VulkanDevice::waitSemaphores(
    cdm::vk::SemaphoreWaitInfo& waitInfo, uint64_t timeout) const
{
	return WaitSemaphores(vkDevice(), &waitInfo, timeout);
}

inline VkResult VulkanDevice::wait(cdm::vk::SemaphoreWaitInfo& waitInfo,
                                   uint64_t timeout) const
{
	return waitSemaphores(waitInfo, timeout);
}

inline VkResult VulkanDevice::createSwapchain(
    const cdm::vk::SwapchainCreateInfoKHR& createInfo,
    VkSwapchainKHR& outSwapchain) const
{
	return CreateSwapchainKHR(vkDevice(), &createInfo, nullptr, &outSwapchain);
}

inline VkResult VulkanDevice::create(
    const cdm::vk::SwapchainCreateInfoKHR& createInfo,
    VkSwapchainKHR& outSwapchain) const
{
	return createSwapchain(createInfo, outSwapchain);
}

inline void VulkanDevice::destroySwapchain(VkSwapchainKHR swapchain) const
{
	DestroySwapchainKHR(vkDevice(), swapchain, nullptr);
}

inline void VulkanDevice::destroy(VkSwapchainKHR swapchain) const
{
	destroySwapchain(swapchain);
}
}  // namespace cdm
