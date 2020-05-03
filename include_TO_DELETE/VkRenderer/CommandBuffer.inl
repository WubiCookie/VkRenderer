#pragma once

#include "CommandBuffer.hpp"

namespace cdm
{
template <typename T>
void CommandBuffer::pushConstants(VkPipelineLayout layout,
                                  VkShaderStageFlags stageFlags,
                                  uint32_t offset, const T* pValues)
{
	pushConstants(layout, stageFlags, offset, uint32_t(sizeof(T)),
	              reinterpret_cast<const void*>(pValues));
}
}  // namespace cdm
