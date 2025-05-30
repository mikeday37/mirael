#include <SDL.h>
#include "debug.hpp"
#include "box.hpp"

int main(int argc, char* argv[]) {

	trace("=== Start ===");

	if (SDL_Init(SDL_INIT_VIDEO) != 0){
		trace("failed to init SDL");
		return 1;
	}

	SDL_Window* window = SDL_CreateWindow(
		"Mirael",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		800, 600,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
	);

	if (!window) {
		trace("failed to create window");
		SDL_Quit();
		return 1;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	if (!renderer) {
		trace("failed to create window");
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}


	trace("entering main loop");

	bool running = true;
	SDL_Event e;
	SDL_Color boxColor = {0, 200, 200, 255};
	Box box = { 100, 100, 200, 150, boxColor };

	// main loop
	while (running) {

		// get all events for this frame
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) running = false;

			box.HandleEvent(e);
		}

		// bg color: deep blue
		SDL_SetRenderDrawColor(renderer, 32, 64, 128, 255);
		SDL_RenderClear(renderer);

		// draw the box
		box.Render(renderer);

		// render the frame
		SDL_RenderPresent(renderer);
	}

	trace("ending");

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	trace("=== End ===");

	return 0;
}

