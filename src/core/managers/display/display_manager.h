#pragma once

#include "core/managers/manager.h"

struct GLFWwindow;

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

    float window_get_dpi() const { return dpi_scale; }

private:
    DisplayManager() = default;
    ~DisplayManager() = default;

    Error initialize() override;
    Error finalize() override;

    void process_events();

    GLFWwindow* get_window() const;

    GLFWwindow* window = nullptr;

    int32_t window_width = 0;
    int32_t window_height = 0;

    float dpi_scale = 1.0f;
};
