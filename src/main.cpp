#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "ui/app.hpp"

int main(int argc, char* argv[]) {
	return App::Main(argc, argv);
}
