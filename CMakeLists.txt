cmake_minimum_required(VERSION 3.16)

set(WGPU_DIR_ROOT     ${CMAKE_CURRENT_SOURCE_DIR})
set(WGPU_DIR_SOURCES "${WGPU_DIR_ROOT}/src")
set(WGPU_DIR_LIBS    "${WGPU_DIR_ROOT}/libraries")

project(webgpuEngine LANGUAGES C CXX)

# Enable multicore compile on VS solution
if(MSVC)
  add_definitions(/MP)
endif()

if (WIN32)
    set(DAWN_BUILD "STATIC" CACHE STRING "Choose between SRC or STATIC to build from source (slower, better debugging) or download an static library (faster)")
else()
    set(DAWN_BUILD "SRC")
endif()

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)
add_definitions(-DWEBGPU_BACKEND=DAWN)

# enable FetchContent
include(FetchContent)

# download tinyobjloader
FetchContent_Declare(
    tinyobjloader
    URL https://raw.githubusercontent.com/tinyobjloader/tinyobjloader/release/tiny_obj_loader.h
    DOWNLOAD_DIR ${WGPU_DIR_LIBS}/tinyobjloader
    DOWNLOAD_NO_EXTRACT TRUE
)

# download tinyobjloader
FetchContent_Declare(
    stb_image
    URL https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
    DOWNLOAD_DIR ${WGPU_DIR_LIBS}/stb
    DOWNLOAD_NO_EXTRACT TRUE
)

# download nlohmann_json
FetchContent_Declare(
    nlohmann_json
    URL https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp
    DOWNLOAD_DIR ${WGPU_DIR_LIBS}/nlohmann_json
    DOWNLOAD_NO_EXTRACT TRUE
)

FetchContent_Declare(
    nlohmann_json_fwd
    URL https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json_fwd.hpp
    DOWNLOAD_DIR ${WGPU_DIR_LIBS}/nlohmann_json
    DOWNLOAD_NO_EXTRACT TRUE
)

# download tiny_gltf
FetchContent_Declare(
    tiny_gltf
    URL https://raw.githubusercontent.com/syoyo/tinygltf/release/tiny_gltf.h
    DOWNLOAD_DIR ${WGPU_DIR_LIBS}/tiny_gltf
    DOWNLOAD_NO_EXTRACT TRUE
)

FetchContent_MakeAvailable(tinyobjloader stb_image nlohmann_json nlohmann_json_fwd tiny_gltf)

# Macro to map filters to folder structure for MSVC projects
macro(GroupSources curdir)
    if(MSVC)
		file(GLOB children RELATIVE ${PROJECT_SOURCE_DIR}/${curdir} ${PROJECT_SOURCE_DIR}/${curdir}/*)

        foreach(child ${children})
            if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${curdir}/${child})
                GroupSources(${curdir}/${child})
            else()
                string(REPLACE "/" "\\" groupname ${curdir})
                source_group(${groupname} FILES ${PROJECT_SOURCE_DIR}/${curdir}/${child})
            endif()
        endforeach()
    endif()
endmacro()

GroupSources(src)

# Sources
macro(WGPU_SOURCES_APPEND)
    file(GLOB FILES_APPEND CONFIGURE_DEPENDS ${ARGV0}/*.h)
    list(APPEND WGPU_HEADERS ${FILES_APPEND})

    file(GLOB FILES_APPEND CONFIGURE_DEPENDS ${ARGV0}/*.cpp)
    list(APPEND WGPU_SOURCES ${FILES_APPEND})
endmacro()

WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES})
WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/framework)
WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/framework/camera)
WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/framework/nodes)
WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/framework/scene)
WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/framework/ui)
WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/framework/utils)
WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/framework/animation)
WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/framework/math)
WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/graphics)
WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/graphics/debug)
WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/engine)

if (NOT EMSCRIPTEN)
    WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/xr)
    WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/xr/dawnxr)
endif()

find_package(PythonInterp 3 REQUIRED)

file(GLOB_RECURSE
    SHADERS
    CONFIGURE_DEPENDS
    RELATIVE ${WGPU_DIR_SOURCES}/shaders
    ${WGPU_DIR_SOURCES}/shaders/**.wgsl
)

message(STATUS "Processing shaders...")

list(APPEND WGPU_SHADER_LIBS ${WGPU_DIR_ROOT}/glsl_builder.py)
list(APPEND WGPU_SHADER_LIBS ${WGPU_DIR_SOURCES}/shaders/math.wgsl)
list(APPEND WGPU_SHADER_LIBS ${WGPU_DIR_SOURCES}/shaders/mesh_includes.wgsl)
list(APPEND WGPU_SHADER_LIBS ${WGPU_DIR_SOURCES}/shaders/pbr_functions.wgsl)
list(APPEND WGPU_SHADER_LIBS ${WGPU_DIR_SOURCES}/shaders/tonemappers.wgsl)
list(APPEND WGPU_SHADER_LIBS ${WGPU_DIR_SOURCES}/shaders/ui/ui_includes.wgsl)

FOREACH(SHADER ${SHADERS})

    add_custom_command (
        OUTPUT ${WGPU_DIR_SOURCES}/shaders/${SHADER}.gen.h
	    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/glsl_builder.py ${SHADER}
        DEPENDS ${WGPU_DIR_SOURCES}/shaders/${SHADER} ${WGPU_SHADER_LIBS}
	    WORKING_DIRECTORY ${WGPU_DIR_SOURCES}/shaders
        VERBATIM
    )

    list(APPEND WGPU_HEADERS ${WGPU_DIR_SOURCES}/shaders/${SHADER}.gen.h)
ENDFOREACH()

add_library(${PROJECT_NAME} ${WGPU_SOURCES} ${WGPU_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ${WGPU_DIR_SOURCES})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

if (MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()

# INSTALL(FILES ${WGPU_HEADERS} DESTINATION ${CMAKE_BINARY_DIR})

# needed to compile tint on web
if (EMSCRIPTEN)
    add_compile_options(-w)
endif()

# dawn
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1")
add_subdirectory(libraries/webgpu)
target_link_libraries(${PROJECT_NAME} PUBLIC webgpu)

# glm
add_subdirectory(libraries/glm)

target_compile_definitions(glm PUBLIC GLM_ENABLE_EXPERIMENTAL)
target_compile_definitions(glm PUBLIC GLM_FORCE_QUAT_DATA_XYZW)
target_compile_definitions(glm PUBLIC GLM_FORCE_QUAT_CTOR_XYZW)

set_property(TARGET glm PROPERTY CXX_STANDARD 20)
set_property(TARGET glm PROPERTY FOLDER "External/glm")
target_link_libraries(${PROJECT_NAME} PUBLIC glm)

if (NOT EMSCRIPTEN)

    # vulkan
    find_package(Vulkan REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC ${Vulkan_LIBRARIES})
    include_directories(${Vulkan_INCLUDE_DIR})

    # openxr
    add_subdirectory(libraries/openxr)
    target_link_libraries(${PROJECT_NAME} PUBLIC openxr_loader)
    set_property(TARGET openxr_loader PROPERTY FOLDER "External/OpenXR")
    set_property(TARGET generate_openxr_header PROPERTY FOLDER "External/OpenXR")
    set_property(TARGET xr_global_generated_files PROPERTY FOLDER "External/OpenXR")

    # glfw
    add_subdirectory(libraries/glfw)
    add_subdirectory(libraries/glfw3webgpu)
    target_link_libraries(${PROJECT_NAME} PUBLIC glfw glfw3webgpu)
    set_property(TARGET glfw PROPERTY FOLDER "External/GLFW3")
    set_property(TARGET update_mappings PROPERTY FOLDER "External/GLFW3")
    set_property(TARGET glfw3webgpu PROPERTY FOLDER "External/GLFW3")

    # renderdoc
    target_include_directories(${PROJECT_NAME} PUBLIC ${WGPU_DIR_LIBS}/renderdoc)

endif()

# tinyobjloader
target_include_directories(${PROJECT_NAME} PUBLIC ${WGPU_DIR_LIBS}/tinyobjloader)

# stb
target_include_directories(${PROJECT_NAME} PUBLIC ${WGPU_DIR_LIBS}/stb)

# nlohmann_json
target_include_directories(${PROJECT_NAME} PUBLIC ${WGPU_DIR_LIBS}/nlohmann_json)

# tiny_gltf
target_include_directories(${PROJECT_NAME} PUBLIC ${WGPU_DIR_LIBS}/tiny_gltf)

# spdlog
SET(SPDLOG_ENABLE_PCH ON CACHE BOOL "BUILD PRECOMPILED HEADER IN SPDLOG")
add_subdirectory(libraries/spdlog)
target_link_libraries(${PROJECT_NAME} PUBLIC spdlog::spdlog)
set_property(TARGET spdlog PROPERTY FOLDER "External/spdlog")

# imgui
add_library(imgui STATIC
    ${WGPU_DIR_LIBS}/imgui/imgui.cpp
    ${WGPU_DIR_LIBS}/imgui/imgui_demo.cpp
    ${WGPU_DIR_LIBS}/imgui/imgui_draw.cpp
    ${WGPU_DIR_LIBS}/imgui/imgui_widgets.cpp
    ${WGPU_DIR_LIBS}/imgui/imgui_tables.cpp
    ${WGPU_DIR_LIBS}/imgui/backends/imgui_impl_glfw.cpp
    ${WGPU_DIR_LIBS}/imgui/backends/imgui_impl_wgpu.cpp
    ${WGPU_DIR_LIBS}/imgui/imconfig.h
    ${WGPU_DIR_LIBS}/imgui/imgui.h
    ${WGPU_DIR_LIBS}/imgui/imgui_internal.h
    ${WGPU_DIR_LIBS}/imgui/imstb_rectpack.h
    ${WGPU_DIR_LIBS}/imgui/imstb_textedit.h
    ${WGPU_DIR_LIBS}/imgui/imstb_truetype.h
)

target_link_libraries(imgui PUBLIC glfw webgpu)
target_include_directories(imgui PUBLIC ${WGPU_DIR_LIBS}/imgui)
target_link_libraries(${PROJECT_NAME} PUBLIC imgui)
set_property(TARGET imgui PROPERTY FOLDER "External/imgui")

target_copy_webgpu_binaries(${PROJECT_NAME})
