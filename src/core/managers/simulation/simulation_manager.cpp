#include "simulation_manager.h"

#include "framework/parsers/parser.h"
#include "framework/ui/io.h"

#include "core/managers/debug/debug_manager.h"
#include "core/managers/display/display_manager.h"
#include "core/managers/input/input_manager.h"
#include "core/managers/render/render_manager.h"
#include "core/managers/xr/xr_manager.h"

#include "scene/main/scene.h"

#include "GLFW/glfw3.h"
#include "spdlog/spdlog.h"

Error SimulationManager::initialize()
{
    singleton_instance = this;

    current_time = glfwGetTime();

    main_scene = new Scene("main_scene");

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
    spdlog::info("Loop started");

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

    SimulationManager::get_singleton()->process_scene();

    // Update IO after updating the engine
    IO::update(delta_time);

    // Render stuff
    DebugManager::get_singleton()->start_render_overlay();

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

Camera* SimulationManager::get_main_camera()
{
    return main_scene ? main_scene->get_main_camera() : nullptr;
}
