#pragma once

#include <GLFW/glfw3.h>

#include "utils.h"
#include "graphics/renderer.h"

class FileWatcher;

class Engine {

    Renderer* renderer = nullptr;
    FileWatcher* shader_reload_watcher = nullptr;

public:

    virtual int initialize(Renderer* renderer, GLFWwindow *window, bool use_mirror_screen);
    void clean();

    bool get_openxr_available();
    bool get_use_mirror_window();

    virtual void update(float delta_time);
    virtual void render();
};
