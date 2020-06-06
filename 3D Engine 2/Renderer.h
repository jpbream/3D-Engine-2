#pragma once
#include "Surface.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include "Mat4.h"
#include "Utility.h"
#include <unordered_map>
#include <thread>

#define MAX_SUPPORTED_THREADS 32
#define NUM_THREADS (std::thread::hardware_concurrency() - 2)

#define RF_BACKFACE_CULL 0x2
#define RF_OUTLINES 0x4
#define RF_WIREFRAME 0x8
#define RF_BILINEAR 0x10
#define RF_MIPMAP 0x20
#define RF_TRILINEAR 0x40

#define RENDERER_DEBUG

#ifdef RENDERER_DEBUG
#undef NDEBUG
#endif
#include <assert.h>

class Renderer
{

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

		int allocatedSpace;

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

		enum {
			POSX = 0,
			NEGX = 1,
			POSY = 2,
			NEGY = 3,
			POSZ = 4,
			NEGZ = 5
		};

		const float WRAP_OFFSET = 1e-7f;

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
			float s = texel.s - (int)(texel.s - WRAP_OFFSET);
			float t = texel.t - (int)(texel.t - WRAP_OFFSET);

			// completely regular texture sample
			return texture.GetPixel((int)(s * (texture.GetWidth() - 1)), (int)(t * (texture.GetHeight() - 1)));
		}

		Vec4 BiLinearSample(const Surface& texture, const Vec2& texel) const {

			// Equations for a Bi Linear Sample
			// From Section 7.5.4, "Mathematics for 3D Game Programming and Computer Graphics", Lengyel

			// enables texture tiling
			float s = texel.s - (int)(texel.s - WRAP_OFFSET);
			float t = texel.t - (int)(texel.t - WRAP_OFFSET);

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

			const Surface* surfaceToSample = &planes[0];
			float finalS = 0;
			float finalT = 0;

			float absS = fabs(s);
			float absT = fabs(t);
			float absP = fabs(p);

			// figure out the face to sample and the coordinate to sample at
			// determined by sign of coordinate with largest absolute value

			if ( absS >= absT && absS >= absP ) {

				if ( s > 0 ) {
					// positive x
					surfaceToSample = &planes[POSX];
					finalS = 0.5f - p / (2 * s);
					finalT = 0.5f - t / (2 * s);

				}
				else {
					// negative x
					surfaceToSample = &planes[NEGX];
					finalS = 0.5f - p / (2 * s);
					finalT = 0.5f + t / (2 * s);
				}

			}
			else if ( absT >= absS && absT >= absP ) {

				if ( t > 0 ) {
					// positive y
					surfaceToSample = &planes[POSY];
					finalS = 0.5f + s / (2 * t);
					finalT = 0.5f + p / (2 * t);
				}
				else {
					// negative y
					surfaceToSample = &planes[NEGY];
					finalS = 0.5f - s / (2 * t);
					finalT = 0.5f + p / (2 * t);
				}

			}
			else {

				if ( p > 0 ) {
					// positive z
					surfaceToSample = &planes[POSZ];
					finalS = 0.5f + s / (2 * p);
					finalT = 0.5f - t / (2 * p);
				}
				else {
					// negative z
					surfaceToSample = &planes[NEGZ];
					finalS = 0.5f + s / (2 * p);
					finalT = 0.5f + t / (2 * p);
				}
			}

			// don't bother with bilinear sampling, cube maps usually are high resolution
			return LinearSample(*surfaceToSample, { finalS, finalT });

		}
	};

	template <class Vertex, class Pixel>
	using VS_TYPE = Pixel(*)(Vertex& v);

	template <class Pixel>
	using PS_TYPE = Vec4(*)(Pixel & sd, const Sampler<Pixel> & sampler);

private:

	Surface* pRenderTarget;

	DepthBuffer depthBuffer;

	// volatile because the main thread could change this while
	// a renderer thread is working
	volatile unsigned short flags = 0;

	enum {
		X_OFFSET = 0,
		Y_OFFSET = 1,
		Z_OFFSET = 2
	};

	enum {
		FLAT_TOP,
		FLAT_BOTTOM
	};

	enum {
		NEAR,
		FAR,
		LEFT,
		RIGHT,
		BOTTOM,
		TOP
	};

	template <class Pixel, typename PSPtr>
	void ClipAndDrawTriangle(Pixel& p1, Pixel& p2, Pixel& p3, PSPtr PixelShader, int iteration) {

		// offset of the position dimension we are looking at (x, y, or z)
		int memberVariableOffset = 0;

		// the sign of a plane with a negative value is POSITIVE
		// the sign of a plane with a positive value is NEGATIVE
		int signOfPlane = 1;
	
		switch (iteration) {

		case NEAR:
			//clipping against near plane
			memberVariableOffset = Z_OFFSET;
			signOfPlane = 1;
			break;

		case FAR:
			// clipping against the far plane
			memberVariableOffset = Z_OFFSET;
			signOfPlane = -1;
			break;

		case LEFT:
			// clipping aginst the left plane
			memberVariableOffset = X_OFFSET;
			signOfPlane = 1;
			break;

		case RIGHT:
			// clipping against the right plane
			memberVariableOffset = X_OFFSET;
			signOfPlane = -1;
			break;

		case BOTTOM:
			// clipping against the bottom plane
			memberVariableOffset = Y_OFFSET;
			signOfPlane = 1;
			break;

		case TOP:
			// clipping against the top plane
			memberVariableOffset = Y_OFFSET;
			signOfPlane = -1;
			break;

		default:
			DrawTriangle<Pixel, PSPtr>(p1, p2, p3, PixelShader);
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
					Clip2Outside<Pixel, PSPtr>(p1, p2, p3, memberVariableOffset, signOfPlane, iteration + 1, PixelShader);
				}
			}
			else {
				if (p3Outside) {

					//p1 outside, p2 inside, p3 outside
					Clip2Outside<Pixel, PSPtr>(p3, p1, p2, memberVariableOffset, signOfPlane, iteration + 1, PixelShader);
				}
				else {
		
					//p1 outside, p2 inside, p3 inside
					Clip1Outside<Pixel, PSPtr>(p1, p2, p3, memberVariableOffset, signOfPlane, iteration + 1, PixelShader);
				}
			}
		}
		else {
			if (p2Outside) {
				if (p3Outside) {

					//p1 inside, p2 outside, p3 outside
					Clip2Outside<Pixel, PSPtr>(p2, p3, p1, memberVariableOffset, signOfPlane, iteration + 1, PixelShader);
				}
				else {

					//p1 inside, p2 outside, p3 inside
					Clip1Outside<Pixel, PSPtr>(p2, p3, p1, memberVariableOffset, signOfPlane, iteration + 1, PixelShader);
				}
			}
			else {
				if (p3Outside) {

					//p1 inside, p2 inside, p3 outside
					Clip1Outside<Pixel, PSPtr>(p3, p1, p2, memberVariableOffset, signOfPlane, iteration + 1, PixelShader);
				}
				else {

					// clip against next plane
					ClipAndDrawTriangle<Pixel, PSPtr>(p1, p2, p3, PixelShader, iteration + 1);
				}
			}
		}

	}

	template <class Pixel, typename PSPtr>
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
		
		ClipAndDrawTriangle<Pixel, PSPtr>(n1, inside1, inside2, PixelShader, nextIteration);
		ClipAndDrawTriangle<Pixel, PSPtr>(n1, inside2, n2, PixelShader, nextIteration);

	}

	template <class Pixel, typename PSPtr>
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
		
		ClipAndDrawTriangle<Pixel, PSPtr>(n1, n2, inside, PixelShader, nextIteration);

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

	template <class Pixel, typename PSPtr>
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

		// 0.01 is being subtracted from screen width to keep pixels from spilling from the right edge
		// of the screen to the left edge of the screen

		// all pixel values must be rounded down (int) to prevent fill gaps
		// im not sure why it works but it does

		// use depth buffer dimensions because they will always be the same as the render targets dimensions
		// if it is not null ptr

		Vec2 v1Screen((float)(int)((float)(p1.GetPos().x + 1.0f) * ((float)depthBuffer.GetWidth() - 0.01f) / 2.0f),
			(float)(int)((-(float)p1.GetPos().y + 1.0f) * (float)((depthBuffer.GetHeight() - 0.01f) / 2)));

		Vec2 v2Screen((float)(int)(((float)p2.GetPos().x + 1.0f) * ((float)depthBuffer.GetWidth() - 0.01f) / 2.0f),
			(float)(int)((-(float)p2.GetPos().y + 1.0f) * (float)((depthBuffer.GetHeight() - 0.01f) / 2)));

		Vec2 v3Screen((float)(int)(((float)p3.GetPos().x + 1.0f) * ((float)depthBuffer.GetWidth() - 0.01f) / 2.0f),
			(float)(int)((-(float)p3.GetPos().y + 1.0f) * (float)((depthBuffer.GetHeight() - 0.01f) / 2)));

		// if wireframe mode is enabled, and there is a valid render target
		if (flags & RF_WIREFRAME && pRenderTarget) {

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

			DrawHalfTriangle<FLAT_BOTTOM, Pixel, PSPtr>(*middlePixel, *middleScreen, *topPixel, *topScreen, cutPixel, cutScreen, PixelShader);
			DrawHalfTriangle<FLAT_TOP, Pixel, PSPtr>(*middlePixel, *middleScreen, *bottomPixel, *bottomScreen, cutPixel, cutScreen, PixelShader);
			
		}
		else {

			DrawHalfTriangle<FLAT_BOTTOM, Pixel, PSPtr>(cutPixel, cutScreen, *topPixel, *topScreen, *middlePixel, *middleScreen, PixelShader);
			DrawHalfTriangle<FLAT_TOP, Pixel, PSPtr>(cutPixel, cutScreen, *bottomPixel, *bottomScreen, *middlePixel, *middleScreen, PixelShader);
			
		}

		// if outlines mode is enabled and there is a valid render target
		if (flags & RF_OUTLINES && pRenderTarget) {

			pRenderTarget->DrawLine((int)v1Screen.x, (int)v1Screen.y, (int)v2Screen.x, (int)v2Screen.y, 0xffffffff);
			pRenderTarget->DrawLine((int)v1Screen.x, (int)v1Screen.y, (int)v3Screen.x, (int)v3Screen.y, 0xffffffff);
			pRenderTarget->DrawLine((int)v3Screen.x, (int)v3Screen.y, (int)v2Screen.x, (int)v2Screen.y, 0xffffffff);

		}
	}

	template <int TYPE, class Pixel, typename PSPtr>
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

				// get the depth and normalize from 0 to 1
				float normalizedDepth = (acrossTravelerPixel.GetPos().z + 1) / 2;

				// test the pixel agains the z buffer
				// if pRenderTarget is null, this is a depth buffer only renderer
				if (TestAndSetPixel(x, y, normalizedDepth) && pRenderTarget) {

					// run the pixel shader
					Vec4 pixelColor = PixelShader(acrossTravelerPixel, sampler2d);
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

	template <class Vertex, class Pixel, typename VSPtr, typename PSPtr>
	void DEA_Thread(int idxStart, int numIdx, int* indices, Vertex* vertices, VSPtr VertexShader, PSPtr PixelShader)
	{
		// holds vertex shader results in case they are needed again
		std::unordered_map<int, Pixel> processedVertices;
		processedVertices.reserve(numIdx * 3);

		// loop through all triangles assigned to this thread
		for ( int i = idxStart; i < idxStart + numIdx; ++i ) {

			int i1 = indices[i * 3];
			int i2 = indices[i * 3 + 1];
			int i3 = indices[i * 3 + 2];

			// run the vertex shaders or look up the vertex if it has already been run
			if ( !processedVertices.count(i1) ) {
				processedVertices.emplace(i1, VertexShader(vertices[i1]));
			}
			if ( !processedVertices.count(i2) ) {
				processedVertices.emplace(i2, VertexShader(vertices[i2]));
			}
			if ( !processedVertices.count(i3) ) {
				processedVertices.emplace(i3, VertexShader(vertices[i3]));
			}

			// draw the triangle
			ClipAndDrawTriangle<Pixel, PSPtr>(processedVertices[i1], processedVertices[i2], processedVertices[i3], PixelShader, NEAR);

		}
	}

	template <class Vertex, class Pixel, typename VSPtr, typename PSPtr>
	void DEA_Launcher(int numIndexGroups, int* indices, Vertex* vertices, VSPtr VertexShader, PSPtr PixelShader) {

		//INVARIANTS

		// depth buffer size must equal render target size, if there is a render target
		assert(!(pRenderTarget != nullptr && (pRenderTarget->GetWidth() != depthBuffer.width || pRenderTarget->GetHeight() != depthBuffer.height)));

		// in case someone has a processor from another planet
		assert(NUM_THREADS <= MAX_SUPPORTED_THREADS);



		if ( NUM_THREADS <= numIndexGroups ) {

			// each thread will get more than one triangle

			// will store all threads that get created
			std::thread threads[MAX_SUPPORTED_THREADS];
			int threadsUsed = 0;

			// how many triangles each thread will be drawing, split equally
			int idxRange = numIndexGroups / NUM_THREADS;

			// use each thread, and tell it to draw its share of the triangles
			for ( int i = 0; i < NUM_THREADS * idxRange; i += idxRange ) {
				threads[threadsUsed++] =
					std::thread(
						&Renderer::DEA_Thread<Vertex, Pixel, VSPtr, PSPtr>,
						this,
						i,
						idxRange,
						indices,
						vertices,
						VertexShader,
						PixelShader
					);
			}

			// the leftover triangles (division remainder) will be drawn on this thread
			DEA_Thread<Vertex, Pixel, VSPtr, PSPtr>
				(
					numIndexGroups - (numIndexGroups % NUM_THREADS), 
					numIndexGroups % NUM_THREADS, 
					indices, 
					vertices, 
					VertexShader, 
					PixelShader
				);

			// clean up the threads
			for ( std::thread* t = threads; t < threads + threadsUsed; ++t )
				t->join();
			
		}
		else if ( NUM_THREADS > 0 ) {

			// each thread will get at most one triangle

			// will store all threads that get created
			std::thread threads[MAX_SUPPORTED_THREADS];
			int threadsUsed = 0;

			// send a single triangle to each thread 
			for ( int i = 0; i < numIndexGroups - 1; ++i ) {
				threads[threadsUsed++] =
					std::thread(
						&Renderer::DEA_Thread<Vertex, Pixel, VSPtr, PSPtr>,
						this,
						i,
						1,
						indices,
						vertices,
						VertexShader,
						PixelShader
					);
			}

			// draw the last triangle on this thread
			DEA_Thread<Vertex, Pixel, VSPtr, PSPtr>
				(
					numIndexGroups - 1,
					1,
					indices,
					vertices,
					VertexShader,
					PixelShader
					);

			// clean up the threads
			for ( std::thread* t = threads; t < threads + threadsUsed; ++t )
				t->join();

		}
		else {

			// if there are no threads, draw the whole model in a single draw call
			DEA_Thread<Vertex, Pixel, VSPtr, PSPtr>(0, numIndexGroups, indices, vertices, VertexShader, PixelShader);

		}

	}

public:

	Renderer(Surface& renderTarget);
	Renderer(int width, int height);
	~Renderer();

	void SetRenderTarget(Surface& renderTarget);

	template <class Vertex, class Pixel>
	void DrawElementArray(int numIndexGroups, int* indices, Vertex* vertices, VS_TYPE<Vertex, Pixel> VertexShader, PS_TYPE<Pixel> PixelShader) {

		DEA_Launcher<Vertex, Pixel, VS_TYPE<Vertex, Pixel>, PS_TYPE<Pixel>>
			(
				numIndexGroups, 
				indices, 
				vertices, 
				VertexShader, 
				PixelShader
			);
	}

	DepthBuffer& GetDepthBuffer();
	const DepthBuffer& GetDepthBuffer() const;
	void ClearDepthBuffer();

	void SetFlags(short flags);
	void ClearFlags(short flags);
	void ToggleFlags(short flags);
	bool TestFlags(short flags) const;

	Surface& GetRenderTarget();

};