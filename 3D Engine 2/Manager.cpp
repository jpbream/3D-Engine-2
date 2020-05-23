#include "Manager.h"
#include <thread>
#include <SDL.h>
#include <iostream>

// true while the front buffer is in process of being copied to the window
static volatile bool drawing = false;

// true while the renderer is in process of drawing onto the back buffer,
// and while events are being handled
static volatile bool rendering = false;

static volatile bool shouldQuit = false;

static Window* pWindow = nullptr;

static Surface* pFrontBuffer = nullptr;
static Surface* pBackBuffer = nullptr;

static EventHandler_T pEventHandler;
static ProgramLogic_T pProgramLogic;
static PostProcess_T pPostProcess;

static void DrawLoop() {

	// while the render loop has not signaled
	// to exit the program
	while (!shouldQuit) {
		
		drawing = true;

		// call the post processing function
		shouldQuit = pPostProcess(*pFrontBuffer);

		// draw to the window
		pWindow->DrawSurface(*pFrontBuffer);
		drawing = false;

		// wait for the renderer to finish
		while ( rendering );
	}
}

void StartDoubleBufferedInstance(Window& window, EventHandler_T eventHandler, ProgramLogic_T programLogic, PostProcess_T postProcessing, short renderFlags)
{
	pWindow = &window;
	pEventHandler = eventHandler;
	pProgramLogic = programLogic;
	pPostProcess = postProcessing;

	Surface s1(window.GetWidth() * REL_RES, window.GetHeight() * REL_RES);
	Surface s2(window.GetWidth() * REL_RES, window.GetHeight() * REL_RES);

	Queue<int> commandQueue;

	pFrontBuffer = &s1;
	pBackBuffer = &s2;

	// start the draw loop in a new thread
	std::thread drawLoop(DrawLoop);

	Renderer renderer(*pBackBuffer);
	renderer.SetFlags(renderFlags);

	float deltaTime = 0;
	while ( !shouldQuit ) {

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
		shouldQuit = pProgramLogic(renderer, deltaTime);

		renderer.ClearDepthBuffer();

#ifdef DIAGNOSTICS
		Uint64 renderEnd = SDL_GetPerformanceCounter();
		totalRenderTime += (renderEnd - start) / (double)SDL_GetPerformanceFrequency();
#endif

		// if drawing is not done, it is taking longer than render
		// wait for drawing to finish before swapping the buffers,

		while ( drawing );

		// swap the buffers
		std::swap(pFrontBuffer, pBackBuffer);
		renderer.SetRenderTarget(*pBackBuffer);

		// handle events before drawing or rendering
		// can start up again
		shouldQuit = pEventHandler(commandQueue);

		// process all commands in the command queue
		while ( !commandQueue.IsEmpty() ) {

			int commandId = commandQueue.Dequeue();

			switch ( commandId ) {

			case C_RESIZE:
			{
				// width and height should be the next 2 values in the queue
				int newWidth = commandQueue.Dequeue();
				int newHeight = commandQueue.Dequeue();

				// need to resize both screen buffers and the renderer's depth map
				pFrontBuffer->Resize(newWidth, newHeight, false);
				pBackBuffer->Resize(newWidth, newHeight, false);
				renderer.GetDepthBuffer().Resize(newWidth, newHeight);

				break;
			}

			}

		}

		rendering = false;

		Uint64 end = SDL_GetPerformanceCounter();
		static Uint64 interval = SDL_GetPerformanceFrequency();


		deltaTime = (end - start) / (float)interval;

#ifdef DIAGNOSTICS
		totalTime += deltaTime;
		frames++;

		if ( totalTime > 1.0 ) {
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
