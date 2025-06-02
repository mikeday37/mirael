#include "app.hpp"
#include "debug.hpp"
#include "box.hpp"
#include "framelimiter.hpp"

#include <exception>
#include <string>
#include <format>
#include <vector>
#include <unordered_set>
#include <ranges>

#include <SDL.h>
#include "glad/glad.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"

int App::Main(int argc, char *argv[])
{
	App app;
	int exitCode = 0;

	try {
		app.Startup();
		app.MainLoop();
	}
	catch (std::exception &e) {
		trace(std::format("Exception caught: {}", e.what()));
		exitCode = 1;
	}
	catch (...) {
		trace("Unknown exception caught.");
		exitCode = 2;
	}

	// Shutdown() will be called when App leaves scope.

    return exitCode;
}

App::App() :
	boxApplet_(*this),
	testApplet1_(*this),
	testApplet2_(*this)
{
}

App::~App() {
	try {
		Shutdown();
	}
	catch (std::exception &e) {
		trace(std::format("Exception caught during Shutdown(): {}", e.what()));
	}
	catch (...) {
		trace("Unknown exception caught during Shutdown().");
	}
}

void App::Startup()
{
	trace("=== Startup ===");

	// SDL setup
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		throw std::runtime_error(std::format("SDL_Init(SDL_INIT_VIDEO) failed: {}", SDL_GetError()));
	}
	sdlInitialized_ = true;

	window_ = SDL_CreateWindow(
		"Mirael",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		800, 600,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
	);
	if (!window_) {
		throw std::runtime_error(std::format("SDL_CreateWindow(\"Mirael\",...) failed: {}", SDL_GetError()));
	}

	// OpenGL setup
	glContext_ = SDL_GL_CreateContext(window_);
	if (!glContext_) {
		throw std::runtime_error(std::format("SDL_GL_CreateContext(c.window) failed: {}", SDL_GetError()));
	}
	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
		throw std::runtime_error("gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress) failed.");
	}

	auto openGlVersion = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	auto openGlRenderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
	trace(std::format("INFO: GL_VERSION = {}", openGlVersion ? openGlVersion : "(null)"));
	trace(std::format("INFO: GL_RENDERER = {}", openGlRenderer ? openGlRenderer : "(null)"));

	// enable vsync
	ApplyVsyncMode();

	// imgui setup
	IMGUI_CHECKVERSION();
	imGuiContext_ = ImGui::CreateContext();
	if (!imGuiContext_) {
		throw std::runtime_error("ImGui::CreateContext() failed.");
	}
	ImGui::StyleColorsDark();

	if (!ImGui_ImplSDL2_InitForOpenGL(window_, glContext_)) {
		throw std::runtime_error("ImGui_ImplSDL2_InitForOpenGL(window, gl_context) failed.");
	}
	implSDL2Initialized_ = true;

	if (!ImGui_ImplOpenGL3_Init("#version 130")) {
		throw std::runtime_error("ImGui_ImplOpenGL3_Init(\"#version 130\") failed.");
	}
	implOpenGL3Initialized_ = true;

	// applet setup
	RegisterApplets({
		&boxApplet_,
		&testApplet1_,
		&testApplet2_
	});
	for (auto a : applets_)
		a->OnStartup();
}

void App::Shutdown()
{
	trace("=== Shutdown ===");

	// cleanup everything established during Startup() in reverse order.

	for (auto a : std::views::reverse(applets_))
		a->OnShutdown();

	if (implOpenGL3Initialized_)
		ImGui_ImplOpenGL3_Shutdown();

	if (implSDL2Initialized_)
		ImGui_ImplSDL2_Shutdown();

	if (imGuiContext_)
		ImGui::DestroyContext(imGuiContext_);

	if (glContext_)
		SDL_GL_DeleteContext(glContext_);

	if (window_)
		SDL_DestroyWindow(window_);

	if (sdlInitialized_)
		SDL_Quit();

	trace("Shutdown complete.");
}

void App::MainLoop()
{
	trace("=== Main Loop ===");

	ImGuiIO& io = ImGui::GetIO();
	bool running = true;
	SDL_Event e;
	Box box = { 100, 100, 200, 150 };
	FrameLimiter frameLimiter;

	// main loop
	while (running) {

		// mark frame start time
		frameLimiter.StartFrame();

		// get all events for this frame
		while (SDL_PollEvent(&e)) {

			bool imGuiHandled = ImGui_ImplSDL2_ProcessEvent(&e);

			if (e.type == SDL_QUIT)
			{
				trace("SDL_QUIT event received.");
				running = false;
			}

			if (currentApplet_ && !io.WantCaptureKeyboard && !io.WantCaptureMouse)
				currentApplet_->OnEvent(e);
		}

		// start imgui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		// clear screen with deep blue
		glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// allow current applet to draw on the background
		if (currentApplet_) {
			Graphics g(ImGui::GetBackgroundDrawList());
			currentApplet_->OnRenderBackground(g);
		}

		// show the UI, which begins with the Options Window
		ShowControlsWindow();
		if (showDebug_) {
			ImGui::ShowDemoWindow(&showDebug_);
		}
		if (showFramerate_) {
			ShowFramerateWindow();
		}
		if (showAbout_) {
			ShowAboutWindow();
		}

		// allow current applet to draw on the foreground
		if (currentApplet_) {
			Graphics g(ImGui::GetForegroundDrawList());
			currentApplet_->OnRenderForeground(g);
		}

		// render the frame
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// limit framerate only if vsync is off
		if (vsyncMode_ == VsyncMode::Off) {
			frameLimiter.EndFrame();
		}

		// swap buffers to display the frame
		SDL_GL_SwapWindow(window_);
	}
}

void App::ApplyFullscreenOption()
{
	if (fullscreen_) {
		SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
	} else {
		SDL_SetWindowFullscreen(window_, 0);
	}
	ApplyVsyncMode();
}

void App::ShowControlsWindow()
{
	if (!ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NavFlattened)) {
		ImGui::End();
		return;
	}
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Exit")) {
				SDL_Event e;
				e.type = SDL_QUIT;
				SDL_PushEvent(&e);
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Applet")) {
			for (auto applet : applets_) {
				bool selected = (applet == currentApplet_);
				if (ImGui::MenuItem(applet->GetDisplayName().data(), nullptr, selected)) {
					SetCurrentApplet(applet);
				}
			}
			ImGui::EndMenu();
		}
		if (currentApplet_) {
			currentApplet_->OnShowMenu();
		}
		if (ImGui::BeginMenu("View")) {
			if (ImGui::MenuItem("Fullscreen", nullptr, &fullscreen_)) {
				ApplyFullscreenOption();
			}
			if (ImGui::BeginMenu("Vsync")) {
				auto vsyncMenuItem = [this](const char *name, VsyncMode vsyncMode) -> void {
					if (ImGui::MenuItem(name, nullptr, vsyncMode_ == vsyncMode)) {
						vsyncMode_ = vsyncMode;
						ApplyVsyncMode();
					}
				};
				vsyncMenuItem("Off", VsyncMode::Off);
				vsyncMenuItem("Double", VsyncMode::Double);
				vsyncMenuItem("Triple", VsyncMode::Triple);
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Debug")) {
			ImGui::MenuItem("Show ImGui Demo", nullptr, &showDebug_);
			ImGui::MenuItem("Show Framerate", nullptr, &showFramerate_);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help")) {
			ImGui::MenuItem("About", nullptr, &showAbout_);
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	if (currentApplet_) {
		currentApplet_->OnShowControls();
	}

	ImGui::End();
}

void App::ShowFramerateWindow()
{
	if (ImGui::Begin("Framerate", nullptr,
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoBackground
		)) {

		const auto cursorPos = ImGui::GetCursorScreenPos();
		const auto pos = ImGui::GetWindowPos();
		const auto size = ImGui::GetWindowSize();

		// draw the text in-window to autosize it
		const auto text = std::format("FPS: {:.1f}", ImGui::GetIO().Framerate);
		ImGui::TextColored(ImVec4(0, 1.0f, 0, 1.0f), text.c_str());

		// now redraw the background and text over everything so it's always visible
		auto drawList = ImGui::GetForegroundDrawList();
		drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(0,0,0,205));
		drawList->AddText(
				ImVec2(cursorPos.x, cursorPos.y),
				IM_COL32(0, 255, 0, 255),
				text.c_str());
	}
	ImGui::End();
}

void AttributionLine(const char *library, const char *libraryUrl, const char *license, const char *licenseUrl, const char *author)
{
		ImGui::TextLinkOpenURL(library, libraryUrl);
		ImGui::SameLine();
		ImGui::Text(" - ");
		ImGui::SameLine();
		ImGui::TextLinkOpenURL(license, licenseUrl);
		ImGui::SameLine();
		ImGui::Text(" - ");
		ImGui::SameLine();
		ImGui::Text(author);
}

void App::ShowAboutWindow()
{
	if (ImGui::Begin("About Mirael", &showAbout_, ImGuiWindowFlags_AlwaysAutoResize)) {

		ImGui::Text("");
		ImGui::Text("(c) 2025 Mike Day");
		ImGui::Text("");
    	ImGui::Separator();
		ImGui::Text("");
		ImGui::Text("Mirael is my modern C++ playground for experimenting with");
		ImGui::Text("algorithms and real-time visualizations.");
		ImGui::Text("");
		ImGui::Separator();
		ImGui::Text("");
		ImGui::Text("This project uses the following 3rd party libraries and");
		ImGui::Text("acknowledges their licenses with thanks to their authors:");
		ImGui::Text("");

		AttributionLine(
			"SDL2", "https://wiki.libsdl.org/SDL2/FrontPage",
			"zlib license", "https://github.com/libsdl-org/SDL/blob/3a4de2ad89606e666168677ab00bcab2b9d2e734/LICENSE.txt",
			"Sam Lantinga"
		);

		AttributionLine(
			"GLAD", "https://github.com/Dav1dde/glad/tree/master?tab=readme-ov-file",
			"MIT license##1", "https://github.com/Dav1dde/glad/blob/e86f90457371c6233053bacf0d6f486a51ddcd67/LICENSE",
			"David Herberth"
		);

		AttributionLine(
			"Dear ImGui", "https://github.com/ocornut/imgui",
			"MIT license##2", "https://github.com/ocornut/imgui/blob/69e1fb50cacbde1c2c585ae59898e68c1818d9b7/LICENSE.txt",
			"Omar Cornut"
		);

		ImGui::Text("");
	}

	ImGui::End();
}

void App::ApplyVsyncMode()
{
	int swapInterval = 0;
	switch (vsyncMode_) {
		case VsyncMode::Off: swapInterval = 0; break;
		case VsyncMode::Double: swapInterval = 1; break;
		case VsyncMode::Triple: swapInterval = -1; break;
	}
	if (SDL_GL_SetSwapInterval(swapInterval) != 0) {
		trace(std::format("ERROR: SDL_GL_SetSwapInterval({}) failed: {}", swapInterval, SDL_GetError()));
	}
}

void App::RegisterApplets(std::initializer_list<Applet *> applets)
{
	std::unordered_set<std::string> names;
	for (auto a : applets) {
		auto name = std::string(a->GetDisplayName());
		auto [_, inserted] = names.insert(name);
		if (!inserted) {
			throw std::runtime_error(std::format("Applet display name collision on: {}", name));
		}
		applets_.push_back(a);
	}
}

void App::SetCurrentApplet(Applet *applet)
{
	if (applet == currentApplet_)
		return;

	if (currentApplet_) {
		currentApplet_->OnLoseFocus();
	}

	currentApplet_ = applet;

	if (currentApplet_) {
		currentApplet_->OnGainFocus();
	}
}
