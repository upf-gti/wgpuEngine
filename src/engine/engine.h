#pragma once

#include "config_structs.h"
#include "framework/camera/camera.h"

struct GLFWwindow;
class FileWatcher;
class Renderer;
class Scene;
class Node;
class Engine;

class Engine {
    void init_imgui(GLFWwindow* window);
    void init_shader_watchers();

    EnginePostInitializeFunc engine_post_initialize = nullptr;
    EngineUpdateFunc engine_pre_update = nullptr;
    EngineUpdateFunc engine_post_update = nullptr;
    EngineRenderFunc engine_render = nullptr;

protected:
    Renderer* renderer = nullptr;
    FileWatcher* shader_reload_watcher = nullptr;
    FileWatcher* engine_shader_reload_watcher = nullptr;
    Scene* main_scene = nullptr;

    sEngineConfiguration configuration = {};

    Node* selected_node = nullptr;

    bool use_glfw = false;
    float delta_time = 0.0f;
    double current_time = 0.0;

    bool stop_game_loop = false;

    bool show_imgui = true;

#ifdef __EMSCRIPTEN__
    bool wasm_module_initialized = false;
#endif

    bool pre_initialize_renderer();

    bool initialize_step();

public:
    static Engine* instance;

    Engine();
    ~Engine();

    virtual int initialize(Renderer* renderer, const sEngineConfiguration& configuration = {});
    virtual int post_initialize();
    virtual void clean();

    virtual void start_loop();
    virtual void on_frame();
    virtual void update(float delta_time);
    virtual void render();

    virtual void render_default_gui();
    bool render_scene_tree_recursive(Node* entity);
    void add_node(Node* node);
    virtual void resize_window(int width, int height);
    void vibrate_hand(int controller, float amplitude, float duration);

    static Engine* get_instance() { return instance; }
    sEngineConfiguration get_configuration() { return configuration; }
    Renderer* get_renderer() { return renderer; }
    bool get_xr_available();
    bool get_use_mirror_window();
    bool should_close();
    Scene* get_main_scene();
    float get_delta_time() { return delta_time; }
    void get_scene_ray(glm::vec3& ray_origin, glm::vec3& ray_direction);

    void set_main_scene(Scene* scene);
    virtual void set_main_scene(const std::string& scene_path);

#ifdef __EMSCRIPTEN__
    bool get_wasm_module_initialized() const { return wasm_module_initialized; }
    void set_wasm_module_initialized(bool value) { wasm_module_initialized = value; }
#endif
};
