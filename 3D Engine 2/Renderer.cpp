#include "Renderer.h"
#include "Vec2.h"

Renderer::Renderer(Surface& renderTarget) : pRenderTarget(&renderTarget), zBuffer(renderTarget.GetWidth(), renderTarget.GetHeight()) {

	zBuffer.WhiteOut();

}

void Renderer::SetRenderTarget(Surface& renderTarget) {

	if (renderTarget.GetWidth() == pRenderTarget->GetWidth() && renderTarget.GetHeight() == pRenderTarget->GetHeight()) {

		pRenderTarget = &renderTarget;
	}
}

bool Renderer::TestAndSetPixel(int x, int y, float normalizedDepth) {

	if (normalizedDepth < zBuffer.GetPixel(x, y).r) {

		zBuffer.PutPixel(x, y, normalizedDepth);
		return true;

	}
	return false;
}

void Renderer::ClearZBuffer() {

	zBuffer.WhiteOut();

}

void Renderer::SetFlags(short flags) {

	this->flags |= flags;

}

void Renderer::ClearFlags(short flags) {

	this->flags &= ~flags;

}
