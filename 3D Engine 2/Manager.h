#pragma once
#include "Window.h"
#include "Surface.h"
#include "Renderer.h"

#define DIAGNOSTICS

void StartDoubleBufferedInstance(Window& window, bool (*ProgramLogic)(Surface& backBuffer, Renderer& renderer, float deltaTime), short renderFlags);