#pragma once

#include <GLFW/glfw3.h>

#include "utils.h"
#include "graphics/renderer.h"

class FileWatcher;

class Engine {

    Renderer* renderer = nullptr;
    FileWatcher* shader_reload_watcher = nullptr;

    bool use_glfw;
    float delta_time = 0.0f;
    double current_time = 0.0;

public:

    virtual int initialize(Renderer* renderer, GLFWwindow *window, bool use_glfw, bool use_mirror_screen);
    void clean();

    bool get_openxr_available();
    bool get_use_mirror_window();

    virtual void on_frame();

    virtual void update(float delta_time);
    virtual void render();
};
