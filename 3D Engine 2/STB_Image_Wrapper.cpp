#define STB_IMAGE_IMPLEMENTATION
#include <STB/stb_image.h>
#include "STB_Image_Wrapper.h"
#include <iostream>

namespace STBI {

	unsigned char* Load(const std::string& filename, int* width, int* height, int bpp) {

		int n = 0;

		unsigned char* p = stbi_load(filename.c_str(), width, height, &n, bpp / 8);

		if (p == nullptr) {
			std::cout << "Error loading " << filename << std::endl;
			return nullptr;
		}
	

		return p;

	}

}