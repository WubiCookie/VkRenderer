#include "PrefilteredCubemap.hpp"

#include "PrefilterCubemap.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace cdm
{
PrefilteredCubemap::PrefilteredCubemap(RenderWindow& renderWindow,
                                       uint32_t resolution, uint32_t mipLevels,
                                       Cubemap& cubemap,
                                       std::string_view filePath)
{
	auto& vk = renderWindow.device();

	using namespace std::literals;
	namespace fs = std::filesystem;

	fs::path path = filePath;
	fs::path cacheDirPath = fs::temp_directory_path() / "VkRenderer";
	fs::path cacheFilePath = cacheDirPath / path.filename();
	fs::path cacheInfoFilePath =
	    cacheDirPath / (path.filename().string() + ".info"s);

	if (fs::exists(cacheInfoFilePath))
	{
		std::ifstream is(cacheInfoFilePath);

		if (is.is_open())
		{
			uint32_t infoResolution;
			uint32_t infoMipLevels;
			is >> infoResolution;
			is >> infoMipLevels;

			m_prefilteredCubemap = Cubemap(
			    renderWindow, infoResolution, infoResolution,
			    VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
			    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			    VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			    infoMipLevels);

			std::string inFileName;
			for (uint32_t mipLevel = 0; mipLevel < infoMipLevels; mipLevel++)
			{
				for (uint32_t layer = 0; layer < 6; layer++)
				{
					inFileName = cacheFilePath.string() + ".layer" +
					             std::to_string(layer) + ".mipLevel" +
					             std::to_string(mipLevel) + ".hdr";

					int w, h, c;
					float* imageData =
					    stbi_loadf(inFileName.c_str(), &w, &h, &c, 4);

					if (imageData && w == h)
					{
						VkBufferImageCopy copy{};
						copy.bufferRowLength = w;
						copy.bufferImageHeight = h;
						copy.imageExtent.width = w;
						copy.imageExtent.height = h;
						copy.imageExtent.depth = 1;
						copy.imageSubresource.aspectMask =
						    VK_IMAGE_ASPECT_COLOR_BIT;
						copy.imageSubresource.baseArrayLayer = layer;
						copy.imageSubresource.layerCount = 1;
						copy.imageSubresource.mipLevel = mipLevel;

						m_prefilteredCubemap.uploadDataImmediate(
						    imageData, w * h * 4 * sizeof(float), copy, layer,
						    VK_IMAGE_LAYOUT_UNDEFINED,
						    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

						stbi_image_free(imageData);
					}
				}
			}

			return;
		}
	}

	PrefilterCubemap pcm(renderWindow, resolution, mipLevels);
	m_prefilteredCubemap = pcm.computeCubemap(cubemap);

	std::filesystem::create_directory(cacheDirPath);

	std::string outFileName;
	for (uint32_t mipLevel = 0; mipLevel < m_prefilteredCubemap.mipLevels();
	     mipLevel++)
	{
		for (uint32_t layer = 0; layer < 6; layer++)
		{
			std::vector<float> texels =
			    m_prefilteredCubemap.downloadDataImmediate<float>(
			        layer, mipLevel, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			outFileName = cacheFilePath.string() + ".layer" +
			              std::to_string(layer) + ".mipLevel" +
			              std::to_string(mipLevel) + ".hdr";

			stbi_write_hdr(
			    outFileName.c_str(),
			    m_prefilteredCubemap.width() * std::pow(0.5, mipLevel),
			    m_prefilteredCubemap.height() * std::pow(0.5, mipLevel), 4,
			    texels.data());
		}
	}

	std::ofstream os(cacheInfoFilePath,
	                 std::ofstream::out | std::ofstream::trunc);
	os << m_prefilteredCubemap.width();
	os << " ";
	os << m_prefilteredCubemap.mipLevels();
	os << " ";
}
}  // namespace cdm
