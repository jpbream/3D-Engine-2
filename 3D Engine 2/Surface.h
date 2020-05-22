#pragma once
#include <string>
#include <iostream>
#include "Vec4.h"
#include "Vec3.h"

#define PI 3.14159265358979323846

// returns an integer color value from a Vec4
#define COMPRESS4(v) ((int)(v.r * 255) | (int)(v.g * 255) << 8 | (int)(v.b * 255) << 16 | (int)(v.a * 255) << 24)

// returns an integer color value from a Vec3
#define COMPRESS3(v) ((int)(v.r * 255) | (int)(v.g * 255) << 8 | (int)(v.b * 255) << 16 | (unsigned char)-1 << 24)

// returns a Vec4 from an integer color value
#define EXPAND4(i) Vec4((i & rMask) / 255.0f, ((i & gMask) >> 8) / 255.0f, ((i & bMask) >> 16) / 255.0f, ((i & aMask) >> 24) / 255.0f )

// returns a Vec3 from an integer color value
#define EXPAND3(i) Vec3((i & rMask) / 255.0f, ((i & gMask) >> 8) / 255.0f, ((i & bMask) >> 16) / 255.0f )

class Surface
{
private:
	int* pPixels;
	int width;
	int height;

	int allocatedSpace;

	int pitch;
	
	unsigned int aMask;
	unsigned int rMask;
	unsigned int gMask;
	unsigned int bMask;

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

	void Resize(int width, int height, bool maintainImage);
	void Rescale(float xScale, float yScale);
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

	void Tint(const Vec4& target, float alpha);

	enum {
		BLUR_VERTICAL,
		BLUR_HORIZONTAL,
		BLUR_BOTH
	};

	void GaussianBlur(int kernelSize, float stdDev, int blurType);

	void Invert();
	void SetContrast(float contrast);

	inline const Vec4& GetPixel(int x, int y) const {
	
		int color = pPixels[width * y + x];
		return EXPAND4(color);

	}

	inline void PutPixel(int x, int y, const Vec4& v) {

		pPixels[width * y + x] = COMPRESS4(v);

	}

	inline void PutPixel(int x, int y, const Vec3& v) {

		pPixels[width * y + x] = COMPRESS3(v);

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

