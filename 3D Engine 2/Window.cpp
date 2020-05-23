#include "Window.h"
#include <SDL.h>
#include <iostream>

#ifdef SDL_DEBUG
#define SDL_CALL(F) { F; if (strlen(SDL_GetError()) > 0) std::cout << #F << " returned the error: " << SDL_GetError() << std::endl; SDL_ClearError();}
#else
#define SDL_CALL(F) F
#endif

Window::Window(const char* title, int x, int y, int width, int height, int options)
 {

	SDL_CALL(pWindow = SDL_CreateWindow(title, x, y, width, height, options));
	SDL_CALL(pRenderer = SDL_CreateRenderer(pWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));


	lastWidth = width;
	lastHeight = height;
}

void Window::DrawSurface(const Surface& surface) {

	if ( lastWidth != GetWidth() || lastHeight != GetHeight() ) {

		// resize detected, this hack is needed because the renderer object
		// breaks when the window is resized, seems to be an SDL issue

		SDL_CALL(SDL_DestroyRenderer(pRenderer));
		SDL_CALL(pRenderer = SDL_CreateRenderer(pWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));

		lastWidth = GetWidth();
		lastHeight = GetHeight();
	}

	SDL_Surface* pSurface;
	
	SDL_CALL(pSurface = SDL_CreateRGBSurfaceFrom(
		(void*)surface.GetPixels(),
		surface.GetWidth(),
		surface.GetHeight(),
		Surface::BPP,
		surface.GetPitch(),
		surface.GetRMask(),
		surface.GetGMask(),
		surface.GetBMask(),
		surface.GetAMask()
	));

	SDL_Texture* texture;

	SDL_CALL(texture = SDL_CreateTextureFromSurface(pRenderer, pSurface));
	SDL_CALL(SDL_FreeSurface(pSurface));

	SDL_CALL(SDL_RenderClear(pRenderer));
	SDL_CALL(SDL_RenderCopy(pRenderer, texture, NULL, NULL));

	SDL_CALL(SDL_RenderPresent(pRenderer));
	SDL_CALL(SDL_DestroyTexture(texture));

}

Window::~Window() {

	SDL_CALL(SDL_DestroyRenderer(pRenderer));
	SDL_CALL(SDL_DestroyWindow(pWindow));

}

int Window::GetWidth() const {

	int width, height;
	SDL_CALL(SDL_GetWindowSize(pWindow, &width, &height));
	return width;

}

int Window::GetHeight() const {

	int width, height;
	SDL_CALL(SDL_GetWindowSize(pWindow, &width, &height));
	return height;

}

void Window::BlockUntilQuit() {

	// set up a dummy message loop to return when it gets a quit message
	SDL_Event event = {};
	while (SDL_WaitEvent(&event)) {

		switch (event.type) {

		case SDL_QUIT:
			return;

		}
	}

}
