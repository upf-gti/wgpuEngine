cmake_minimum_required(VERSION 3.13)

set(WGPU_DIR_ROOT     ${CMAKE_CURRENT_SOURCE_DIR})
set(WGPU_DIR_SOURCES "${WGPU_DIR_ROOT}/src")
set(WGPU_DIR_LIBS    "${WGPU_DIR_ROOT}/libraries")

project(webgpuEngine LANGUAGES C CXX)

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

FetchContent_MakeAvailable(tinyobjloader stb_image nlohmann_json)

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
WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/framework/entities)
WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/graphics)

if (NOT EMSCRIPTEN)
    WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/xr)
    WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/dawnxr)
endif()

add_library(${PROJECT_NAME} ${WGPU_SOURCES} ${WGPU_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ${WGPU_DIR_SOURCES})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

# INSTALL(FILES ${WGPU_HEADERS} DESTINATION ${CMAKE_BINARY_DIR})

# dawn
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1")
add_subdirectory(libraries/webgpu)
target_link_libraries(${PROJECT_NAME} PUBLIC webgpu libtint)

if (NOT EMSCRIPTEN)

    # vulkan
    find_package(Vulkan REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC ${Vulkan_LIBRARIES})
    include_directories(${Vulkan_INCLUDE_DIR})

    # openxr
    add_subdirectory(libraries/openxr)
    target_link_libraries(${PROJECT_NAME} PUBLIC openxr_loader)
    
    # glfw
    add_subdirectory(libraries/glfw3webgpu)
    target_link_libraries(${PROJECT_NAME} PUBLIC glfw glfw3webgpu)

endif()

# glm
add_subdirectory(libraries/glm)
target_link_libraries(${PROJECT_NAME} PUBLIC glm)

# tinyobjloader
target_include_directories(${PROJECT_NAME} PUBLIC ${WGPU_DIR_LIBS}/tinyobjloader)

# stb
target_include_directories(${PROJECT_NAME} PUBLIC ${WGPU_DIR_LIBS}/stb)

# nlohmann_json
target_include_directories(${PROJECT_NAME} PUBLIC ${WGPU_DIR_LIBS}/nlohmann_json)

target_copy_webgpu_binaries(${PROJECT_NAME})