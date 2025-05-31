#include <SDL.h>
#include "glad/glad.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"

#include <string>
#include <format>

#include "debug.hpp"
#include "box.hpp"
#include "uistate.hpp"

void ShowFramerateWindow(UiState &uiState)
{
	if (ImGui::Begin("Framerate", NULL,
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoBackground
		)) {

		auto cursorPos = ImGui::GetCursorScreenPos();
		auto pos = ImGui::GetWindowPos();
		auto size = ImGui::GetWindowSize();

		// draw the text in-window to autosize it
		auto text = std::format("FPS: {:.1f}", ImGui::GetIO().Framerate);
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

void ShowOptionsWindow(UiState &uiState, Box &box)
{
	if (!ImGui::Begin("Controls", NULL, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NavFlattened)) {
		ImGui::End();
		return;
	}

	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("Debug")) {
			ImGui::Checkbox("Show ImGui Demo", &uiState.showDebug);
			ImGui::Checkbox("Show Framerate", &uiState.showFramerate);
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	if (ImGui::Button("Reset Box Position"))
		box.ResetPosition();

	ImGui::End();
}

int main(int argc, char* argv[]) {

	trace("=== Start ===");

	// SDL setup
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		trace("failed to init SDL");
		return 1;
	}

	SDL_Window* window = SDL_CreateWindow(
		"Mirael",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		800, 600,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
	);
	if (!window) {
		trace("failed to create window");
		SDL_Quit();
		return 1;
	}

	// OpenGL setup
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	if (!gl_context) {
		trace(std::format("failed to create OpenGL context.  Error: {}", SDL_GetError()));
		SDL_Quit();
		return 1;
	}
	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
		trace("failed to init GLAD");
		SDL_GL_DeleteContext(gl_context);
		SDL_Quit();
		return 1;
	}
	std::string openGlVersion = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	trace(std::format("INFO: GL_VERSION = {}", openGlVersion));

	// enable vsync
	if (SDL_GL_SetSwapInterval(1) != 0) {
		trace("WARNING: unable to enable vsync");
	}

	// imgui setup
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();  (void)io; // silence warnings - remove later if we need to use io to update config/etc.
	ImGui::StyleColorsDark();

	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init("#version 130");

	// main loop
	trace("entering main loop");

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
				running = false;

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
		ShowOptionsWindow(uiState, box);
		if (uiState.showDebug) {
			ImGui::ShowDemoWindow(&uiState.showDebug);
		}
		if (uiState.showFramerate) {
			ShowFramerateWindow(uiState);
		}

		// draw the box
		Graphics g;
		box.Render(g);

		// render the frame
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}

	trace("ending");

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	trace("=== End ===");

	return 0;
}
