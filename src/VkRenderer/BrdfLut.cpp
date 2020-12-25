#include "BrdfLut.hpp"

#include "BrdfLutGenerator.hpp"
#include "TextureFactory.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

#include <filesystem>
#include <iostream>

namespace cdm
{
BrdfLut::BrdfLut(RenderWindow& renderWindow, uint32_t resolution,
                 std::string_view filePath)

{
    auto& vk = renderWindow.device();

    using namespace std::literals;
    namespace fs = std::filesystem;

    fs::path path = filePath;
    fs::path cacheDirPath = "../runtime_cache";
    fs::path cacheFilePath = cacheDirPath / path.filename();

    if (fs::exists(cacheFilePath))
    {
        int w, h, c;
        float* imageData =
            stbi_loadf(cacheFilePath.string().c_str(), &w, &h, &c, 2);

        if (imageData && w == h)
        {
            TextureFactory f(vk);

            f.setWidth(w);
            f.setHeight(h);
            f.setFormat(VK_FORMAT_R32G32_SFLOAT);
            f.setUsage(VK_IMAGE_USAGE_SAMPLED_BIT |
                       VK_IMAGE_USAGE_TRANSFER_DST_BIT);

            m_brdfLut = f.createTexture2D();

            VkBufferImageCopy copy{};
            copy.bufferRowLength = w;
            copy.bufferImageHeight = h;
            copy.imageExtent.width = w;
            copy.imageExtent.height = h;
            copy.imageExtent.depth = 1;
            copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copy.imageSubresource.baseArrayLayer = 0;
            copy.imageSubresource.layerCount = 1;
            copy.imageSubresource.mipLevel = 0;

            m_brdfLut.uploadDataImmediate(
                imageData, w * h * 2 * sizeof(float), copy,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            stbi_image_free(imageData);
            return;
        }
    }

    std::cout << "brdfLut not found in \"" << cacheFilePath
              << "\". Generating it, please wait..." << std::endl;

    BrdfLutGenerator blg(renderWindow, resolution);
    m_brdfLut = blg.computeBrdfLut();

    std::filesystem::create_directory(cacheDirPath);

    std::vector<float> texels = m_brdfLut.downloadDataImmediate<float>(
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    stbi_write_hdr(cacheFilePath.string().c_str(), m_brdfLut.width(),
                   m_brdfLut.height(), 2, texels.data());
}
}  // namespace cdm
