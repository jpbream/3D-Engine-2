#include "Surface.h"
#include <memory>
#include <SDL.h>
#include "STB_Image_Wrapper.h"

Surface::Surface(int width, int height) : width(width), height(height) {

	pPixels = new int[width * height];
	BlackOut();

	pitch = width * 4;
	aMask = 0xff000000;
	bMask = 0x00ff0000;
	gMask = 0x0000ff00;
	rMask = 0x000000ff;

	std::cout << "Allocating " << GetAllocationString() << " for Surface." << std::endl;

}
Surface::Surface(const Surface& surface)
	: 
	width(surface.width), height(surface.height), pitch(surface.pitch), 
	rMask(surface.rMask), gMask(surface.gMask), bMask(surface.bMask), aMask(surface.aMask) 
{

	if (pPixels != nullptr)
		delete[] pPixels;

	pPixels = new int[width * height];
	memcpy((void*)pPixels, (void*)surface.pPixels, GetBufferSize());

	std::cout << "Allocating " << GetAllocationString() << " for Surface." << std::endl;

}
Surface::Surface(Surface&& surface) noexcept :
	width(surface.width), height(surface.height), pitch(surface.pitch),
	rMask(surface.rMask), gMask(surface.gMask), bMask(surface.bMask), aMask(surface.aMask), pPixels(surface.pPixels)
{

	surface.pPixels = nullptr;
}

Surface::Surface(const std::string& filename) {

	pPixels = (int*)STBI::Load(filename, &width, &height, BPP);

	pitch = width * 4;
	aMask = 0xff000000;
	bMask = 0x00ff0000;
	gMask = 0x0000ff00;
	rMask = 0x000000ff;

	std::cout << "Allocating " << GetAllocationString() << " for Surface." << std::endl;

}

Surface& Surface::operator=(const Surface& surface) {

	width = surface.width;
	height = surface.height;
	pitch = surface.pitch;
	rMask = surface.rMask;
	gMask = surface.gMask;
	bMask = surface.bMask;
	aMask = surface.aMask;

	if (pPixels != nullptr)
		delete[] pPixels;

	pPixels = new int[width * height];
	memcpy((void*)pPixels, (void*)surface.pPixels, GetBufferSize());

	return *this;

}
Surface& Surface::operator=(Surface&& surface) noexcept {

	width = surface.width;
	height = surface.height;
	rMask = surface.rMask;
	gMask = surface.gMask;
	bMask = surface.bMask;
	aMask = surface.aMask;
	pPixels = surface.pPixels;

	surface.pPixels = nullptr;

	return *this;

}

Surface::~Surface() {

	std::cout << "Freeing " << GetAllocationString() << " for Surface" << std::endl;

	if (pPixels != nullptr)
		delete[] pPixels;

	// do not call delete mip maps
	if (mipMap != nullptr)
		delete mipMap;
}

int Surface::GetWidth() const {
	return width;
}

int Surface::GetHeight() const {
	return height;
}

const int* Surface::GetPixels() const {
	return (const int*)pPixels;
}

int Surface::GetPitch() const {
	return pitch;
}

int Surface::GetRMask() const {
	return rMask;
}

int Surface::GetGMask() const {
	return gMask;
}

int Surface::GetBMask() const {
	return bMask;
}

int Surface::GetAMask() const {
	return aMask;
}

int Surface::GetBufferSize() const {

	return sizeof(int) * width * height;

}

void Surface::SetColorMasks(int aMask, int rMask, int gMask, int bMask) {

	this->rMask = rMask;
	this->gMask = gMask;
	this->bMask = bMask;
	this->aMask = aMask;

}

void Surface::SaveToFile(const std::string& filename) const {

	SDL_Surface* surface = SDL_CreateRGBSurfaceFrom((void*)pPixels, width, height, BPP, pitch, rMask, gMask, bMask, aMask);
	SDL_SaveBMP(surface, filename.c_str());
	SDL_FreeSurface(surface);

}

void Surface::Resize(int width, int height) {

}

void Surface::WhiteOut() {

	memset(pPixels, -1, GetBufferSize());

}

void Surface::BlackOut() {

	memset(pPixels, 0, GetBufferSize());

}

void Surface::DrawLine(int x1, int y1, int x2, int y2, int rgb) {

	int dx = x2 - x1;
	int dy = y2 - y1;

	if (dx == 0 && dy == 0) {
		return;
	}

	if (abs(dy) > abs(dx)) {

		double slope = (double)dx / dy;

		int inc = (y2 - y1) / abs(y2 - y1);

		for (int y = y1; y != y2; y += inc) {
			int x = (int)(x1 + (y - y1) * slope);

			PutPixel(x, y, rgb);
		}

	}
	else {

		double slope = (double)dy / dx;

		int inc = (x2 - x1) / abs(x2 - x1);

		for (int x = x1; x != x2; x += inc) {
			int y = (int)(y1 + (x - x1) * slope);

			PutPixel(x, y, rgb);
		}

	}

}

void Surface::GenerateMipMaps() {

	if (mipMap != nullptr)
		return;

	int mmWidth = width / 2;
	int mmHeight = height / 2;

	if (mmWidth < 1 || mmHeight < 1)
		return;

	mipMap = new Surface(mmWidth, mmHeight);

	for (int r = 0; r < mmWidth; ++r) {
		for (int c = 0; c < mmHeight; ++c) {

			const Vec4& c1 = GetPixel(r * 2, c * 2);
			const Vec4& c2 = GetPixel(r * 2 + 1, c * 2);
			const Vec4& c3 = GetPixel(r * 2, c * 2 + 1);
			const Vec4& c4 = GetPixel(r * 2 + 1, c * 2 + 1);

			Vec4 avg = (c1 + c2 + c3 + c4) / 4;
			mipMap->PutPixel(r, c, avg);

		}
	}

	mipMap->GenerateMipMaps();
}

const Surface* Surface::GetMipMap(int level) const {

	if (level <= 0 || mipMap == nullptr)
		return this;

	return mipMap->GetMipMap(level - 1);
}

void Surface::FlipHorizontally() {

	// loop halfway across the image, column by column
	for (int c = 0; c < width / 2; ++c) {

		// loop down the column, row by row
		for (int r = 0; r < height; ++r) {

			// swap the pixels at r, c and r, width - c - 1

			int temp = pPixels[width * r + c];
			PutPixel(c, r, pPixels[width * r + (width - c - 1)]);
			PutPixel(width - c - 1, r, temp);

		}

	}

	// apply transformation to any mip maps
	if (mipMap != nullptr)
		mipMap->FlipHorizontally();

}

void Surface::FlipVertically() {

	// loop halway down the image, row by row
	for (int r = 0; r < height / 2; ++r) {

		// loop across the image, column by column
		for (int c = 0; c < width; ++c) {

			// swap the pixels at r, c and height - r - 1, c

			int temp = pPixels[width * r + c];
			PutPixel(c, r, pPixels[width * (height - r - 1) + c]);
			PutPixel(c, height - r - 1, temp);


		}

	}

	// apply transformation to any mip maps
	if (mipMap != nullptr)
		mipMap->FlipVertically();

}

void Surface::RotateRight() {

	int* newBuf = new int[height * width];

	int newHeight = width;
	int newWidth = height;

	// loop through each row of the original image
	for (int i = 0; i < height; ++i) {

		// loop through each pixel in that row
		for (int j = 0; j < width; ++j) {

			// fill the column in the new buffer with this row,
			// starting from the last column 
			newBuf[newWidth * i + newHeight - j - 1] = pPixels[width * j + i];

		}
	}

	std::swap(newBuf, pPixels);
	delete[] newBuf;

	width = newWidth;
	height = newHeight;
	pitch = width * 4;

	// apply transformation to any mip maps
	if (mipMap != nullptr)
		mipMap->RotateRight();

}

void Surface::RotateLeft() {

	int* newBuf = new int[height * width];

	int newHeight = width;
	int newWidth = height;

	// loop through each row of the original image
	for (int i = 0; i < height; ++i) {

		// loop through each pixel in that row
		for (int j = 0; j < width; ++j) {

			// fill the column in the new buffer with this row,
			// starting from the bottom of the first column 
			newBuf[newHeight * (newHeight - i - 1) + j] = pPixels[width * j + i];

		}
	}

	std::swap(newBuf, pPixels);
	delete[] newBuf;

	width = newWidth;
	height = newHeight;
	pitch = width * 4;

	// apply transformation to any mip maps
	if (mipMap != nullptr)
		mipMap->RotateLeft();

}

void Surface::DeleteMipMaps() {

	if (mipMap != nullptr) {

		// delete the mip maps mip map, if it exists
		if (mipMap->mipMap != nullptr)
			mipMap->DeleteMipMaps();

		// delete the mip map
		delete mipMap;

	}
	

}

std::string Surface::GetAllocationString() const {

	int size = GetBufferSize();

	if (size > 1000000) {
		return std::to_string(size / (1 << 20)) + " MB";
	}
	if (size > 1000) {
		return std::to_string(size / (1 << 10)) + " KB";
	}
	return std::to_string(size) + " B";

}
