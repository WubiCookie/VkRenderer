#include "load_dds.hpp"

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

//#define MEAN_AND_LEAN
//#define NO_MINMAX
//#include <windows.h>

#include <iostream>

/*
Can load easier and more indepth with https://github.com/Hydroque/DDSLoader
Because a lot of crappy, weird DDS file loader files were found online. The
resources are actually VERY VERY limited. Written in C, can very easily port to
C++ through casting mallocs (ensure your imports are correct), goto can be
replaced.
https://www.gamedev.net/forums/topic/637377-loading-dds-textures-in-opengl-black-texture-showing/
http://www.opengl-tutorial.org/beginners-tutorials/tutorial-5-a-textured-cube/
^ Two examples of terrible code
https://gist.github.com/Hydroque/d1a8a46853dea160dc86aa48618be6f9
^ My first look and clean up 'get it working'
https://ideone.com/WoGThC
^ Improvement details
File Structure:
  Section     Length
  ///////////////////
  FILECODE    4
  HEADER      124
  HEADER_DX10* 20
(https://msdn.microsoft.com/en-us/library/bb943983(v=vs.85).aspx) PIXELS
fseek(f, 0, SEEK_END); (ftell(f) - 128) - (fourCC == "DX10" ? 17 or 20 : 0)
* the link tells you that this section isn't written unless its a DX10 file
Supports DXT1, DXT3, DXT5.
The problem with supporting DX10 is you need to know what it is used for and
how opengl would use it. File Byte Order: typedef unsigned int DWORD; // 32bits
little endian type   index    attribute           // description
///////////////////////////////////////////////////////////////////////////////////////////////
  DWORD  0        file_code;          //. always `DDS `, or 0x20534444
  DWORD  4        size;               //. size of the header, always 124
(includes PIXELFORMAT) DWORD  8        flags;              //. bitflags that
tells you if data is present in the file
                                      //      CAPS         0x1
                                      //      HEIGHT       0x2
                                      //      WIDTH        0x4
                                      //      PITCH        0x8
                                      //      PIXELFORMAT  0x1000
                                      //      MIPMAPCOUNT  0x20000
                                      //      LINEARSIZE   0x80000
                                      //      DEPTH        0x800000
  DWORD  12       height;             //. height of the base image (biggest
mipmap) DWORD  16       width;              //. width of the base image
(biggest mipmap) DWORD  20       pitchOrLinearSize;  //. bytes per scan line in
an uncompressed texture, or bytes in the top level texture for a compressed
texture
                                      //     D3DX11.lib and other similar
libraries unreliably or inconsistently provide the pitch, convert with
                                      //     DX* && BC*: max( 1, ((width+3)/4)
) * block-size
                                      //     *8*8_*8*8 && UYVY && YUY2:
((width+1) >> 1) * 4
                                      //     (width * bits-per-pixel + 7)/8
(divide by 8 for byte alignment, whatever that means) DWORD  24       depth;
//. Depth of a volume texture (in pixels), garbage if no volume data DWORD  28
mipMapCount;        //. number of mipmaps, garbage if no pixel data DWORD  32
reserved1[11];      //. unused DWORD  76       Size;               //. size of
the following 32 bytes (PIXELFORMAT) DWORD  80       Flags;              //.
bitflags that tells you if data is present in the file for following 28 bytes
                                      //      ALPHAPIXELS  0x1
                                      //      ALPHA        0x2
                                      //      FOURCC       0x4
                                      //      RGB          0x40
                                      //      YUV          0x200
                                      //      LUMINANCE    0x20000
  DWORD  84       FourCC;             //. File format: DXT1, DXT2, DXT3, DXT4,
DXT5, DX10. DWORD  88       RGBBitCount;        //. Bits per pixel DWORD  92
RBitMask;           //. Bit mask for R channel DWORD  96       GBitMask; //.
Bit mask for G channel DWORD  100      BBitMask;           //. Bit mask for B
channel DWORD  104      ABitMask;           //. Bit mask for A channel DWORD
108      caps;               //. 0x1000 for a texture w/o mipmaps
                                      //      0x401008 for a texture w/ mipmaps
                                      //      0x1008 for a cube map
  DWORD  112      caps2;              //. bitflags that tells you if data is
present in the file
                                      //      CUBEMAP           0x200 Required
for a cube map.
                                      //      CUBEMAP_POSITIVEX 0x400 Required
when these surfaces are stored in a cube map.
                                      //      CUBEMAP_NEGATIVEX 0x800     ^
                                      //      CUBEMAP_POSITIVEY 0x1000    ^
                                      //      CUBEMAP_NEGATIVEY 0x2000    ^
                                      //      CUBEMAP_POSITIVEZ 0x4000    ^
                                      //      CUBEMAP_NEGATIVEZ 0x8000    ^
                                      //      VOLUME            0x200000
Required for a volume texture. DWORD  114      caps3;              //. unused
  DWORD  116      caps4;              //. unused
  DWORD  120      reserved2;          //. unused
*/

struct PixelFormatHeader
{
	enum class FlagBits : uint32_t
	{
		AlphaPixels = 0x1,
		Alpha = 0x2,
		FourCC = 0x4,
		Rgb = 0x40,
		Yuv = 0x200,
		Luminance = 0x20000,
	};

	uint32_t dwSize;
	FlagBits dwFlags;
	char dwFourCC[4];
	uint32_t dwRGBBitCount;
	uint32_t dwRBitMask;
	uint32_t dwGBitMask;
	uint32_t dwBBitMask;
	uint32_t dwABitMask;
};

struct Header
{
	enum class FlagBits : uint32_t
	{
		Caps = 0x1,
		Height = 0x2,
		Width = 0x4,
		Pitch = 0x8,
		PixelFormat = 0x1000,
		MipmapCount = 0x20000,
		LinearSize = 0x80000,
		Depth = 0x800000,
		Texture = Caps | Height | Width | PixelFormat,
		Mipmap = MipmapCount,
		Volume = Depth,
	};

	enum class Caps_bits : uint32_t
	{
		Complex = 0x8,
		Mipmap = 0x400000,
		Texture = 0x1000,
	};

	enum class Caps2_bits : uint32_t
	{
		Cubemap = 0x200,
		CubemapPositiveX = 0x400,
		CubemapNegativeX = 0x800,
		CubemapPositiveY = 0x1000,
		CubemapNegativeY = 0x2000,
		CubemapPositiveZ = 0x4000,
		CubemapNegativeZ = 0x8000,
		Volume = 0x200000,
		CubemapAllFaces = CubemapPositiveX | CubemapNegativeX |
		                  CubemapPositiveY | CubemapNegativeY |
		                  CubemapPositiveZ | CubemapNegativeZ,
	};

	char fourCC[4];
	FlagBits dwSize;
	uint32_t dwFlags;
	uint32_t dwHeight;
	uint32_t dwWidth;
	uint32_t dwPitchOrLinearSize;
	uint32_t dwDepth;
	uint32_t dwMipMapCount;
	uint32_t dwReserved1[11];
	PixelFormatHeader ddspf;
	Caps_bits dwCaps;
	Caps2_bits dwCaps2;
	uint32_t dwCaps3;
	uint32_t dwCaps4;
	uint32_t dwReserved2;
};

static_assert(sizeof(Header) == 128);

cdm::Texture2D texture_loadDDS(const char* path, cdm::TextureFactory& factory,
                               cdm::CommandBufferPool& pool,
                               VkImageLayout outputLayout)
{
	// lay out variables to be used
	Header header;
	memset(&header, 0xcccc, sizeof(header));

	uint32_t width;
	uint32_t height;
	uint32_t mipmapCount;

	uint32_t blockSize;
	VkFormat format;

	uint32_t w;
	uint32_t h;

	std::vector<std::byte> buffer;

	// GLuint tid = 0;
	cdm::Texture2D texture;

	// open the DDS file for binary reading and get file size
	FILE* f;
	if ((f = fopen(path, "rb")) == 0)
		return texture;
	fseek(f, 0, SEEK_END);
	long file_size = ftell(f);
	fseek(f, 0, SEEK_SET);

	// allocate new unsigned char space with 4 (file code) + 124 (header size)
	// bytes read in 128 bytes from the file
	fread(&header, 1, sizeof(header), f);

	// compare the `DDS ` signature
	if (memcmp(&header, "DDS ", 4) != 0)
		goto exit;

	//*

	// extract height, width, and amount of mipmaps - yes it is stored height
	// then width
	height = header.dwHeight;
	width = header.dwWidth;
	mipmapCount = header.dwMipMapCount;

	factory.setWidth(width);
	factory.setHeight(height);
	// factory.setMipLevels(mipmapCount);
	factory.setMipLevels(1);

	// figure out what format to use for what fourCC file type it is
	// block size is about physical chunk storage of compressed data in file
	// (important)
	if (char(header.ddspf.dwFourCC[0]) == 'D')
	{
		switch (char(header.ddspf.dwFourCC[3]))
		{
		case '1':  // DXT1
			format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
			blockSize = 8;
			break;
		case '3':  // DXT3
			format = VK_FORMAT_BC3_UNORM_BLOCK;
			blockSize = 16;
			break;
		case '5':  // DXT5
			format = VK_FORMAT_BC5_UNORM_BLOCK;
			blockSize = 16;
			break;
		case '0':  // DX10
		           // unsupported, else will error
		           // as it adds sizeof(struct DDS_HEADER_DXT10) between pixels
		           // so, buffer = malloc((file_size - 128) - sizeof(struct
		           // DDS_HEADER_DXT10));
		default: goto exit;
		}
	}
	else  // BC4U/BC4S/ATI2/BC55/R8G8_B8G8/G8R8_G8B8/UYVY-packed/YUY2-packed
	      // unsupported
		goto exit;

	factory.setFormat(format);

	// allocate new unsigned char space with file_size - (file_code +
	// header_size) magnitude read rest of file
	buffer.resize(file_size - 128);
	// buffer = (unsigned char*)malloc(file_size - 128);
	// if(buffer == 0)
	//	goto exit;
	fread(buffer.data(), 1, file_size, f);

	// prepare new incomplete texture
	// glGenTextures(1, &tid);
	// if(tid == 0)
	//	goto exit;

	// texture = factory.createTexture2D();

	// bind the texture
	// make it complete by specifying all needed parameters and ensuring all
	// mipmaps are filled
	// glBindTexture(GL_TEXTURE_2D, tid);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipMapCount-1); //
	// opengl likes array length of mipmaps 	glTexParameteri(GL_TEXTURE_2D,
	// GL_TEXTURE_MAG_FILTER, GL_LINEAR); 	glTexParameteri(GL_TEXTURE_2D,
	// GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // don't forget to
	// enable mipmaping 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
	// GL_REPEAT); 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
	// GL_REPEAT);

	// prepare some variables
	unsigned int offset = 0;
	unsigned int size = 0;
	w = width;
	h = height;

	// loop through sending block at a time with the magic formula
	// upload to opengl properly, note the offset transverses the pointer
	// assumes each mipmap is 1/2 the size of the previous mipmap

	//std::cout << path << " has " << mipmapCount << " mipLevels" << std::endl;
	std::cout << path << std::endl;

	for (unsigned int i = 0; i < mipmapCount; i++)
	{
		if (w == 0 || h == 0)
		{  // discard any odd mipmaps 0x1 0x2 resolutions
			std::cout << "odd mipmap " << i << "/" << mipmapCount << " (" << w
			          << ";" << h << ")" << std::endl;
			// mipmapCount--;
			// continue;
		}
		size = ((w + 3) / 4) * ((h + 3) / 4) * blockSize;

		VkBufferImageCopy region{};
		region.imageExtent.width = w;
		region.imageExtent.height = h;
		region.imageExtent.depth = 1;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageSubresource.mipLevel = i;

		if (i == mipmapCount - 2)
		{
			std::cout << "loading level " << i << "/" << mipmapCount << " ("
			          << w << ";" << h << ")" << std::endl;
			factory.setWidth(w);
			factory.setHeight(h);
			texture = factory.createTexture2D();
			region.imageSubresource.mipLevel = 0;
			texture.uploadData(buffer.data() + offset, size, region,
			                   VK_IMAGE_LAYOUT_UNDEFINED, outputLayout, pool);

			if (texture == nullptr)
			{
				std::cout << "what" << std::endl;
			}

			break;
		}
		// glCompressedTexImage2D(GL_TEXTURE_2D, i, format, w, h, 0, size,
		// buffer + offset);

		offset += size;
		w /= 2;
		h /= 2;
	}

	if (texture.image() == nullptr)
	{
		VkBufferImageCopy region{};
		region.imageExtent.width = width;
		region.imageExtent.height = height;
		region.imageExtent.depth = 1;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageSubresource.mipLevel = 0;
		std::cout << "loading level " << 0 << "/" << mipmapCount << " ("
		          << width << ";" << height << ")" << std::endl;

		size = ((width + 3) / 4) * ((width + 3) / 4) * blockSize;
		//size = width  * width  * blockSize;

		factory.setWidth(width);
		factory.setHeight(height);
		texture = factory.createTexture2D();
		texture.uploadData(buffer.data(), size, region,
		                   VK_IMAGE_LAYOUT_UNDEFINED, outputLayout, pool);
	}

	// discard any odd mipmaps, ensure a complete texture
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipMapCount-1);
	// unbind
	// glBindTexture(GL_TEXTURE_2D, 0);

	//*/

	// easy macro to get out quick and uniform (minus like 15 lines of bulk)
exit:
	// free(buffer);
	// free(header);
	fclose(f);
	// return tid;
	return texture;
}
