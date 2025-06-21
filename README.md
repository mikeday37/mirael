# Mirael

A modern C++ playground for experimenting with algorithms and real-time visualizations.

This is a work-in-progress, currently built and tested only with MSVC on Windows.  Support for Linux and the Clang and GCC compilers is planned.

## Table of Contents

- [Purpose](#purpose)
- [Intended Use](#intended-use)
- [Status](#status)
- [Getting Started](#getting-started)
- [What's Next?](#whats-next)
- [Roadmap](#roadmap)
- [Contributing](#contributing)
- [Tech stack](#tech-stack)
- [Licensing](#licensing)

## Purpose

Mirael enables C++ developers to quickly and easily create and run high-performance Applets that allow real-time interaction and live re-configuration.  Mirael provides the scaffolding to get up and running quickly with SDL2, OpenGL and ImGui as easily as defining a single Applet-derived class.

The scaffolding includes a complete IDE setup for VS Code with integrated building, debugging, unit testing, and (optionally) linting.

## Intended Use

Fork the repo and define one or more classes derived from `Applet`.  Register them in `include/app/known_applets.hpp`, then rebuild and run.  Your applets will appear in the Applet menu of the Controls window.

More detailed instructions for developing applets will be found in `docs/AppletDev.md` (coming soon).

## Status

This is currently a solo project in the early stages.  When it's far enough along, I'll move the Applets I am making to a fork so that Mirael may remain of general use to C++ developers.

The Untangle Applet I'm making is for creating, editing, manipulating, and animating undirected graphs.  It already demonstrates some unexpectedly compelling graphical effects at high performance, even with the small feature set.

The groundwork is being laid for experimenting with a physics-based approach to finding aesthetically pleasing, arbitrary planar graph embeddings (an unsolved problem).

## Getting Started

See: [docs/GettingStarted.md](docs/GettingStarted.md)

## What's Next?

I'm preparing to add Eigen to the stack, isolated in a `sim_lib` separate from the `app_lib` in which the bulk of the current code resides.

### Why both GLM and Eigen?

GLM is great for graphics-focused code, especially with OpenGL.  I use it in the app_lib where the focus is on the user interface and rendering.

Eigen is far more powerful and able to take advantage of SIMD acceleration for matrix operations larger than what GLM can handle, but is more complicated.  It is well-suited for the heavy calculations I expect in the `sim_lib`.

The architecture of Mirael will be expanded to run Sim classes on a background thread, to enable very computation-heavy algorithms to be explored with a UI that is still smooth and responsive.

## Roadmap

1. Eigen Integration
2. Multithreaded, Extensible Sim Engine
3. Untangle Physics Simulation
4. Python Scripting
5. Clang Support
6. Linux Support
7. GCC Support

## Contributing

Pull Requests should be limited to scaffolding improvements and bugfixes.  New Applets should remain in your fork, unless you want to share a really good tutorial Applet, demo showcase Applet, or other Applet of general usefulness to Mirael users.

If you are interested in Mirael but do not develop on Windows with MSVC, I would love to collaborate with you on accelerating the planned support goals.  To start that process, open an Issue.

## Tech stack

### Tools:

|Aspect|Tool|
|-|-|
|IDE|[VS Code](https://code.visualstudio.com/docs)|
|Language|[C++20](https://en.cppreference.com/w/cpp/20)|
|Operating System|[Windows](https://learn.microsoft.com/en-us/windows/)<br>(support for Linux is planned)|
|Compiler|[MSVC](https://learn.microsoft.com/en-us/cpp/?view=msvc-170)<br>(support for Clang and GCC is planned)|
|Build|[CMake](https://cmake.org/documentation/) + [Ninja](https://github.com/ninja-build/ninja#readme)|
|Code Styling|[clang-format](https://clang.llvm.org/docs/ClangFormat.html)|
|Linter|[clang-tidy](https://clang.llvm.org/extra/clang-tidy/)|
|Scripting|None yet. Python is planned.|

### Libraries:

|Aspect|Library|
|-|-|
|Application Base|[SDL2](https://wiki.libsdl.org/SDL2/FrontPage)|
|UI|[Dear ImGui](https://github.com/ocornut/imgui#readme)|
|Graphics|[OpenGL](https://www.opengl.org/)<br>Loader: [GLAD](https://github.com/Dav1dde/glad#readme)|
|Math|[GLM](https://github.com/g-truc/glm#readme)|
|Testing|[Catch2](https://github.com/catchorg/Catch2)|

### Future / Under Consideration

- [Eigen](https://eigen.tuxfamily.org/)
- [nlohmann/json](https://github.com/nlohmann/json#readme)
- [LodePNG](https://github.com/lvandeve/lodepng#readme)
- [stb_image_write](https://github.com/nothings/stb#readme)
- [CGAL](https://www.cgal.org/)
- [EnTT](https://github.com/skypjack/entt#readme)

## Licensing

Mirael is licensed under the [MIT License](LICENSE.txt).

This project uses the following 3rd party libraries and acknowledges their licenses with thanks to their authors:
- SDL2 - [zlib License](LICENSES/SDL2_LICENSE.txt) - Sam Lantinga
- GLAD - [MIT License](LICENSES/GLAD_LICENSE.txt) - David Herberth
- Dear ImGui - [MIT License](LICENSES/Dear_ImGui_LICENSE.txt) - Omar Cornut
- GLM - [The Happy Bunny License or MIT License](LICENSES/GLM_License.txt) - G-Truc Creation
- Catch2 - [Boost Software License](LICENSES/Catch2_LICENSE.txt) - Martin Hořeňovský, Phil Nash, and Contributors
