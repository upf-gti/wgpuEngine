#pragma once

struct GLFWwindow;
class FileWatcher;
class Renderer;
class Scene;

class Engine {

protected:

    Renderer* renderer = nullptr;
    FileWatcher* shader_reload_watcher = nullptr;
    Scene* main_scene;

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

    Scene* get_main_scene();

    virtual void on_frame();

    virtual void update(float delta_time);
    virtual void render();

    float get_delta_time() { return delta_time; }

    void resize_window(int width, int height);

    void vibrate_hand(int controller, float amplitude, float duration);
};
