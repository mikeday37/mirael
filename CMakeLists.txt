cmake_minimum_required(VERSION 3.15)
project(mirael VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)


# === External Paths ===

# -- ImGui --
set(IMGUI_SOURCES
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
    external/imgui/backends/imgui_impl_sdl2.cpp
)
set(IMGUI_INCLUDE_DIRS
    external/imgui
    external/imgui/backends
)

# -- GLAD --
set(GLAD_SOURCE
    external/glad/src/glad.c
)
set(GLAD_INCLUDE_DIR
    external/glad/include
)

# -- GLM --
set(GLM_INCLUDE_DIR
    external/glm
)


# === SDL2 ===

include(FetchContent)

# Set minimal SDL2 features for OpenGL + ImGui GUI only
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
set(SDL_POWER OFF CACHE BOOL "" FORCE)
set(SDL_FILESYSTEM OFF CACHE BOOL "" FORCE)
set(SDL_THREADS ON CACHE BOOL "" FORCE)
set(SDL_TIMERS ON CACHE BOOL "" FORCE)       # Needed for ImGui/SDL2 backend
set(SDL_VIDEO ON CACHE BOOL "" FORCE)        # Needed for OpenGL windowing
set(SDL_RENDER OFF CACHE BOOL "" FORCE)      # ImGui uses OpenGL directly

FetchContent_Declare(
  SDL2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-2.32.8  # Lock to a specific release
)

FetchContent_MakeAvailable(SDL2)


# === Externals Library ===

add_library(externals_lib STATIC
    ${IMGUI_SOURCES}
    ${GLAD_SOURCE}
)

target_include_directories(externals_lib
    PUBLIC
        ${IMGUI_INCLUDE_DIRS}
        ${GLAD_INCLUDE_DIR}
        ${GLM_INCLUDE_DIR}
        $<TARGET_PROPERTY:SDL2::SDL2,INTERFACE_INCLUDE_DIRECTORIES>
)
target_link_libraries(externals_lib
    PRIVATE
        opengl32
)


# === App Library ===

file(GLOB_RECURSE APP_SOURCES "src/app/*.cpp")
set(APP_LIB_INCLUDE_DIRS
    ${IMGUI_INCLUDE_DIRS}
    ${GLAD_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
)

add_library(app_lib STATIC
    ${APP_SOURCES}
)

target_precompile_headers(app_lib
    PRIVATE
        src/app/app_pch.hpp
)
target_include_directories(app_lib
    PRIVATE
        ${APP_LIB_INCLUDE_DIRS}
    PUBLIC
        include
)
target_link_libraries(app_lib
    PRIVATE
        externals_lib
        SDL2::SDL2
)


# === Main Executable ===

add_executable(mirael
    src/main.cpp
)

target_include_directories(mirael
    PRIVATE
        include
        ${APP_LIB_INCLUDE_DIRS}
)
target_link_libraries(mirael
    app_lib
    externals_lib
)

if (NOT MIRAEL_DISABLE_INSTALL)
    install(TARGETS mirael RUNTIME DESTINATION .)
endif()


# === Warning Level ===

if (MSVC)
    target_compile_options(app_lib PRIVATE /W4)
    target_compile_options(mirael PRIVATE /W4)
endif()


# === Unit Testing ===

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.8.1
)

FetchContent_MakeAvailable(Catch2)

file(GLOB_RECURSE MIRAEL_TEST_SOURCES "tests/*.cpp")

add_executable(mirael_tests
    ${MIRAEL_TEST_SOURCES}
)

target_include_directories(mirael_tests
    PRIVATE
        include
        ${APP_LIB_INCLUDE_DIRS}
)
target_link_directories(mirael_tests
    PRIVATE
        ${SDL2_LIB_DIR}
)
target_link_libraries(mirael_tests
    PRIVATE
        Catch2::Catch2WithMain
        app_lib
)

include(Catch)

add_test(NAME mirael_tests COMMAND mirael_tests)


# === Code Quality ===

file(GLOB_RECURSE QUALITY_CHECK_INCLUDES
    "${CMAKE_SOURCE_DIR}/include/app/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/app/app_pch.hpp"
	"${CMAKE_SOURCE_DIR}/tests/*.hpp"
)

file(GLOB_RECURSE QUALITY_CHECK_SOURCES
    "${CMAKE_SOURCE_DIR}/src/app/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/main.cpp"
	"${CMAKE_SOURCE_DIR}/tests/*.cpp"
)

set(QUALITY_CHECK_ALL
    ${QUALITY_CHECK_INCLUDES}
    ${QUALITY_CHECK_SOURCES}
)

find_program(CLANG_FORMAT_EXE NAMES clang-format)

add_custom_target(clang_format_check
    COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror --Wclang-format-violations -style=file ${QUALITY_CHECK_ALL}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Checking code format with clang-format"
)

add_custom_target(clang_format_apply
    COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${QUALITY_CHECK_ALL}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Applying code format with clang-format"
)

find_program(CLANG_TIDY_EXE NAMES clang-tidy)

add_custom_target(clang_tidy
    COMMAND ${CLANG_TIDY_EXE} -p=${CMAKE_BINARY_DIR} -extra-arg=-Xclang -extra-arg=-Winvalid-pch ${QUALITY_CHECK_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-tidy"
    VERBATIM
)


# === Build Metadata ===

string(TIMESTAMP MIRAEL_BUILD_CONFIGURED_TIMESTAMP "%Y-%m-%d %H:%M:%S")
add_compile_definitions(MIRAEL_BUILD_CONFIGURED_TIMESTAMP="${MIRAEL_BUILD_CONFIGURED_TIMESTAMP}")

if(DEFINED ENV{MIRAEL_BUILD_PRESET})
    add_compile_definitions(MIRAEL_BUILD_PRESET="$ENV{MIRAEL_BUILD_PRESET}")
else()
    add_compile_definitions(MIRAEL_BUILD_PRESET="unknown")
endif()

if (CMAKE_BUILD_TYPE)
    add_compile_definitions(MIRAEL_BUILD_TYPE="${CMAKE_BUILD_TYPE}")
else()
    add_compile_definitions(MIRAEL_BUILD_TYPE="unknown")
endif()
