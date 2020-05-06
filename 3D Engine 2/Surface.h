#pragma once
#include <string>
#include <iostream>
#include "Vec4.h"
#include "Vec3.h"

class Surface
{
private:
	int* pPixels;
	int width;
	int height;

	int pitch;
	
	unsigned int aMask;
	unsigned int rMask;
	unsigned int gMask;
	unsigned int bMask;

	//int numMipMaps = 0;
	//Surface** mipMaps = nullptr;
	Surface* mipMap = nullptr;

	std::string GetAllocationString() const;

public:

	static constexpr int BPP = 32;

	Surface(int width, int height);
	Surface(const Surface& surface);
	Surface(Surface&& surface) noexcept;
	Surface(const std::string& filename);

	Surface& operator=(const Surface& surface);
	Surface& operator=(Surface&& surface) noexcept;

	~Surface();

	const int* GetPixels() const;
	int GetWidth() const;
	int GetHeight()const;
	int GetPitch() const;
	int GetRMask() const;
	int GetGMask() const;
	int GetBMask() const;
	int GetAMask() const;
	int GetBufferSize() const;

	void Resize(int width, int height);
	void SetColorMasks(int aMask, int rMask, int gMask, int bMask);

	void SaveToFile(const std::string& filename) const;

	void WhiteOut();
	void BlackOut();

	void DrawLine(int x1, int y1, int x2, int y2, int rgb);

	void GenerateMipMaps();
	void DeleteMipMaps();
	const Surface* GetMipMap(int level) const;

	void FlipHorizontally();
	void FlipVertically();
	void RotateRight();
	void RotateLeft();

	inline const Vec4& GetPixel(int x, int y) const {

		int color = pPixels[width * y + x];
		return { (color & rMask) / 255.0f, ((color & gMask) >> 8) / 255.0f, ((color & bMask) >> 16) / 255.0f, ((color & aMask) >> 24) / 255.0f };

	}

	inline void PutPixel(int x, int y, const Vec4& v) {

		pPixels[width * y + x] = (int)(v.r * 255) | (int)(v.g * 255) << 8 | (int)(v.b * 255) << 16 | (int)(v.a * 255) << 24;

	}

	inline void PutPixel(int x, int y, const Vec3& v) {

		pPixels[width * y + x] = (int)(v.r * 255) | (int)(v.g * 255) << 8 | (int)(v.b * 255) << 16 | (unsigned char)-1 << 24;

	}

	inline void PutPixel(int x, int y, int rgb) {

		pPixels[width * y + x] = rgb;

	}

	inline void PutPixel(int x, int y, float grayscale) {

		// memset to the grayscale value * 255, or'd with the Alpha mask so it does not vary in transparency
		memset(pPixels + width * y + x, (unsigned char)(255 * grayscale), sizeof(int));
		pPixels[width * y + x] |= aMask;

	}

};

