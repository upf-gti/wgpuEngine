#pragma once

#include <GLFW/glfw3.h>

#include "framework/utils/utils.h"
#include "graphics/renderer.h"

class FileWatcher;

class Engine {

    Renderer* renderer = nullptr;
    FileWatcher* shader_reload_watcher = nullptr;

    bool use_glfw;
    float delta_time = 0.0f;
    double current_time = 0.0;

public:

    static Engine* instance;

    Engine();

    virtual int initialize(Renderer* renderer, GLFWwindow *window, bool use_glfw, bool use_mirror_screen);
    virtual void clean();

    bool get_openxr_available();
    bool get_use_mirror_window();

    virtual void on_frame();

    virtual void update(float delta_time);
    virtual void render();

    float get_delta_time() { return delta_time; }

    void resize_window(int width, int height);
};
