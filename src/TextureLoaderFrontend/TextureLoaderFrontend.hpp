#ifndef TEXTURELOADERFRONTEND_HPP
#define TEXTURELOADERFRONTEND_HPP 1

#include <vector>
#include <filesystem>

namespace tlf
{
struct Texture
{
	std::vector<std::byte> data;
	size_t width;
	size_t height;
	size_t depth;
	size_t layerCount;
	size_t mipLevels;
	size_t componentCount;
};

class TextureLoader
{
public:
	static Texture Load(std::filesystem::path& path);
};
}

#endif  // !TEXTURELOADERFRONTEND_HPP
