#pragma once
#include <SDL.h>
#include "Surface.h"
#include "Renderer.h"

#define SDL_DEBUG

class Window
{
private:
	SDL_Window* pWindow = nullptr;
	SDL_Renderer* pRenderer = nullptr;

	mutable int lastWidth;
	mutable int lastHeight;

public:
	Window(const char* title, int x, int y, int width, int height, int options);
	~Window();

	void DrawSurface(const Surface& surface);

	int GetWidth() const;
	int GetHeight() const;

	void BlockUntilQuit();

};

