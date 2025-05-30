#include <SDL.h>
#include "glad/glad.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"

#include <string>
#include <format>

#include "debug.hpp"
#include "box.hpp"

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
	while (running) {

		// get all events for this frame
		while (SDL_PollEvent(&e)) {
			ImGui_ImplSDL2_ProcessEvent(&e);

			if (e.type == SDL_QUIT)
				running = false;

			box.HandleEvent(e);
		}

		// start imgui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		// menu bar
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Box")) {
				if (ImGui::MenuItem("Reset Position")) {
					box.ResetPosition();
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		// TODO: add ImGui UI elements

		// clear screen with deep blue
		glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

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

