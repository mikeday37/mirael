cmake_minimum_required (VERSION 3.12)

project("Mirael")


#
# === Compiler Setup ===
#

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()


#
# === Dependencies ===
#

find_package(glfw3 3.4 CONFIG REQUIRED)
find_package(glm 1.0 CONFIG REQUIRED)
find_package(nlohmann_json 3.12 CONFIG REQUIRED)
find_package(stduuid CONFIG REQUIRED)
find_package(Vulkan 1.4 REQUIRED)


#
# === Native File Dialog Extended ===
#

add_subdirectory("external/nfde")
set(NFDE_LIB_DEPENDENCIES ole32.lib uuid.lib shell32.lib) # TODO: portability


#
# === Shader Compiler ===
#

find_program(SLANGC_EXECUTABLE
	NAMES slangc slangc.exe
	HINTS
		"$ENV{VULKAN_SDK}/bin"
	REQUIRED
)

function (add_slang_shader_target TARGET)
	cmake_parse_arguments("SHADER" "" "" "SOURCES" ${ARGN})
	set(SHADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
	set(ENTRY_POINTS -entry vertMain -entry fragMain)
	add_custom_command (
		OUTPUT ${SHADERS_DIR}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
	)
	add_custom_command (
		OUTPUT ${SHADERS_DIR}/slang.spv
		COMMAND ${SLANGC_EXECUTABLE}
			${SHADER_SOURCES}
			-target spirv
			-profile spirv_1_4
			-emit-spirv-directly
			-fvk-use-entrypoint-name
			${ENTRY_POINTS}
			-o slang.spv
		WORKING_DIRECTORY ${SHADERS_DIR}
		DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
		COMMENT "Compiling Slang Shaders"
		VERBATIM
	)
	add_custom_target(${TARGET} DEPENDS ${SHADERS_DIR}/slang.spv)
endfunction()


#
# === Dear ImGui ===
#

set(IMGUI_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui")
file(GLOB IMGUI_SOURCES
	"${IMGUI_SRC_DIR}/*.cpp"
	"${IMGUI_SRC_DIR}/*.h"
	"${IMGUI_SRC_DIR}/backends/imgui_impl_vulkan.*"
	"${IMGUI_SRC_DIR}/backends/imgui_impl_glfw.*"
	"${IMGUI_SRC_DIR}/misc/cpp/imgui_stdlib.*"
)
set_source_files_properties(${IMGUI_SOURCES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
set(IMGUI_INCLUDES
	"${IMGUI_SRC_DIR}"
	"${IMGUI_SRC_DIR}/backends"
)


#
# === ImGui Node Editor ===
#

set(INE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui-node-editor")
file(GLOB INE_SOURCES
	"${INE_SRC_DIR}/*.cpp"
	"${INE_SRC_DIR}/utilities/*.cpp"
)
set_source_files_properties(${INE_SOURCES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
set(INE_INCLUDES
	"${INE_SRC_DIR}"
)


#
# === Other Includes ===
#

set(OTHER_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/external/other")


#
# === Mirael ===
#
set(MIRAEL_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(MIRAEL_RES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/res")
set(MIRAEL_PCH "${MIRAEL_SRC_DIR}/pch.h")

file(GLOB MIRAEL_SOURCES
	"${MIRAEL_SRC_DIR}/*.cpp"
	"${MIRAEL_SRC_DIR}/node_types/*.cpp"
	"${MIRAEL_SRC_DIR}/os_specific/*.cpp"
)
set(MIRAEL_INCLUDES
	"${MIRAEL_SRC_DIR}"
	"${MIRAEL_SRC_DIR}/node_types"
	"${MIRAEL_SRC_DIR}/os_specific"
	"${MIRAEL_RES_DIR}/win32" # TODO: portability
)
file(GLOB MIRAEL_EMBEDDED_RESOURCES
	"${MIRAEL_RES_DIR}/fonts/*.cpp"
	"${MIRAEL_RES_DIR}/win32/mirael.rc" # TODO: portability
)

add_executable (Mirael
	${MIRAEL_SOURCES}
	${IMGUI_SOURCES}
	${INE_SOURCES}
	${MIRAEL_EMBEDDED_RESOURCES}
)
target_precompile_headers(Mirael PRIVATE ${MIRAEL_PCH})
set_property(TARGET Mirael PROPERTY CXX_STANDARD 20)
target_link_libraries(Mirael PRIVATE
	glfw
	glm::glm
	nfd
	nlohmann_json::nlohmann_json
	stduuid
	Vulkan::Vulkan
	${NFDE_LIB_DEPENDENCIES}
	dwmapi.lib # TODO: portability
)
target_compile_definitions(Mirael PRIVATE
	VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
	VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
	VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
	UUID_SYSTEM_GENERATOR=1
)
target_include_directories(Mirael PRIVATE
	${MIRAEL_INCLUDES}
	${IMGUI_INCLUDES}
	${INE_INCLUDES}
	${OTHER_INCLUDES}
)


#
# === Shaders ===
#

file(GLOB SHADER_SLANG_SOURCES
	"${MIRAEL_SRC_DIR}/*.slang"
)
add_slang_shader_target(MiraelShaders SOURCES ${SHADER_SLANG_SOURCES})
add_dependencies(Mirael MiraelShaders)