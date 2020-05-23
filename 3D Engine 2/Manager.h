#pragma once
#include "Window.h"
#include "Surface.h"
#include "Renderer.h"
#include "Queue.h"

#define REL_RES 1
#define DIAGNOSTICS

enum Commands {

	C_RESIZE

};

// types for function callbacks
using EventHandler_T = bool (*)(Queue<int>& commandQueue);
using ProgramLogic_T = bool (*)(Renderer & renderer, float deltaTime);
using PostProcess_T = bool (*)(Surface & frontBuffer);

void StartDoubleBufferedInstance(
	Window& window,
	EventHandler_T eventHandler,
	ProgramLogic_T programLogic, 
	PostProcess_T postProcessing, 
	short renderFlags
);