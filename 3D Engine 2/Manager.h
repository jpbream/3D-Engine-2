#pragma once
#include "Window.h"
#include "Surface.h"
#include "Renderer.h"

#define REL_RES 1
#define DIAGNOSTICS

void StartSingleBufferedInstance(Window& window, bool (*ProgramLogic)(Surface& backBuffer, Renderer& renderer, float deltaTime), short renderFlags);
void StartDoubleBufferedInstance(Window& window, bool (*ProgramLogic)(Renderer& renderer, float deltaTime), void (*PostProcessing)(Surface& frontBuffer), short renderFlags);