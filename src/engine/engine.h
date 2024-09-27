#pragma once

#include <string>

struct GLFWwindow;
class FileWatcher;
class Renderer;
class Scene;
class Node;

struct sEngineConfiguration {
    uint16_t window_width = 1600;
    uint16_t window_height = 900;
};

class Engine {

    void init_imgui(GLFWwindow* window);
    void init_shader_watchers();

protected:

    Renderer* renderer = nullptr;
    FileWatcher* shader_reload_watcher = nullptr;
    FileWatcher* engine_shader_reload_watcher = nullptr;
    Scene* main_scene = nullptr;

    Node* selected_node = nullptr;

    bool use_glfw = false;
    float delta_time = 0.0f;
    double current_time = 0.0;

    bool stop_game_loop = false;

    bool show_imgui = true;

public:

    static Engine* instance;

    static Engine* get_instance() { return instance; }

    Engine();
    ~Engine();

    virtual int initialize(Renderer* renderer, sEngineConfiguration configuration = {});
    virtual void clean();

    virtual void start_loop();

    Node* (*node_factory)(const std::string& node_type);

    void add_node(Node* node);

    bool get_openxr_available();
    bool get_use_mirror_window();

    virtual void set_main_scene(const std::string& scene_path);
    Scene* get_main_scene();

    virtual void on_frame();

    virtual void update(float delta_time);
    virtual void render();

    virtual void render_default_gui();
    bool render_scene_tree_recursive(Node* entity);

    float get_delta_time() { return delta_time; }

    void resize_window(int width, int height);

    void vibrate_hand(int controller, float amplitude, float duration);
};
