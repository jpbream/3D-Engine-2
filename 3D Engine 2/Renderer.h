#pragma once
#include "Surface.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include "Mat4.h"
#include <unordered_map>
#include <thread>
#include <vector>

#ifndef NEAR
#define NEAR 1.0f
#endif

#ifndef FAR
#define FAR 100.0f
#endif

#ifndef _DEBUG
#define THREADS (std::thread::hardware_concurrency() - 2) / 2
#else
#define THREADS 0
#endif

#define X_OFFSET 0
#define Y_OFFSET 1
#define Z_OFFSET 2

#define FLAT_TOP 1
#define FLAT_BOTTOM 2

#define RF_LINEAR_ZBUF 0b0000000000000001
#define RF_BACKFACE_CULL 0b0000000000000010
#define RF_OUTLINES 0b0000000000000100
#define RF_WIREFRAME 0b0000000000001000
#define RF_BILINEAR 0b0000000000010000
#define RF_MIPMAP 0b0000000000100000
#define RF_TRILINEAR 0b0000000001000000

class Renderer
{

private:

	template <class Pixel>
	static Pixel Lerp(const Pixel& p1, const Pixel& p2, float alpha) {

		// do not use this for small objects, it will be outperformed by the operator overloads

		constexpr short NUMFLOATS = sizeof(Pixel) / sizeof(float);
		float buf[NUMFLOATS];

		for (int i = 0; i < NUMFLOATS; ++i)
			*(buf + i) = *((float*)&p1 + i) * (1 - alpha) + *((float*)&p2 + i) * alpha;


		return *(Pixel*)buf;
	}

	template <class Pixel>
	static Pixel Mult(const Pixel& p, float s) {

		// do not use this for small objects, it will be outperformed by the operator overloads

		constexpr short NUMFLOATS = sizeof(Pixel) / sizeof(float);
		float buf[NUMFLOATS];

		for (int i = 0; i < NUMFLOATS; ++i)
			*(buf + i) = *((float*)&p + i) * s;


		return *(Pixel*)buf;

	}

public:

	// any class that will be used as vertex data in this renderer
	// must extend this class and implement GetPos
	class PixelShaderInput {

	public:

		PixelShaderInput() {}

		virtual Vec4& GetPos() = 0;

		inline void WDivide() {

			GetPos().x /= GetPos().w;
			GetPos().y /= GetPos().w;
			GetPos().z /= GetPos().w;

		}

	};

	class DepthBuffer {

		friend class Renderer;

	private:
		float* pDepths;
		int width;
		int height;

		DepthBuffer(int width, int height);

		inline void PutPixel(int x, int y, float depth) {
			pDepths[width * y + x] = depth;
		}

	public:

		DepthBuffer();
		DepthBuffer(const DepthBuffer& db);
		DepthBuffer& operator=(const DepthBuffer& db);
		~DepthBuffer();

		void Resize(int width, int height);
		void SaveToFile(const std::string& filename) const;

		int GetWidth() const;
		int GetHeight() const;

		void WhiteOut();

		inline float GetPixel(int x, int y) const {

			return pDepths[y * width + x];
		}

	};

	template <class Pixel>
	class Sampler {

		friend class Renderer;

	private:

		// needed to access the rendering flags
		const Renderer& parentRenderer;

		bool flatTop;

		const Pixel* topVertex = nullptr;
		const Pixel* leftVertex;
		const Pixel* rightVertex;
		const Pixel* bottomVertex = nullptr;

		const Vec2* topScreen = nullptr;
		const Vec2* leftScreen;
		const Vec2* rightScreen;
		const Vec2* bottomScreen = nullptr;

		const int& xCoord;
		const int& yCoord;

		const Pixel& current;
		std::unordered_map<int, Pixel> aboveLookup;

		mutable bool newScanline = true;

		Pixel GetInterpolatedPixel(int x, int y) const {

			// this function is needed if a pixels above is unknown (edges of the triangle)
			// finds what the pixel at xCoord yCoord is using interpolation

			// top and bottom values need to be exactly the same as in the rasterization
			int pixelTop = flatTop ? (int)ceil(leftScreen->y - 0.5) : (int)ceil(topScreen->y - 0.5);
			int pixelBottom = flatTop ? (int)ceil(bottomScreen->y - 0.5) : (int)ceil(leftScreen->y - 0.5);

			// how much to interpolate down the triangle
			float alphaDown = (float)(y - pixelTop) / (pixelBottom - pixelTop);

			// protect against out of bounds pixels
			if (alphaDown > 1)
				alphaDown = 1;
			if (alphaDown < 0)
				alphaDown = 0;

			Pixel leftTravelerVertex = flatTop ? Lerp(*leftVertex, *bottomVertex, alphaDown) : Lerp(*topVertex, *leftVertex, alphaDown);
			Vec2 leftTravelerScreen = flatTop ? (*leftScreen * (1 - alphaDown) + *bottomScreen * alphaDown) :
				(*topScreen * (1 - alphaDown) + *leftScreen * alphaDown);

			Pixel rightTravelerVertex = flatTop ? Lerp(*rightVertex, *bottomVertex, alphaDown) : Lerp(*topVertex, *rightVertex, alphaDown);
			Vec2 rightTravelerScreen = flatTop ? (*rightScreen * (1 - alphaDown) + *bottomScreen * alphaDown) :
				(*topScreen * (1 - alphaDown) + *rightScreen * alphaDown);

			// left and right values need to be exactly the same as in the rasterization
			int pixelLeft = (int)ceil(leftTravelerScreen.x - 0.5);
			int pixelRight = (int)ceil(rightTravelerScreen.x - 0.5);

			// interpolate across the triangle
			float alphaAcross = (float)(x - pixelLeft) / (pixelRight - pixelLeft);

			if (alphaAcross > 1)
				alphaAcross = 1;
			if (alphaAcross < 0)
				alphaAcross = 0;

			Pixel acrossTraveler = Lerp(leftTravelerVertex, rightTravelerVertex, alphaAcross);

			// flip perspective, the pixels this value was derived from have inverted perspectives
			FlipPerspective<Pixel>(acrossTraveler);

			//return the pixel
			return acrossTraveler;

		}

		Vec4 LinearSample(const Surface& texture, const Vec2& texel) const {

			// enables texture tiling
			float s = texel.s - (int)texel.s;
			float t = texel.t - (int)texel.t;

			// completely regular texture sample
			return texture.GetPixel((int)(s * (texture.GetWidth() - 1)), (int)(t * (texture.GetHeight() - 1)));
		}

		Vec4 BiLinearSample(const Surface& texture, const Vec2& texel) const {

			// Equations for a Bi Linear Sample
			// From Section 7.5.4, "Mathematics for 3D Game Programming and Computer Graphics", Lengyel

			// enables texture tiling
			float s = texel.s - (int)texel.s;
			float t = texel.t - (int)texel.t;

			// texel location minus half a pixel in x and y
			int i = (texture.GetWidth() * s - 0.5);
			int j = (texture.GetHeight() * t - 0.5);

			// fractional parts of the displaced texel location
			float alpha = i - (int)i;
			float beta = j - (int)j;

			i = (int)i;
			j = (int)j;

			// texture samples of the 4 pixel square
			const Vec4& c1 = texture.GetPixel(i, j);
			const Vec4& c2 = i + 1 >= texture.GetWidth() ? texture.GetPixel(i, j) : texture.GetPixel(i + 1, j);
			const Vec4& c3 = j + 1 >= texture.GetHeight() ? texture.GetPixel(i, j) : texture.GetPixel(i, j + 1);
			const Vec4& c4 = i + 1 >= texture.GetWidth() || j + 1 >= texture.GetHeight() ? texture.GetPixel(i, j) : texture.GetPixel(i + 1, j + 1);

			// weighted average of the 4
			return c1 * (1 - alpha) * (1 - beta) +
				c2 * alpha * (1 - beta) +
				c3 * (1 - alpha) * beta +
				c4 * alpha * beta;
		}

	public:

		// if flat top is true, the pixels are left, bottom, right
		// if flat top is false, the pixels are left, top, right
		Sampler(const Renderer& parentRenderer, const Pixel& currentPixel, const Pixel* p1, const Vec2* v1, const Pixel* p2, const Vec2* v2, const Pixel* p3, const Vec2* v3, bool flatTop, int& xCounter, int& yCounter)
			:
			parentRenderer(parentRenderer), current(currentPixel), flatTop(flatTop), xCoord(xCounter), yCoord(yCounter)
		{

			leftVertex = p1;
			leftScreen = v1;

			rightVertex = p3;
			rightScreen = v3;

			if (flatTop) {

				bottomVertex = p2;
				bottomScreen = v2;

			}
			else {

				topVertex = p2;
				topScreen = v2;

			}

			aboveLookup.reserve((unsigned int)(rightScreen->x - leftScreen->x + 1));
		}

		Vec4 SampleTex2D(const Surface& texture, int texelOffsetIntoPixel) const {

			// Equations from Section 7.5, "Mathematics for 3D Game Programming and Computer Graphics", Lengyel

			// the texel we want to sample from
			const Vec2& texel1 = *(Vec2*)((float*)&current + texelOffsetIntoPixel);

			// texture that will eventually be read from (could be a mip map)
			const Surface* textureToSample = &texture;

			// if mip mapping is enabled
			if (parentRenderer.flags & RF_MIPMAP) {

				// read texels of surrounding pixels, necessary for mip mapping
				// if the pixel does not have a known pixel above it, their values will be interpolated
				// if it is the beginning of a new scanline, it is possible there is a pixel above but
				// the previous pixel is on the previous scanline

				// the previous can be retrieved from the previous entry in the hash map
				const Pixel pixel2 = aboveLookup.count(xCoord) && !newScanline ? aboveLookup.at(xCoord - 1) : GetInterpolatedPixel(xCoord - 1, yCoord);
				const Vec2& texel2 = *(Vec2*)((float*)&pixel2 + texelOffsetIntoPixel);

				const Pixel pixel3 = aboveLookup.count(xCoord) && !newScanline ? aboveLookup.at(xCoord) : GetInterpolatedPixel(xCoord, yCoord - 1);
				const Vec2& texel3 = *(Vec2*)((float*)&pixel3 + texelOffsetIntoPixel);

				// we have processed a pixel, so are no longer on the first pixel of a scanline
				newScanline = false;

				// find derivatives of the texel pixels in the x pixel direction
				float dudx = texture.GetWidth() * (texel2 - texel1).s;
				float dvdx = texture.GetHeight() * (texel2 - texel1).t;

				// find derivatives of the texel pixels in the y pixel direction
				float dudy = texture.GetWidth() * (texel3 - texel1).s;
				float dvdy = texture.GetHeight() * (texel3 - texel1).t;

				// find densities of pixels on the texture per pixel on the screen in the x and y directions
				float densityX = sqrt(dudx * dudx + dvdx * dvdx);
				float densityY = sqrt(dudy * dudy + dvdy * dvdy);

				// log2 of the greatest density is the mip map level
				float mipMapLod = logf(fmaxf(densityX, densityY)) + 0.5;
			
				textureToSample = texture.GetMipMap((int)mipMapLod);

				// only consider the trilinear flag if mipmapping is also enabled
				if (parentRenderer.flags & RF_TRILINEAR) {

					// get this and the next mip map
					const Surface* mm1 = texture.GetMipMap((int)mipMapLod);
					const Surface* mm2 = texture.GetMipMap((int)mipMapLod + 1);

					// sample the mip maps
					Vec4 mm1Sample = parentRenderer.flags & RF_BILINEAR ? BiLinearSample(*mm1, texel1) : LinearSample(*mm1, texel1);
					Vec4 mm2Sample = parentRenderer.flags & RF_BILINEAR ? BiLinearSample(*mm2, texel1) : LinearSample(*mm2, texel1);

					float lodFrac = mipMapLod - (int)mipMapLod;

					// return a weighted average of the two samples
					return mm1Sample * (1 - lodFrac) + mm2Sample * lodFrac;
				}
			}

			// if bilinear sampling is enabled
			return parentRenderer.flags & RF_BILINEAR ? BiLinearSample(*textureToSample, texel1) : LinearSample(*textureToSample, texel1);

		}

		Vec4 SampleCubeMap(const Surface* planes, float s, float t, float p) const {

			// Cube map sampling equations from section 7.5, "Mathematics for 3D Game Programming and Computer Graphics", Lengyel

			const Surface* surfaceToSample = nullptr;
			float finalS = 0;
			float finalT = 0;

			float absS = fabs(s);
			float absT = fabs(t);
			float absP = fabs(p);

			// figure out the face to sample and the coordinate to sample at
			// determined by sign of coordinate with largest absolute value

			if (absS >= absT && absS >= absP) {

				if (s > 0) {
					// positive x
					surfaceToSample = &planes[0];
					finalS = 0.5f - p / (2 * s);
					finalT = 0.5f - t / (2 * s);

				}
				else {
					// negative x
					surfaceToSample = &planes[1];
					finalS = 0.5f - p / (2 * s);
					finalT = 0.5f + t / (2 * s);
				}

			}
			else if (absT >= absS && absT >= absP) {

				if (t > 0) {
					// positive y
					surfaceToSample = &planes[2];
					finalS = 0.5f + s / (2 * t);
					finalT = 0.5f + p / (2 * t);
				}
				else {
					// negative y
					surfaceToSample = &planes[3];
					finalS = 0.5f - s / (2 * t);
					finalT = 0.5f + p / (2 * t);
				}

			}
			else {
				if (p > 0) {
					// positive z
					surfaceToSample = &planes[4];
					finalS = 0.5f + s / (2 * p);
					finalT = 0.5f - t / (2 * p);
				}
				else {
					// negative z
					surfaceToSample = &planes[5];
					finalS = 0.5f + s / (2 * p);
					finalT = 0.5f + t / (2 * p);
				}
			}

			// don't bother with bilinear sampling, cube maps usually are high resolution
			return LinearSample(*surfaceToSample, { finalS, finalT });

		}
	};

private:

	// if a draw call is made where the 
	// pixel and vertex shaders are member functions
	// this will store the object that made the call
	void* boundObject = nullptr;

	Surface* pRenderTarget;

	DepthBuffer depthBuffer;

	short flags = 0;

	// don't want to use threads for large models
	// creating many of them kills performance
	// they are good for cube maps
	bool usingThreads = false;

	template <class Vertex, class Pixel>
	using VS_TYPE = Pixel(*)(Vertex& v);

	template <class Pixel>
	using PS_TYPE = Vec4(*)(Pixel & sd, const Sampler<Pixel> & sampler);

	template <class Vertex, class Pixel>
	class NO_CLASS {

	public:
		virtual Pixel VertexShader(Vertex& v) = 0;
		virtual Vec4 PixelShader(Pixel& p, const Sampler<Pixel>& sampler) = 0;

	};

	template <class Pixel, class Mesh, typename PSPtr>
	void ClipAndDrawTriangle(Pixel& p1, Pixel& p2, Pixel& p3, PSPtr PixelShader, int iteration) {

		int memberVariableOffset = 0;

		// the sign of a plane with a negative value is POSITIVE
		// the sign of a plane with a positive value is NEGATIVE
		int signOfPlane = 1;
	
		switch (iteration) {

		case 0:
			//clipping against near plane
			memberVariableOffset = Z_OFFSET;
			signOfPlane = 1;
			break;

		case 1:
			// clipping against the far plane
			memberVariableOffset = Z_OFFSET;
			signOfPlane = -1;
			break;

		case 2:
			// clipping aginst the left plane
			memberVariableOffset = X_OFFSET;
			signOfPlane = 1;
			break;

		case 3:
			// clipping against the right plane
			memberVariableOffset = X_OFFSET;
			signOfPlane = -1;
			break;

		case 4:
			// clipping against the bottom plane
			memberVariableOffset = Y_OFFSET;
			signOfPlane = 1;
			break;

		case 5:
			// clipping against the top plane
			memberVariableOffset = Y_OFFSET;
			signOfPlane = -1;
			break;

		default:
			DrawTriangle<Pixel, Mesh, PSPtr>(p1, p2, p3, PixelShader);
			return;
		}

		// get the value of the position members being tested
		float p1Value = *((float*)&p1.GetPos() + memberVariableOffset);
		float p2Value = *((float*)&p2.GetPos() + memberVariableOffset);
		float p3Value = *((float*)&p3.GetPos() + memberVariableOffset);

		// if the w plus the plane sign times the value is less than 0, the point is outside the clipping volume
		bool p1Outside = p1.GetPos().w + signOfPlane * p1Value < 0;
		bool p2Outside = p2.GetPos().w + signOfPlane * p2Value < 0;
		bool p3Outside = p3.GetPos().w + signOfPlane * p3Value < 0;

		if (p1Outside) {
			if (p2Outside) {
				if (p3Outside) {
					
					//all points outside
					return;
				}
				else {

					//p1 outside, p2 outside, p3 inside
					Clip2Outside<Pixel, Mesh, PSPtr>(p1, p2, p3, memberVariableOffset, signOfPlane, iteration + 1, PixelShader);
				}
			}
			else {
				if (p3Outside) {

					//p1 outside, p2 inside, p3 outside
					Clip2Outside<Pixel, Mesh, PSPtr>(p3, p1, p2, memberVariableOffset, signOfPlane, iteration + 1, PixelShader);
				}
				else {
		
					//p1 outside, p2 inside, p3 inside
					Clip1Outside<Pixel, Mesh, PSPtr>(p1, p2, p3, memberVariableOffset, signOfPlane, iteration + 1, PixelShader);
				}
			}
		}
		else {
			if (p2Outside) {
				if (p3Outside) {

					//p1 inside, p2 outside, p3 outside
					Clip2Outside<Pixel, Mesh, PSPtr>(p2, p3, p1, memberVariableOffset, signOfPlane, iteration + 1, PixelShader);
				}
				else {

					//p1 inside, p2 outside, p3 inside
					Clip1Outside<Pixel, Mesh, PSPtr>(p2, p3, p1, memberVariableOffset, signOfPlane, iteration + 1, PixelShader);
				}
			}
			else {
				if (p3Outside) {

					//p1 inside, p2 inside, p3 outside
					Clip1Outside<Pixel, Mesh, PSPtr>(p3, p1, p2, memberVariableOffset, signOfPlane, iteration + 1, PixelShader);
				}
				else {

					// clip against next plane
					ClipAndDrawTriangle<Pixel, Mesh, PSPtr>(p1, p2, p3, PixelShader, iteration + 1);
				}
			}
		}

	}

	template <class Pixel, class Mesh, typename PSPtr>
	void Clip1Outside(Pixel& outside, Pixel& inside1, Pixel& inside2, int memberVariableOffset, int signOfPlane, int nextIteration, PSPtr PixelShader) {

		// the triangle will be drawn with the order, outside, inside1, inside2

		float outsideValue = *((float*)&outside.GetPos() + memberVariableOffset);
		float inside1Value = *((float*)&inside1.GetPos() + memberVariableOffset);
		float inside2Value = *((float*)&inside2.GetPos() + memberVariableOffset);

		// general formula for alpha values
		// a = (w1 +- x1) / ((w1 +- x1) - (w2 +- x2)), from "Clipping Using Homogeneous Coordinates", Blinn, Newell
		float alpha1 = (outside.GetPos().w + signOfPlane * outsideValue) / ((outside.GetPos().w + signOfPlane * outsideValue) - (inside1.GetPos().w + signOfPlane * inside1Value));
		float alpha2 = (outside.GetPos().w + signOfPlane * outsideValue) / ((outside.GetPos().w + signOfPlane * outsideValue) - (inside2.GetPos().w + signOfPlane * inside2Value));
		
		Pixel n1 = Lerp(outside, inside1, alpha1);
		Pixel n2 = Lerp(outside, inside2, alpha2);
		
		ClipAndDrawTriangle<Pixel, Mesh, PSPtr>(n1, inside1, inside2, PixelShader, nextIteration);
		ClipAndDrawTriangle<Pixel, Mesh, PSPtr>(n1, inside2, n2, PixelShader, nextIteration);

	}

	template <class Pixel, class Mesh, typename PSPtr>
	void Clip2Outside(Pixel& outside1, Pixel& outside2, Pixel& inside, int memberVariableOffset, int signOfPlane, int nextIteration, PSPtr PixelShader) {

		// the triangle will be drawn with the order outside1, outside2, inside

		float outside1Value = *((float*)&outside1.GetPos() + memberVariableOffset);
		float outside2Value = *((float*)&outside2.GetPos() + memberVariableOffset);
		float insideValue = *((float*)&inside.GetPos() + memberVariableOffset);

		// general formula for alpha values
		// a = (w1 +- x1) / ((w1 +- x1) - (w2 +- x2)), from "Clipping Using Homogeneous Coordinates", Blinn, Newell
		float alpha1 = (outside1.GetPos().w + signOfPlane * outside1Value) / ((outside1.GetPos().w + signOfPlane * outside1Value) - (inside.GetPos().w + signOfPlane * insideValue));
		float alpha2 = (outside2.GetPos().w + signOfPlane * outside2Value) / ((outside2.GetPos().w + signOfPlane * outside2Value) - (inside.GetPos().w + signOfPlane * insideValue));
		
		Pixel n1 = Lerp(outside1, inside, alpha1);
		Pixel n2 = Lerp(outside2, inside, alpha2);
		
		ClipAndDrawTriangle<Pixel, Mesh, PSPtr>(n1, n2, inside, PixelShader, nextIteration);

	}

	template <class Pixel>
	static void FlipPerspective(Pixel& p) {

		// store old position
		Vec4 oldPosition = p.GetPos();

		// flip the positions w
		oldPosition.w = 1 / oldPosition.w;

		// the first time this runs, it has the effect of dividing by w
		// the second time this runs, it will undo the previous division
		p = Mult(p, oldPosition.w);

		// restore the old position, position does not get divided by w
		// in perspective correct interpolation, but everything else does
		p.GetPos() = oldPosition;

	}

	template <class Pixel, class Mesh, typename PSPtr>
	void DrawTriangle(Pixel p1, Pixel p2, Pixel p3, PSPtr PixelShader) {
		
		// w divide
		p1.WDivide();
		p2.WDivide();
		p3.WDivide();
		
		// backface culling, must be done after w divide
		if (flags & RF_BACKFACE_CULL) {

			Vec4 p1p2 = p2.GetPos() - p1.GetPos();
			Vec4 p1p3 = p3.GetPos() - p1.GetPos();

			// must do cross product of p1-p2 with p1-p3
			// a positive z value would be pointing away from the camera,
			// but the handedness of the coordinate system was reversed
			// in the perspective projection, so we check  < 0
			if ((p1p2 % p1p3).z < 0)
				return;
		}

		//calculate pixel coordinates

		// all of these casts are to get rid of compiler warnings, here is what is actually happening
		// screen x = (vec.x + 1) * (screen width / 2)
		// screen y = (-vec.y + 1) * (screeh height / 2)

		// 0.1 is being subtracted from screen width to keep pixels from spilling from the right edge
		// of the screen to the left edge of the screen

		// all pixel values must be rounded down (int) to prevent fill gaps
		// im not sure why it works but it does

		Vec2 v1Screen((float)(int)((float)(p1.GetPos().x + 1.0f) * ((float)pRenderTarget->GetWidth() - 0.1f) / 2.0f),
			(float)(int)((-(float)p1.GetPos().y + 1.0f) * (float)((pRenderTarget->GetHeight() - 0.1f) / 2)));

		Vec2 v2Screen((float)(int)(((float)p2.GetPos().x + 1.0f) * ((float)pRenderTarget->GetWidth() - 0.1f) / 2.0f),
			(float)(int)((-(float)p2.GetPos().y + 1.0f) * (float)((pRenderTarget->GetHeight() - 0.1f) / 2)));

		Vec2 v3Screen((float)(int)(((float)p3.GetPos().x + 1.0f) * ((float)pRenderTarget->GetWidth() - 0.1f) / 2.0f),
			(float)(int)((-(float)p3.GetPos().y + 1.0f) * (float)((pRenderTarget->GetHeight() - 0.1f) / 2)));

		// if wireframe mode is enabled
		if (flags & RF_WIREFRAME) {

			pRenderTarget->DrawLine((int)v1Screen.x, (int)v1Screen.y, (int)v2Screen.x, (int)v2Screen.y, 0xffffffff);
			pRenderTarget->DrawLine((int)v1Screen.x, (int)v1Screen.y, (int)v3Screen.x, (int)v3Screen.y, 0xffffffff);
			pRenderTarget->DrawLine((int)v3Screen.x, (int)v3Screen.y, (int)v2Screen.x, (int)v2Screen.y, 0xffffffff);

			return;

		}

		// make pointers for top middle and bottom
		Pixel* topPixel = &p1;
		Vec2* topScreen = &v1Screen;

		Pixel* middlePixel = &p2;
		Vec2* middleScreen = &v2Screen;

		Pixel* bottomPixel = &p3;
		Vec2* bottomScreen = &v3Screen;

		// sort vertices by height
		if (middleScreen->y < topScreen->y) {
			std::swap(middlePixel, topPixel);
			std::swap(middleScreen, topScreen);
		}

		if (bottomScreen->y < middleScreen->y) {
			std::swap(bottomPixel, middlePixel);
			std::swap(bottomScreen, middleScreen);
		}

		if (middleScreen->y < topScreen->y) {
			std::swap(middlePixel, topPixel);
			std::swap(middleScreen, topScreen);
		}

		// for perspective correct interpolation
		FlipPerspective(*bottomPixel);
		FlipPerspective(*topPixel);
		FlipPerspective(*middlePixel);

		// needed if a triangle is clipped down to a single pixel
		if ((int)bottomScreen->y == (int)topScreen->y)
			return;

		// make the cut vertex
		float cutAlpha = (middleScreen->y - topScreen->y) / (bottomScreen->y - topScreen->y);
		Vec2 cutScreen = *topScreen * (1 - cutAlpha) + *bottomScreen * cutAlpha;
		Pixel cutPixel = Lerp(*topPixel, *bottomPixel, cutAlpha);

		// draw the flat top and flat bottom
		if (cutScreen.x > middleScreen->x) {

			// cut is on the right
			if (usingThreads) {

				std::thread flatBottom(&Renderer::DrawHalfTriangle<FLAT_BOTTOM, Pixel, Mesh, PSPtr>, this, std::ref(*middlePixel), std::ref(*middleScreen), std::ref(*topPixel), std::ref(*topScreen), std::ref(cutPixel), std::ref(cutScreen), PixelShader);
				DrawHalfTriangle<FLAT_TOP, Pixel, Mesh, PSPtr>(*middlePixel, *middleScreen, *bottomPixel, *bottomScreen, cutPixel, cutScreen, PixelShader);
				flatBottom.join();
			}
			else {

				DrawHalfTriangle<FLAT_BOTTOM, Pixel, Mesh, PSPtr>(*middlePixel, *middleScreen, *topPixel, *topScreen, cutPixel, cutScreen, PixelShader);
				DrawHalfTriangle<FLAT_TOP, Pixel, Mesh, PSPtr>(*middlePixel, *middleScreen, *bottomPixel, *bottomScreen, cutPixel, cutScreen, PixelShader);
			}
	
		}
		else {

			// cut is on the left
			if (usingThreads) {

				std::thread flatBottom(&Renderer::DrawHalfTriangle<FLAT_BOTTOM, Pixel, Mesh, PSPtr>, this, std::ref(cutPixel), std::ref(cutScreen), std::ref(*topPixel), std::ref(*topScreen), std::ref(*middlePixel), std::ref(*middleScreen), PixelShader);
				DrawHalfTriangle<FLAT_TOP, Pixel, Mesh, PSPtr>(cutPixel, cutScreen, *bottomPixel, *bottomScreen, *middlePixel, *middleScreen, PixelShader);
				flatBottom.join();
			}
			else {
				DrawHalfTriangle<FLAT_BOTTOM, Pixel, Mesh, PSPtr>(cutPixel, cutScreen, *topPixel, *topScreen, *middlePixel, *middleScreen, PixelShader);
				DrawHalfTriangle<FLAT_TOP, Pixel, Mesh, PSPtr>(cutPixel, cutScreen, *bottomPixel, *bottomScreen, *middlePixel, *middleScreen, PixelShader);
			}
		}

		// if outlines mode is enabled
		if (flags & RF_OUTLINES) {

			pRenderTarget->DrawLine((int)v1Screen.x, (int)v1Screen.y, (int)v2Screen.x, (int)v2Screen.y, 0xffffffff);
			pRenderTarget->DrawLine((int)v1Screen.x, (int)v1Screen.y, (int)v3Screen.x, (int)v3Screen.y, 0xffffffff);
			pRenderTarget->DrawLine((int)v3Screen.x, (int)v3Screen.y, (int)v2Screen.x, (int)v2Screen.y, 0xffffffff);

		}
	}

	template <class Pixel, class Mesh, typename PSPtr>
	Vec4 RunPixelShader(PSPtr PixelShader, Pixel& args, const Sampler<Pixel>& sampler) {

		if (boundObject == nullptr) {

			return PixelShader(args, sampler);

		}
		else {

			Mesh* mesh = (Mesh*)boundObject;
			return mesh->PixelShader(args, sampler);

		}

	}

	template <int TYPE, class Pixel, class Mesh, typename PSPtr>
	void DrawHalfTriangle(Pixel& leftPixel, Vec2& leftScreen, Pixel& otherPixel, Vec2& otherScreen, Pixel& rightPixel, Vec2& rightScreen, PSPtr PixelShader) {
		
		// incase an invalid triangle type gets passed in
		if constexpr (TYPE != FLAT_TOP && TYPE != FLAT_BOTTOM)
			return;
		
		// if type is flat top, other pixel/screen are the bottom
		// if type is flat bottom, other pixel/screen are the top

		// absolute highest and lowest pixels of the triangle
		int pixelTop, pixelBottom;

		if constexpr (TYPE == FLAT_TOP) {
			pixelTop = (int)ceil(leftScreen.y - 0.5);
			pixelBottom = (int)ceil(otherScreen.y - 0.5);
		}
		else if constexpr (TYPE == FLAT_BOTTOM) {
			pixelTop = (int)ceil(otherScreen.y - 0.5);
			pixelBottom = (int)ceil(leftScreen.y - 0.5);
		}

		// will walk down the left and right edges of the triangle
		Pixel leftTravelerPixel, rightTravelerPixel;
		Vec2 leftTravelerScreen, rightTravelerScreen;

		if constexpr (TYPE == FLAT_TOP) {
			leftTravelerPixel = leftPixel;
			leftTravelerScreen = leftScreen;
		}
		else if constexpr (TYPE == FLAT_BOTTOM) {
			leftTravelerPixel = otherPixel;
			leftTravelerScreen = otherScreen;
		}

		if constexpr (TYPE == FLAT_TOP) {
			rightTravelerPixel = rightPixel;
			rightTravelerScreen = rightScreen;
		}
		else if constexpr (TYPE == FLAT_BOTTOM) {
			rightTravelerPixel = otherPixel;
			rightTravelerScreen = otherScreen;
		}

		// create x and y here for the sampler
		int x, y;

		// will walk across the scanlines
		Pixel acrossTravelerPixel;

		if constexpr (TYPE == FLAT_TOP)
			acrossTravelerPixel = leftPixel;
		else if constexpr (TYPE == FLAT_BOTTOM)
			acrossTravelerPixel = otherPixel;

		// create 2d sampler for this flat top triangle
		Sampler<Pixel> sampler2d(*this, acrossTravelerPixel, &leftPixel, &leftScreen, &otherPixel, &otherScreen, &rightPixel, &rightScreen, TYPE == FLAT_TOP, x, y);
		
		for (y = pixelTop; y < pixelBottom; ++y) {
			
			// % of the way down the triangle we are
			float howFarDown = (float)(y - pixelTop) / (pixelBottom - pixelTop);

			// interpolate the travelers down the left and right edges of the triangle
			if constexpr (TYPE == FLAT_TOP) {

				leftTravelerPixel = Lerp(leftPixel, otherPixel, howFarDown);
				leftTravelerScreen = leftScreen * (1 - howFarDown) + otherScreen * howFarDown;
				rightTravelerPixel = Lerp(rightPixel, otherPixel, howFarDown);
				rightTravelerScreen = rightScreen * (1 - howFarDown) + otherScreen * howFarDown;
			}
			else if constexpr (TYPE == FLAT_BOTTOM) {

				leftTravelerPixel = Lerp(otherPixel, leftPixel, howFarDown);
				leftTravelerScreen = otherScreen * (1 - howFarDown) + leftScreen * howFarDown;
				rightTravelerPixel = Lerp(otherPixel, rightPixel, howFarDown);
				rightTravelerScreen = otherScreen * (1 - howFarDown) + rightScreen * howFarDown;
			}

			// left and rightmost pixels in this scanline
			int pixelLeft = (int)ceil(leftTravelerScreen.x - 0.5);
			int pixelRight = (int)ceil(rightTravelerScreen.x - 0.5);

			// will walk across this scanline
			acrossTravelerPixel = leftTravelerPixel;

			for (x = pixelLeft; x <= pixelRight; ++x) {
				
				// % of the way across the scanline we are
				float howFarAcross = (float)(x - pixelLeft) / (pixelRight - pixelLeft);
				
				// interpolate the traveler across the scanline
				acrossTravelerPixel = Lerp(leftTravelerPixel, rightTravelerPixel, howFarAcross);

				// undo the perspective correct interpolation
				FlipPerspective(acrossTravelerPixel);

				// get the depth and normalize from 0 to 1, use w for depth
				float normalizedDepth = flags & RF_LINEAR_ZBUF ?
					(acrossTravelerPixel.GetPos().w - NEAR) / (FAR - NEAR)
					:
					(1 / acrossTravelerPixel.GetPos().w - 1 / NEAR) / (1 / FAR - 1 / NEAR);
				normalizedDepth = (acrossTravelerPixel.GetPos().z + 1) / 2;

				// test the pixel agains the z buffer
				if (TestAndSetPixel(x, y, normalizedDepth)) {

					// run the pixel shader
					Vec4 pixelColor = RunPixelShader<Pixel, Mesh, PSPtr>(PixelShader, acrossTravelerPixel, sampler2d);
					pRenderTarget->PutPixel(x, y, pixelColor);

				}

				// update the sampler before flipping the perspective
				sampler2d.aboveLookup[x] = sampler2d.current;

				// redo the perspective correct interpolation
				FlipPerspective(acrossTravelerPixel);

			}

			// tell the sampler that a new scan line is beginning
			sampler2d.newScanline = true;
			
		}

	}

	bool TestAndSetPixel(int x, int y, float normalizedDepth);

	template <class Vertex, class Pixel, class Mesh, typename VSPtr>
	Pixel RunVertexShader(VSPtr VertexShader, Vertex& args) {

		if (boundObject == nullptr) {

			return VertexShader(args);

		}
		else {

			Mesh* mesh = (Mesh*)boundObject;
			return mesh->VertexShader(args);

		}

	}

	template <class Vertex, class Pixel, class Mesh, typename VSPtr, typename PSPtr>
	void _DrawElementArray(int numIndexGroups, int* indices, Vertex* vertices, VSPtr VertexShader, PSPtr PixelShader) {

		if (pRenderTarget == nullptr)
			return;

		std::vector<std::thread> threads;
		std::unordered_map<int, Pixel> processedVertices;

		// creating many many threads seems to cause major lag
		// so only use threads for models with a few large triangles
		if (numIndexGroups < 20)
			usingThreads = true;

		for (int i = 0; i < numIndexGroups; ++i) {

			int i1 = indices[i * 3];
			int i2 = indices[i * 3 + 1];
			int i3 = indices[i * 3 + 2];

			// run the vertex shaders or look up the vertex if it has already been run
			if (!processedVertices.count(i1)) {
				processedVertices.emplace(i1, RunVertexShader<Vertex, Pixel, Mesh, VSPtr>(VertexShader, vertices[i1]));
			}
			if (!processedVertices.count(i2)) {
				processedVertices.emplace(i2, RunVertexShader<Vertex, Pixel, Mesh, VSPtr>(VertexShader, vertices[i2]));
			}
			if (!processedVertices.count(i3)) {
				processedVertices.emplace(i3, RunVertexShader<Vertex, Pixel, Mesh, VSPtr>(VertexShader, vertices[i3]));
			}

			// clear out any threads that have already finished
			auto it = threads.begin();
			while (it != threads.end()) {

				if (!it->joinable()) {
					it = threads.erase(it);

				}
				else {
					++it;
				}

			}

			// if we have enough resources to draw this triangle in a new thread, do so
			if (threads.size() < THREADS && usingThreads) {

				threads.emplace_back(&Renderer::ClipAndDrawTriangle<Pixel, Mesh, PSPtr>, this, std::ref(processedVertices[i1]), std::ref(processedVertices[i2]), std::ref(processedVertices[i3]), PixelShader, 0);

			}
			else {
				ClipAndDrawTriangle<Pixel, Mesh, PSPtr>(processedVertices[i1], processedVertices[i2], processedVertices[i3], PixelShader, 0);
			}


		}

		// wait for all threads to finish before returning
		for (unsigned int i = 0u; i < threads.size(); i++)
			threads[i].join();

		usingThreads = false;
	}

public:

	Renderer(Surface& renderTarget);
	~Renderer();

	void SetRenderTarget(Surface& renderTarget);

	template <class Vertex, class Pixel>
	void DrawElementArray(int numIndexGroups, int* indices, Vertex* vertices, VS_TYPE<Vertex, Pixel> VertexShader, PS_TYPE<Pixel> PixelShader) {

		// used when vertex and pixel shader are not member functions

		boundObject = nullptr;

		_DrawElementArray<Vertex, Pixel, NO_CLASS<Vertex, Pixel>, VS_TYPE<Vertex, Pixel>, PS_TYPE<Pixel>>(numIndexGroups, indices, vertices, VertexShader, PixelShader);

	}

	template <class Vertex, class Pixel, class Mesh>
	void DrawElementArray(Mesh* mesh, int numIndexGroups, int* indices, Vertex* vertices) {

		// used when vertex and pixel shader are member functions

		boundObject = (void*)mesh;

		_DrawElementArray<Vertex, Pixel, Mesh, VS_TYPE<Vertex, Pixel>, PS_TYPE<Pixel>>(numIndexGroups, indices, vertices, nullptr, nullptr);

		boundObject = nullptr;
	}

	const DepthBuffer& GetDepthBuffer() const;
	void ClearDepthBuffer();

	void SetFlags(short flags);
	void ClearFlags(short flags);
	bool TestFlags(short flags) const;

	Surface& GetRenderTarget();

};