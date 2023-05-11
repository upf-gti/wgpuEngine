cmake_minimum_required(VERSION 3.13)

SET(CMAKE_CXX_FLAGS  "-g")
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(WGPU_DIR_ROOT     ${CMAKE_CURRENT_LIST_DIR})
set(WGPU_DIR_SOURCES "${WGPU_DIR_ROOT}/src")

project(webgpuEngine LANGUAGES C CXX)

# PROJECT SOURCE
# file(GLOB CPP_SOURCES "src/*.cpp")
# file(GLOB CPP_SUBFOLDER_SOURCES "src/**/*.cpp")
# file(GLOB H_SOURCES "src/*.h")
# file(GLOB C_SOURCES "src/*.c")
# file(GLOB C_SUBFOLDER_SOURCES "src/**/*.c")
# set(includes_dir "includes/")

add_definitions(-DWEBGPU_BACKEND=DAWN)

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
macro(WGPU_FILES_APPEND)
    file(GLOB FILES_APPEND CONFIGURE_DEPENDS ${ARGV})
    list(APPEND WGPU_SOURCES ${FILES_APPEND})
endmacro()
macro(WGPU_SOURCES_APPEND)
    WGPU_FILES_APPEND(${ARGV0}/*.h)
    WGPU_FILES_APPEND(${ARGV0}/*.cpp)
endmacro()

WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES})
WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/dawnxr)

add_executable(${PROJECT_NAME} ${WGPU_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${WGPU_DIR_SOURCES})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

# add_subdirectory(libraries/wgpu)
# target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libraries/wgpu/include/webgpu")
# target_link_libraries(${PROJECT_NAME} webgpu)

# dawn
add_subdirectory(libraries/webgpu)
target_link_libraries(${PROJECT_NAME} webgpu)
# target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libraries/wgpu/include/webgpu")

# vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES})
include_directories(${Vulkan_INCLUDE_DIR})

# glfw
# add_subdirectory(glfw)
target_link_libraries(${PROJECT_NAME} glfw)

# openxr
add_subdirectory(libraries/openxr)
target_link_libraries(${PROJECT_NAME} openxr_loader)

target_copy_webgpu_binaries(${PROJECT_NAME})