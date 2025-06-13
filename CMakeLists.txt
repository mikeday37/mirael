cmake_minimum_required(VERSION 3.10)
project(mirael)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# SDL2 setup
set(SDL2_ROOT "D:/dev/libs/SDL2-2.32.6")
set(SDL2_INCLUDE_DIR ${SDL2_ROOT}/include)
set(SDL2_LIB_DIR ${SDL2_ROOT}/lib/x64)
set(SDL2_DLL ${SDL2_LIB_DIR}/SDL2.dll)

# ImGui sources
set(IMGUI_SOURCES
	external/imgui/imgui.cpp
	external/imgui/imgui_demo.cpp
	external/imgui/imgui_draw.cpp
	external/imgui/imgui_tables.cpp
	external/imgui/imgui_widgets.cpp
	external/imgui/backends/imgui_impl_opengl3.cpp
	external/imgui/backends/imgui_impl_sdl2.cpp
)

# GLAD source
set(GLAD_SOURCE external/glad/src/glad.c)


# === UI Object Library ===
file(GLOB_RECURSE APP_SOURCES "src/app/*.cpp")

add_library(app_obj OBJECT ${APP_SOURCES})

target_precompile_headers(app_obj PRIVATE src/app/app_pch.hpp)
target_include_directories(app_obj
	PRIVATE
		src
		${SDL2_INCLUDE_DIR}
		external/imgui
		external/imgui/backends
		external/glad/include
		external/glm
	PUBLIC
		include
)
target_link_libraries(app_obj PRIVATE SDL2 SDL2main opengl32)


# === Main Executable ===
add_executable(mirael src/main.cpp $<TARGET_OBJECTS:app_obj> ${IMGUI_SOURCES} ${GLAD_SOURCE})

target_include_directories(mirael PRIVATE
	include
	${SDL2_INCLUDE_DIR}
	external/imgui
	external/imgui/backends
	external/glad/include
	external/glm
)
target_link_directories(mirael PRIVATE ${SDL2_LIB_DIR})
target_link_libraries(mirael SDL2 SDL2main opengl32)


# === Post-Build ===
# copy the SDL2.dll to the output directory after building
if(EXISTS ${SDL2_DLL})
	add_custom_command(TARGET mirael POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${SDL2_DLL}" $<TARGET_FILE_DIR:mirael>)
else()
	message(FATAL_ERROR "SDL2.dll not found in ${SDL2_LIB_DIR}")
endif()


# === Unit Testing ===
file(GLOB_RECURSE MIRAEL_TEST_SOURCES "tests/*.cpp")

list(APPEND CMAKE_PREFIX_PATH "D:/dev/packages/Catch2_v3_2025_06_13_3013cb89")
find_package(Catch2 3 REQUIRED)

add_executable(mirael_tests
    ${MIRAEL_TEST_SOURCES}
)

target_link_libraries(mirael_tests PRIVATE Catch2::Catch2WithMain)

enable_testing()
include(CTest)
include(Catch)

catch_discover_tests(mirael_tests)
