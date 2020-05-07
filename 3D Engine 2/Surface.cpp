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

void Surface::Resize(int width, int height, bool maintainImage) {

	if (width * height > this->width* this->height || maintainImage) {

		// if there is a need to reallocate or they want to maintain the image

		int* newBuf = new int[width * height];
		
		if (maintainImage) {

			// the original image should be maintained during the resize

			int rowMin = height < this->height ? height : this->height;
			int colMin = width < this->width ? width : this->width;

			for (int row = 0; row < rowMin; ++row) {

				int* startOfBufferRow = newBuf + width * row;
				int* startOfThisRow = pPixels + this->width * row;

				// copy row by row
				memcpy(startOfBufferRow, startOfThisRow, colMin * sizeof(int));

			}

		}
		else {

			// they don't want to maintain the image,
			// just copy everything over
			memcpy(newBuf, pPixels, GetBufferSize());
		}

		std::swap(newBuf, pPixels);
		delete[] newBuf;
	}

	this->width = width;
	this->height = height;
	pitch = width * 4;

}

void Surface::Rescale(float xScale, float yScale) {

	if (xScale <= 0 || yScale <= 0)
		return;

	int newWidth = width * xScale;
	int newHeight = height * yScale;

	int* newBuf = new int[newWidth * newHeight];

	for (int row = 0; row < newHeight; ++row) {

		int sampleY = (float)row / newHeight * height;

		for (int col = 0; col < newWidth; ++col) {

			int sampleX = (float)col / newWidth * width;

			newBuf[row * newWidth + col] = pPixels[sampleY * width + sampleX];

		}
	}

	std::swap(pPixels, newBuf);
	delete[] newBuf;

	width = newWidth;
	height = newHeight;
	pitch = width * 4;


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

void Surface::Tint(const Vec4& target, float alpha)
{

	if (alpha < 0)
		alpha = 0;
	if (alpha > 1)
		alpha = 1;

	for (int* traveler = pPixels; traveler < pPixels + width * height; ++traveler) {

		Vec4 tint = EXPAND4(*traveler) * (1 - alpha) + target * alpha;
		*traveler = COMPRESS4(tint);

	}

	//apply operation to mip map as well
	if (mipMap != nullptr)
		mipMap->Tint(target, alpha);
}

static float SampleGaussianFunction(int x, float stdDev) {

	// Equation from Wikipedia: Gaussian Blur

	float c1 = 1 / sqrt(2 * PI * stdDev * stdDev);
	float c2 = exp(-x * x / (2 * stdDev * stdDev));

	return c1 * c2;

}

void Surface::GaussianBlur(int kernelSize, float stdDev, int blurType) {

	if (kernelSize <= 0)
		return;

	if (stdDev <= 0)
		return;

	float* weights = new float[kernelSize];
	float sumWeights = 0;

	for (int w = 0; w < kernelSize; ++w) {

		// calculate the weights
		weights[w] = SampleGaussianFunction(w - (kernelSize / 2), stdDev);
		sumWeights += weights[w];

	}

	// renormalize the weights so they sum to 1
	for (float* traveler = weights; traveler < weights + kernelSize; ++traveler)
		*traveler /= sumWeights;

	// do vertical blur
	if (blurType == BLUR_VERTICAL || blurType == BLUR_BOTH) {

		int* blurredImage = new int[width * height];

		for (int r = 0; r < height; ++r) {
			for (int c = 0; c < width; ++c) {

				Vec4 weightedAverage = {};

				for (int w = 0; w < kernelSize; ++w) {

					int offset = w - (kernelSize / 2);

					// if this offset takes us off the image, ignore this pixel
					// NOTE: this makes the edges appear darker
					if (r + offset < 0 || r + offset > height - 1)
						continue;

					weightedAverage += EXPAND4(pPixels[width * (r + offset) + c]) * weights[w];

				}

				blurredImage[width * r + c] = COMPRESS4(weightedAverage);
			}
		}

		std::swap(pPixels, blurredImage);
		delete[] blurredImage;
	}

	// do horizontal blur
	if (blurType == BLUR_HORIZONTAL || blurType == BLUR_BOTH) {

		int* blurredImage = new int[width * height];

		for (int r = 0; r < height; ++r) {
			for (int c = 0; c < width; ++c) {

				Vec4 weightedAverage = {};

				for (int w = 0; w < kernelSize; ++w) {

					int offset = w - (kernelSize / 2);

					// if this offset takes us off the image, ignore this pixel
					// NOTE: this makes the edges appear darker
					if (c + offset < 0 || c + offset > width - 1)
						continue;

					weightedAverage += EXPAND4(pPixels[width * r + c + offset]) * weights[w];

				}

				blurredImage[width * r + c] = COMPRESS4(weightedAverage);
			}
		}

		std::swap(pPixels, blurredImage);
		delete[] blurredImage;
	}
	delete[] weights;

	// do operation on mip maps with half the kernel size
	if (mipMap != nullptr)
		mipMap->GaussianBlur(kernelSize / 2, stdDev, blurType);

}

void Surface::Invert() {

	for (int* traveler = pPixels; traveler < pPixels + width * height; ++traveler) {

		Vec4 old = EXPAND4(*traveler);
		old.r = 1 - old.r;
		old.g = 1 - old.g;
		old.b = 1 - old.b;
		*traveler = COMPRESS4(old);

	}

}

void Surface::SetContrast(float contrast) {

	for (int* traveler = pPixels; traveler < pPixels + width * height; ++traveler) {

		Vec4 old = EXPAND4(*traveler);
		
		old.r -= 0.5;
		old.r *= (1 + contrast);
		old.r += 0.5;
		if (old.r > 1)
			old.r = 1;
		if (old.r < 0)
			old.r = 0;

		old.g -= 0.5;
		old.g *= (1 + contrast);
		old.g += 0.5;
		if (old.g > 1)
			old.g = 1;
		if (old.g < 0)
			old.g = 0;

		old.b -= 0.5;
		old.b *= (1 + contrast);
		old.b += 0.5;
		if (old.b > 1)
			old.b = 1;
		if (old.b < 0)
			old.b = 0;


		*traveler = COMPRESS4(old);

	}

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
