#pragma once

#include <stb_image_write.h>

inline int jpeg_save(unsigned char* data, int width, int height, int quality, const char* filename)
{
	return stbi_write_jpg(filename, width, height, 3, data, quality);
}
