#pragma once

#include <SDL.h>
#include "imgui.h"

class App {
public:
	[[nodiscard]] static int Main(int argc, char *argv[]);

private:
	int Run();

	struct StartupContext;

	void Startup(StartupContext &c);
	void MainLoop(StartupContext &c);
	void Shutdown(StartupContext &c);

	struct StartupContext {
		App &_app;

		StartupContext(App &app);
		~StartupContext();

		bool sdlInitialized = false;
		SDL_Window* window = NULL;
		SDL_GLContext gl_context = NULL;
		ImGuiContext* imGuiContext = NULL;
		bool implSDL2Initialized = false;
		bool implOpenGL3Initialized = false;
	};
};
