cmake_minimum_required(VERSION 3.10)
project(mirael)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDL2_ROOT "D:/dev/libs/SDL2-2.32.6")
include_directories(${SDL2_ROOT}/include)
link_directories(${SDL2_ROOT}/lib/x64)

include_directories(include)
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(mirael src/main.cpp ${SOURCES})
target_link_libraries(mirael SDL2 SDL2main)

# Copy the SDL2.dll to the output directory after building
set(SDL2_DLL "${SDL2_ROOT}/lib/x64/SDL2.dll")
if(EXISTS ${SDL2_DLL})
	add_custom_command(TARGET mirael POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${SDL2_DLL}" $<TARGET_FILE_DIR:mirael>)
else()
	message(FATAL_ERROR "SDL2.dll not found in ${SDL2_DLL}")
endif()
