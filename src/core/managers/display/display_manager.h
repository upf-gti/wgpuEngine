#pragma once

#include "core/managers/manager.h"

#include "glm/vec2.hpp"

struct GLFWwindow;

#define DISPLAY_MANAGER DisplayManager::get_singleton()

class DisplayManager : public Manager {
    MANAGER_DECLARE(DisplayManager)
    friend class SimulationManager;
    friend class InputManager;
    friend class DebugManager;
    friend class RenderAPI;

public:
    bool window_is_minimized() const;
    bool window_should_close() const;

    void window_set_size(int32_t width, int32_t height);
    int32_t window_get_width() const { return window_width; }
    int32_t window_get_height() const { return window_height; }

    glm::ivec2 window_get_framebuffer_size();

    float window_get_dpi() const
    {
        return dpi_scale;
    }

private:
    DisplayManager() = default;
    ~DisplayManager() = default;

    Error initialize() override;
    Error finalize() override;

    void process_events();

    static void glfw_resize_callback(GLFWwindow* window, int width, int height);

    GLFWwindow* get_window() const;

    GLFWwindow* window = nullptr;

    int32_t window_width = 0;
    int32_t window_height = 0;

    float dpi_scale = 1.0f;
};
