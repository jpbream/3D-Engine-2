#include "Renderer.h"
#include "Vec2.h"

Renderer::Renderer(Surface& renderTarget) : pRenderTarget(&renderTarget), zBuffer(renderTarget.GetWidth(), renderTarget.GetHeight()) {

	depthBuffer = new float[renderTarget.GetWidth() * renderTarget.GetHeight()];
	ClearZBuffer();

}

Renderer::~Renderer() {
	delete[] depthBuffer;
}

void Renderer::SetRenderTarget(Surface& renderTarget) {

	if (renderTarget.GetWidth() == pRenderTarget->GetWidth() && renderTarget.GetHeight() == pRenderTarget->GetHeight()) {

		pRenderTarget = &renderTarget;
	}
}

bool Renderer::TestAndSetPixel(int x, int y, float normalizedDepth) {

	if (normalizedDepth < depthBuffer[y * pRenderTarget->GetWidth() + x]) {
		depthBuffer[y * pRenderTarget->GetWidth() + x] = normalizedDepth;
		return true;
	}
	return false;
}

void Renderer::ClearZBuffer() {

	// random float I found that is a super big number 0x7a7a7a7a
	memset(depthBuffer, 0x7a, pRenderTarget->GetWidth() * pRenderTarget->GetHeight() * sizeof(float));

}

void Renderer::SetFlags(short flags) {

	this->flags |= flags;

}

void Renderer::ClearFlags(short flags) {

	this->flags &= ~flags;

}
