#include "Instance.h"
#include <SDL.h>

void Instance::Init() {

	SDL_Init(SDL_INIT_EVERYTHING);

}

void Instance::Quit() {

	SDL_Quit();

}