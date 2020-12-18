#include "TextureLoaderFrontend.hpp"

#ifdef TEXTURELOADERFRONTEND_USE_STB
#	define STB_IMAGE_IMPLEMENTATION 1
#	include "stb_image.h"
#endif

#ifdef TEXTURELOADERFRONTEND_USE_TEXAS
#	include <Texas/Texas.hpp>
#	include <Texas/VkTools.hpp>
#endif

#ifdef TEXTURELOADERFRONTEND_USE_ASTC_CODEC
#	include <astc-codec/astc-codec.h>
#endif

#ifdef TEXTURELOADERFRONTEND_USE_LOAD_DDS
#	include "load_dds.hpp"
#endif

namespace tlf
{
Texture TextureLoader::Load(std::filesystem::path& path)
{
	auto extension = path.extension();

	Texture tex;

#ifdef TEXTURELOADERFRONTEND_USE_STB
	if (extension == "png")
	{

	}
#endif

#ifdef TEXTURELOADERFRONTEND_USE_TEXAS
#endif

#ifdef TEXTURELOADERFRONTEND_USE_ASTC_CODEC
#endif

	return tex;
}
}
