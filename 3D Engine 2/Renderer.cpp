#include "Renderer.h"
#include "Vec2.h"
#include <SDL.h>

Renderer::Renderer(Surface& renderTarget) : pRenderTarget(&renderTarget), depthBuffer(renderTarget.GetWidth(), renderTarget.GetHeight()) {

	ClearDepthBuffer();

}

Renderer::~Renderer() {
	
}

void Renderer::SetRenderTarget(Surface& renderTarget) {

	if (renderTarget.GetWidth() == pRenderTarget->GetWidth() && renderTarget.GetHeight() == pRenderTarget->GetHeight()) {

		pRenderTarget = &renderTarget;
	}
}

bool Renderer::TestAndSetPixel(int x, int y, float normalizedDepth) {

	if (normalizedDepth < depthBuffer.GetPixel(x, y)) {
		depthBuffer.PutPixel(x, y, normalizedDepth);
		return true;
	}
	return false;
}

const Renderer::DepthBuffer& Renderer::GetDepthBuffer() const {

	return depthBuffer;
}

void Renderer::ClearDepthBuffer() {

	depthBuffer.WhiteOut();

}

void Renderer::SetFlags(short flags) {

	this->flags |= flags;

}

void Renderer::ClearFlags(short flags) {

	this->flags &= ~flags;

}

bool Renderer::TestFlags(short flags) const {

	return this->flags & flags;

}

Renderer::DepthBuffer::DepthBuffer(int width, int height) : width(width), height(height) {

	pDepths = new float[width * height];

}

Renderer::DepthBuffer::~DepthBuffer() {
	delete[] pDepths;
}

void Renderer::DepthBuffer::Resize(int width, int height) {

	if (width * height > this->width* this->height) {

		delete[] pDepths;
		pDepths = new float[width * height];

	}

	this->width = width;
	this->height = height;

}

void Renderer::DepthBuffer::SaveToFile(const std::string& filename) const {

	unsigned int* normalized = new unsigned int[width * height];

	unsigned int aMask = 0xff000000;

	for (int i = 0; i < width * height; ++i) {
		normalized[i] = (unsigned int)std::fminf(255.0f, pDepths[i] * 255);

		normalized[i] |= (normalized[i] << 16) | (normalized[i] << 8) | aMask;
	}

	SDL_Surface* surface = SDL_CreateRGBSurfaceFrom((void*)normalized, width, height, 32, width * 4, 0x000000ff, 0x0000ff00, 0x00ff0000, aMask);
	SDL_SaveBMP(surface, filename.c_str());
	SDL_FreeSurface(surface);

	delete[] normalized;


}

void Renderer::DepthBuffer::WhiteOut() {

	// random float I found that is a super big number 0x7a7a7a7a
	memset(pDepths, 0x7a, width * height * sizeof(float));

}
