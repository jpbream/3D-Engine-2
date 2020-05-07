#pragma once
#include "Window.h"
#include "Surface.h"

#define DIAGNOSTICS

void StartDoubleBufferedInstance(const Window& window, bool (*ProgramLogic)(Surface* pBackBuffer, float deltaTime));