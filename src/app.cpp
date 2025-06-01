#include "app.hpp"
#include "debug.hpp"
#include "box.hpp"
#include "uistate.hpp"

#include <exception>
#include <string>
#include <format>

#include <SDL.h>
#include "glad/glad.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"

int App::Main(int argc, char *argv[])
{
	App app;
	return app.Run();
}

int App::Run()
{
	int exitCode = 0;
	StartupContext c(*this);

	try {
		Startup(c);
		MainLoop(c);
	}
	catch (std::exception &e) {
		trace(std::format("Exception caught: {}", e.what()));
		exitCode = 1;
	}
	catch (...) {
		trace("Unknown exception caught.");
		exitCode = 2;
	}

	// Shutdown(c) will be called when StartupContext leaves scope.

    return exitCode;
}

App::StartupContext::StartupContext(App &app) : _app(app) {}
App::StartupContext::~StartupContext() {_app.Shutdown(*this);}

void App::Startup(StartupContext &c)
{
	trace("=== Startup ===");

	// SDL setup
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		throw std::runtime_error(std::format("SDL_Init(SDL_INIT_VIDEO) failed: {}", SDL_GetError()));
	}
	c.sdlInitialized = true;

	c.window = SDL_CreateWindow(
		"Mirael",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		800, 600,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
	);
	if (!c.window) {
		throw std::runtime_error(std::format("SDL_CreateWindow(\"Mirael\",...) failed: {}", SDL_GetError()));
	}

	// OpenGL setup
	c.gl_context = SDL_GL_CreateContext(c.window);
	if (!c.gl_context) {
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
	if (SDL_GL_SetSwapInterval(1) != 0) {
		trace("WARNING: Unable to enable vsync.");
	}

	// imgui setup
	IMGUI_CHECKVERSION();
	c.imGuiContext = ImGui::CreateContext();
	if (!c.imGuiContext) {
		throw std::runtime_error("ImGui::CreateContext() failed.");
	}
	ImGui::StyleColorsDark();

	if (!ImGui_ImplSDL2_InitForOpenGL(c.window, c.gl_context)) {
		throw std::runtime_error("ImGui_ImplSDL2_InitForOpenGL(window, gl_context) failed.");
	}
	c.implSDL2Initialized = true;

	if (!ImGui_ImplOpenGL3_Init("#version 130")) {
		throw std::runtime_error("ImGui_ImplOpenGL3_Init(\"#version 130\") failed.");
	}
	c.implOpenGL3Initialized = true;
}

// helper methods used by MainLoop()
void ShowFramerateWindow(UiState &uiState);
void ShowOptionsWindow(SDL_Window *window, UiState &uiState, Box &box);
void ShowAboutWindow(UiState &uiState);

void App::MainLoop(StartupContext &c)
{
	trace("=== Main Loop ===");

	ImGuiIO& io = ImGui::GetIO();
	bool running = true;
	SDL_Event e;
	Box box = { 100, 100, 200, 150 };

	// main loop
	UiState uiState;
	while (running) {

		// get all events for this frame
		while (SDL_PollEvent(&e)) {

			bool imGuiHandled = ImGui_ImplSDL2_ProcessEvent(&e);

			if (e.type == SDL_QUIT)
			{
				trace("SDL_QUIT event received.");
				running = false;
			}

			if (!io.WantCaptureKeyboard && !io.WantCaptureMouse)
				box.HandleEvent(e);
		}

		// start imgui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		// clear screen with deep blue
		glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// show the UI, which begins with the Options Window
		ShowOptionsWindow(c.window, uiState, box);
		if (uiState.showDebug) {
			ImGui::ShowDemoWindow(&uiState.showDebug);
		}
		if (uiState.showFramerate) {
			ShowFramerateWindow(uiState);
		}
		if (uiState.showAbout) {
			ShowAboutWindow(uiState);
		}

		// draw the box
		Graphics g;
		box.Render(g);

		// render the frame
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(c.window);
	}
}

void App::Shutdown(StartupContext &c)
{
	trace("=== Shutdown ===");

	// cleanup everything established during Startup() in reverse order.

	if (c.implOpenGL3Initialized)
		ImGui_ImplOpenGL3_Shutdown();

	if (c.implSDL2Initialized)
		ImGui_ImplSDL2_Shutdown();

	if (c.imGuiContext)
		ImGui::DestroyContext(c.imGuiContext);

	if (c.gl_context)
		SDL_GL_DeleteContext(c.gl_context);

	if (c.window)
		SDL_DestroyWindow(c.window);

	if (c.sdlInitialized)
		SDL_Quit();

	trace("Shutdown complete.");
}

void ShowFramerateWindow(UiState &uiState)
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

void ToggleFullscreen(SDL_Window *window, bool fullscreen)
{
	if (fullscreen) {
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	} else {
		SDL_SetWindowFullscreen(window, 0);
	}
}

void ShowOptionsWindow(SDL_Window *window, UiState &uiState, Box &box)
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
		if (ImGui::BeginMenu("View")) {
			if (ImGui::MenuItem("Fullscreen", nullptr, &uiState.fullscreen)) {
				ToggleFullscreen(window, uiState.fullscreen);
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Debug")) {
			ImGui::MenuItem("Show ImGui Demo", nullptr, &uiState.showDebug);
			ImGui::MenuItem("Show Framerate", nullptr, &uiState.showFramerate);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help")) {
			ImGui::MenuItem("About", nullptr, &uiState.showAbout);
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	if (ImGui::Button("Reset Box Position"))
		box.ResetPosition();

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

void ShowAboutWindow(UiState &uiState)
{
	if (ImGui::Begin("About Mirael", &uiState.showAbout, ImGuiWindowFlags_AlwaysAutoResize)) {

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
