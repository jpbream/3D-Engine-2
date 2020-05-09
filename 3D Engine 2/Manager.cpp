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

static void (*PostProcessingFunc)(Surface& frontBuffer);

static void DrawLoop() {

	// while the render loop has not signaled
	// to exit the program
	while (!shouldQuit) {

		drawing = true;

		// call the post processing function
		PostProcessingFunc(*pFrontBuffer);

		// draw to the window
		pWindow->DrawSurface(*pFrontBuffer);
		drawing = false;

		// wait for the renderer to finish
		while (rendering);
	}
}

void StartDoubleBufferedInstance(Window& window, bool(*ProgramLogic)(Renderer& renderer, float deltaTime), void (*PostProcessing)(Surface& frontBuffer), short renderFlags)
{
	pWindow = &window;

	Surface s1(window.GetWidth() * REL_RES, window.GetHeight() * REL_RES);
	Surface s2(window.GetWidth() * REL_RES, window.GetHeight() * REL_RES);

	pFrontBuffer = &s1;
	pBackBuffer = &s2;

	Renderer renderer(*pBackBuffer);
	renderer.SetFlags(renderFlags);

	PostProcessingFunc = PostProcessing;

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
		shouldQuit = ProgramLogic(renderer, deltaTime);

		renderer.ClearDepthBuffer();

#ifdef DIAGNOSTICS
		Uint64 renderEnd = SDL_GetPerformanceCounter();
		totalRenderTime += (renderEnd - start) / (double)SDL_GetPerformanceFrequency();
#endif

		// if drawing is not done, it is taking longer than render
		// wait for drawing to finish before swapping the buffers,

		while (drawing);

		// swap the buffers
		std::swap(pFrontBuffer, pBackBuffer);
		renderer.SetRenderTarget(*pBackBuffer);

		rendering = false;

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

void StartSingleBufferedInstance(Window& window, bool (*ProgramLogic)(Surface& backBuffer, Renderer& renderer, float deltaTime), short renderFlags) {

	Surface screenBuffer(window.GetWidth() * REL_RES, window.GetHeight() * REL_RES);

	Renderer renderer(screenBuffer);
	renderer.SetFlags(renderFlags);

	float deltaTime = 0;
	while (!shouldQuit) {

#ifdef DIAGNOSTICS
		static int frames = 0;
		static double totalTime = 0;
#endif

		Uint64 start = SDL_GetPerformanceCounter();

		//clear the buffer about to be drawn to
		screenBuffer.BlackOut();

		// call the program and render logic
		shouldQuit = ProgramLogic(screenBuffer, renderer, deltaTime);

		renderer.ClearDepthBuffer();

		// draw the frame buffer to the window
		window.DrawSurface(screenBuffer);

		Uint64 end = SDL_GetPerformanceCounter();
		static Uint64 interval = SDL_GetPerformanceFrequency();

		deltaTime = (end - start) / (double)interval;

#ifdef DIAGNOSTICS
		totalTime += deltaTime;
		frames++;

		if (totalTime > 1.0) {
			std::cout << "Frame Time: " << totalTime * 1000 / frames << " ms.   "
				<< frames << " frames.   " << std::endl;
			frames = 0;
			totalTime = 0;
		}
#endif

	}

}
