#include "Window.h"
#include <SDL.h>
#include <iostream>

Window::Window(const char* title, int x, int y, int width, int height, int options)
 {

	pWindow = SDL_CreateWindow(title, x, y, width, height, options);
	pRenderer = SDL_CreateRenderer(pWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

}

void Window::DrawSurface(const Surface& surface) {

	SDL_Surface* pSurface = SDL_CreateRGBSurfaceFrom(
		(void*)surface.GetPixels(),
		surface.GetWidth(),
		surface.GetHeight(),
		Surface::BPP,
		surface.GetPitch(),
		surface.GetRMask(),
		surface.GetGMask(),
		surface.GetBMask(),
		surface.GetAMask()
	);

	SDL_Texture* texture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
	SDL_FreeSurface(pSurface);

	SDL_RenderClear(pRenderer);
	SDL_RenderCopy(pRenderer, texture, NULL, NULL);
	SDL_RenderPresent(pRenderer);
	SDL_DestroyTexture(texture);

}

Window::~Window() {

	SDL_DestroyRenderer(pRenderer);
	SDL_DestroyWindow(pWindow);

}

int Window::GetWidth() const {

	int width, height;
	SDL_GetWindowSize(pWindow, &width, &height);
	return width;

}

int Window::GetHeight() const {

	int width, height;
	SDL_GetWindowSize(pWindow, &width, &height);
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
