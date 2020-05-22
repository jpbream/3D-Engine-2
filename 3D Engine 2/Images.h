#pragma once
#include <string>

// WRAPPER FOR THE STB_IMAGE LIBRARY

namespace Images {

	unsigned char* Load(const std::string& filename, int* width, int* height, int bpp);

}