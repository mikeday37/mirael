#pragma once

#include "app/territoriesapplet.hpp"
#include "app/untangleapplet.hpp"

#include <vector>

#include <SDL.h>
#include "imgui.h"

class App {
public:
	[[nodiscard]] static int Main(int argc, char *argv[]);

	inline SDL_Window *GetWindow() {return window_;}

private:
	App();
	~App();

	void Startup();
	void Shutdown();

	void MainLoop();

	void ShowControlsWindow();
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
	void ApplyFullscreenOption();

	enum class VsyncMode {
		Off,
		Double,
		Triple
	};
	VsyncMode vsyncMode_ = VsyncMode::Off;
	void ApplyVsyncMode();

	// applet system
	std::vector<Applet *> applets_;
	Applet *currentApplet_ = nullptr;
	void RegisterApplets(std::initializer_list<Applet *> applets);
	void SetCurrentApplet(Applet *applet);

	// supported applets
	TerritoriesApplet territoriesApplet_;
	UntangleApplet untangleApplet_;
};
