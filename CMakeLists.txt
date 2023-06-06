cmake_minimum_required(VERSION 3.13)

set(WGPU_DIR_ROOT     ${CMAKE_CURRENT_LIST_DIR})
set(WGPU_DIR_SOURCES "${WGPU_DIR_ROOT}/src")

project(webgpuEngine LANGUAGES C CXX)

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
WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/graphics)

if (NOT EMSCRIPTEN)
    WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/xr)
    # WGPU_SOURCES_APPEND(${WGPU_DIR_SOURCES}/dawnxr)
endif()

add_executable(${PROJECT_NAME} ${WGPU_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${WGPU_DIR_SOURCES})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

# dawn
#set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1")
add_subdirectory(libraries/webgpu)
target_link_libraries(${PROJECT_NAME} webgpu)

if (EMSCRIPTEN)
	set(SHELL_FILE shell.html)

	target_link_options(${PROJECT_NAME} PRIVATE
		-sUSE_GLFW=3
		-sUSE_WEBGPU
		-sALLOW_MEMORY_GROWTH
		--shell-file "${CMAKE_CURRENT_SOURCE_DIR}/${SHELL_FILE}"
	)

	# Make sure to re-link when the shell file changes
	set_property(
		TARGET ${PROJECT_NAME}
		PROPERTY LINK_DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/${SHELL_FILE}"
	)

	set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
endif()

if (NOT EMSCRIPTEN)

    # vulkan
    find_package(Vulkan REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES})
    include_directories(${Vulkan_INCLUDE_DIR})

    # openxr
    add_subdirectory(libraries/openxr)
    target_link_libraries(${PROJECT_NAME} openxr_loader)

endif()

# glfw
add_subdirectory(libraries/glfw3webgpu)
target_link_libraries(${PROJECT_NAME} glfw glfw3webgpu)

# glm
add_subdirectory(libraries/glm)
target_link_libraries(${PROJECT_NAME} glm)

target_copy_webgpu_binaries(${PROJECT_NAME})