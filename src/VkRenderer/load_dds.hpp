#ifndef LOAD_DDS_HPP
#define LOAD_DDS_HPP

#include "CommandBufferPool.hpp"
#include "Texture2D.hpp"
#include "TextureFactory.hpp"

cdm::Texture2D texture_loadDDS(
    const char* path, cdm::TextureFactory& factory,
    cdm::CommandBufferPool& pool,
    VkImageLayout outputLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

#endif  // !LOAD_DDS_HPP
