#include "IrradianceMap.hpp"

#include "EquirectangularToIrradianceMap.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <iostream>

namespace cdm
{
IrradianceMap::IrradianceMap(RenderWindow& renderWindow, uint32_t resolution,
                             Texture2D& equirectangularTexture,
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
			is >> infoResolution;

			m_irradianceMap = Cubemap(
			    renderWindow, infoResolution, infoResolution,
			    VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
			    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			    VMA_MEMORY_USAGE_GPU_ONLY,
			    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			std::string inFileName;
			for (uint32_t layer = 0; layer < 6; layer++)
			{
				inFileName = cacheFilePath.string() + ".layer" +
				             std::to_string(layer) + ".hdr";

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
					copy.imageSubresource.mipLevel = 0;

					m_irradianceMap.uploadDataImmediate(
					    imageData, w * h * 4 * sizeof(float), copy, layer,
					    VK_IMAGE_LAYOUT_UNDEFINED,
					    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

					stbi_image_free(imageData);
				}
			}

			return;
		}
	}

	EquirectangularToIrradianceMap e2i(renderWindow, resolution);
	m_irradianceMap = e2i.computeCubemap(equirectangularTexture);

	std::filesystem::create_directory(cacheDirPath);
	
	std::string outFileName;
	for (uint32_t layer = 0; layer < 6; layer++)
	{
		std::vector<float> texels =
		    m_irradianceMap.downloadDataImmediate<float>(
		        layer, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		outFileName =
		    cacheFilePath.string() + ".layer" + std::to_string(layer) + ".hdr";

		stbi_write_hdr(outFileName.c_str(), m_irradianceMap.width(),
		               m_irradianceMap.height(), 4, texels.data());
	}

	std::ofstream os(cacheInfoFilePath,
	                 std::ofstream::out | std::ofstream::trunc);
	os << m_irradianceMap.width();
	os << " ";
}
}  // namespace cdm
