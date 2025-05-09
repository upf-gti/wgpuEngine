# This is only meant to be included as a subdirectory in another project.
# It assumes that targets 'glfw' and 'webgpu' exist.
# Loot at examples/CMakeLists.txt to see how to use it in a project.

# The glfw3webgpu target
add_library(glfw3webgpu STATIC glfw3webgpu.cpp glfw3webgpu.hpp)

set_property(TARGET glfw3webgpu PROPERTY LANGUAGES CXX)
set_property(TARGET glfw3webgpu PROPERTY CXX_STANDARD 20)

target_include_directories(glfw3webgpu PUBLIC .)
target_link_libraries(glfw3webgpu PUBLIC glfw webgpu)

if (APPLE)
  target_compile_options(glfw3webgpu PRIVATE -x objective-c++)
  target_link_libraries(glfw3webgpu PRIVATE "-framework Cocoa" "-framework CoreVideo" "-framework IOKit" "-framework QuartzCore")
endif ()

# Copy compile definitions that are PRIVATE in glfw
if (GLFW_BUILD_COCOA)
    target_compile_definitions(glfw3webgpu PRIVATE _GLFW_COCOA)
endif()
if (GLFW_BUILD_WIN32)
    target_compile_definitions(glfw3webgpu PRIVATE _GLFW_WIN32)
endif()
if (GLFW_BUILD_X11)
    target_compile_definitions(glfw3webgpu PRIVATE _GLFW_X11)
endif()
if (GLFW_BUILD_WAYLAND)
    target_compile_definitions(glfw3webgpu PRIVATE _GLFW_WAYLAND)
endif()