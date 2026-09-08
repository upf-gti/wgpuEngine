#include "simulation_manager.h"

#include "framework/parsers/parser.h"
#include "framework/ui/io.h"

#include "core/managers/debug/debug_manager.h"
#include "core/managers/display/display_manager.h"
#include "core/managers/engine/engine_manager.h"
#include "core/managers/input/input_manager.h"
#include "core/managers/render/render_manager.h"
#include "core/managers/xr/xr_manager.h"

#include "scene/main/scene.h"

#include "GLFW/glfw3.h"
#include "core/managers/debug/debug_manager.h"

#ifdef __EMSCRIPTEN__

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>

EM_JS(void, on_engine_initialized, (), {
    onEngineInitialized();
});

#endif

// Called when callbacks are not provided by user
void dummy_engine_post_initialize() {}
void dummy_engine_pre_update(float delta_time) {}
void dummy_engine_post_update(float delta_time) {}
void dummy_engine_render() {}

Error SimulationManager::initialize()
{
    singleton_instance = this;

    const sEngineConfig& config = EngineManager::get_singleton()->get_configuration();

    engine_post_initialize = config.engine_post_initialize ? config.engine_post_initialize : dummy_engine_post_initialize;
    engine_pre_update = config.engine_pre_update ? config.engine_pre_update : dummy_engine_pre_update;
    engine_post_update = config.engine_post_update ? config.engine_post_update : dummy_engine_post_update;
    engine_render = config.engine_render ? config.engine_render : dummy_engine_render;

#ifdef __EMSCRIPTEN__
    on_engine_initialized();
#endif

    current_time = glfwGetTime();

    main_scene = new Scene("main_scene");

    engine_post_initialize();

    return Error::OK;
}

Error SimulationManager::finalize()
{
    singleton_instance = nullptr;
    return Error::OK;
}

Error SimulationManager::process_scene()
{
    if (!main_scene) {
        return Error::FAILED;
    }

    main_scene->update(delta_time);

    return Error::OK;
}

void SimulationManager::start_main_loop()
{
    LOG_INFO("Loop started");

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(
            [](void* user_data) {
                SimulationManager* sm = reinterpret_cast<SimulationManager*>(user_data);
                glfwPollEvents();
                sm->iteration();
            },
            (void*)this,
            0, true);
#else
    bool stop_game_loop = false;
    DisplayManager* display_manager = DisplayManager::get_singleton();

    while (!stop_game_loop) {
        display_manager->process_events();

        //if (display_manager->window_is_minimized()) {
        //    ImGui_ImplGlfw_Sleep(10);
        //    continue;
        //}

        iteration();

        stop_game_loop = display_manager->window_should_close();
    }
#endif
}

void SimulationManager::iteration()
{
    RenderAPI::get_singleton()->process_events();

    // Update stuff
    XRManager::get_singleton()->poll_actions();

    Parser::poll_async_parsers();

    InputManager::get_singleton()->update_mouse();

    double last_time = current_time;
    current_time = glfwGetTime();
    delta_time = static_cast<float>((current_time - last_time));

    IO::start_frame();

    engine_pre_update(delta_time);

    process_scene();

    engine_post_update(delta_time);

    // Update IO after updating the engine
    IO::update(delta_time);

    // Render stuff
    DebugManager::get_singleton()->start_render_overlay();
    DebugManager::get_singleton()->render_default_gui();

    engine_render();

    main_scene->render();

    RenderManager::get_singleton()->render();

#ifdef __EMSCRIPTEN__
    on_end_frame();
#endif

    InputManager::get_singleton()->set_mouse_wheel(0.0f, 0.0f);
    InputManager::get_singleton()->set_prev_state();
    XRManager::get_singleton()->set_prev_state();

    IO::end_frame();
}

void SimulationManager::set_main_scene(Scene* scene)
{
    main_scene = scene;
}

Scene* SimulationManager::get_main_scene()
{
    return main_scene;
}

Camera3D* SimulationManager::get_main_camera()
{
    return main_scene ? main_scene->get_main_camera() : nullptr;
}
