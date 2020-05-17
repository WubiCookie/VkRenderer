#include "BrdfLut.hpp"

#include "BrdfLutGenerator.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

#include <filesystem>

namespace cdm
{
BrdfLut::BrdfLut(RenderWindow& renderWindow, uint32_t resolution,
                 std::string_view filePath)

{
	auto& vk = renderWindow.device();

	using namespace std::literals;
	namespace fs = std::filesystem;

	fs::path path = filePath;
	fs::path cacheDirPath = fs::temp_directory_path() / "VkRenderer";
	fs::path cacheFilePath = cacheDirPath / path.filename();

	if (fs::exists(cacheFilePath))
	{
		int w, h, c;
		float* imageData =
		    stbi_loadf(cacheFilePath.string().c_str(), &w, &h, &c, 2);

		if (imageData && w == h)
		{
			m_brdfLut = Texture2D(
			    renderWindow, w, h, VK_FORMAT_R32G32_SFLOAT,
			    VK_IMAGE_TILING_OPTIMAL,
			    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			    VMA_MEMORY_USAGE_GPU_ONLY,
			    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

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

	BrdfLutGenerator blg(renderWindow, resolution);
	m_brdfLut = blg.computeBrdfLut();

	std::filesystem::create_directory(cacheDirPath);

	std::vector<float> texels = m_brdfLut.downloadDataImmediate<float>(
	    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	stbi_write_hdr(cacheFilePath.string().c_str(), m_brdfLut.width(),
	               m_brdfLut.height(), 2, texels.data());
}
}  // namespace cdm
