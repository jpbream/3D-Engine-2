#pragma once
#include <string>

namespace STBI {

	unsigned char* Load(const std::string& filename, int* width, int* height, int bpp);

}