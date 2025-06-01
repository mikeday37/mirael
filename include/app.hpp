#pragma once

#include "box.hpp"

#include <SDL.h>
#include "imgui.h"

class App {
public:
	[[nodiscard]] static int Main(int argc, char *argv[]);

private:
	App() = default;
	~App();

	void Startup();
	void Shutdown();

	void MainLoop();

	void ShowOptionsWindow(Box &box);
	void OnFullscreenToggled();
	void ShowFramerateWindow();
	void ShowAboutWindow();

	// subsystem setup
	bool sdlInitialized_ = false;
	SDL_Window* window_ = nullptr;
	SDL_GLContext glContext_ = nullptr;
	ImGuiContext* imGuiContext_ = nullptr;
	bool implSDL2Initialized_ = false;
	bool implOpenGL3Initialized_ = false;

	// app-level ui state
	bool showDebug_ = false;
	bool showFramerate_ = false;
	bool showAbout_ = false;
	bool fullscreen_ = false;
};
