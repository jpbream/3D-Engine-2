#include "Manager.h"
#include <thread>
#include <SDL.h>
#include <iostream>

static Surface* pFrontBuffer = nullptr;
static Surface* pBackBuffer = nullptr;

static volatile bool drawing = false;
static volatile bool rendering = false;
static volatile bool shouldQuit = false;

static Window* pWindow = nullptr;

static void DrawLoop() {

	// while the render loop has not signaled
	// to exit the program
	while (!shouldQuit) {

		drawing = true;
		pWindow->DrawSurface(*pFrontBuffer);
		drawing = false;

		// wait for the renderer to finish
		while (rendering);
	}
}

void StartDoubleBufferedInstance(Window& window, bool(*ProgramLogic)(Surface* pBackBuffer, float deltaTime))
{
	pWindow = &window;

	Surface s1(window.GetWidth(), window.GetHeight());
	Surface s2(window.GetWidth(), window.GetHeight());

	pFrontBuffer = &s1;
	pBackBuffer = &s2;

	// start the drawing loop in a new thread
	std::thread drawLoop(DrawLoop);

	float deltaTime = 0;
	while (!shouldQuit) {

#ifdef DIAGNOSTICS
		static double totalRenderTime = 0;
		static int frames = 0;
		static double totalTime = 0;
#endif

		Uint64 start = SDL_GetPerformanceCounter();
		
		rendering = true;

		//clear the buffer about to be drawn to
		pBackBuffer->BlackOut();

		// call the program and render logic
		shouldQuit = ProgramLogic(pBackBuffer, deltaTime);

		//swap the buffers
		std::swap(pFrontBuffer, pBackBuffer);

#ifdef DIAGNOSTICS
		Uint64 renderEnd = SDL_GetPerformanceCounter();
		totalRenderTime += (renderEnd - start) / (double)SDL_GetPerformanceFrequency();
#endif

		rendering = false;
		
		// wait for the drawing to finish
		while (drawing);

		Uint64 end = SDL_GetPerformanceCounter();
		static Uint64 interval = SDL_GetPerformanceFrequency();

		
		deltaTime = (end - start) / (double)interval;

#ifdef DIAGNOSTICS
		totalTime += deltaTime;
		frames++;

		if (totalTime > 1.0) {
			std::cout << "Frame Time: " << totalTime * 1000 / frames << " ms.   " 
				      << frames << " frames.   " << totalRenderTime * 1000 / frames 
				      << " ms render time.   (" << (int)(totalRenderTime * 100 / totalTime) << "%)" 
				      << std::endl;
			frames = 0;
			totalTime = 0;
			totalRenderTime = 0;
		}
#endif

	}

	drawLoop.join();
}
