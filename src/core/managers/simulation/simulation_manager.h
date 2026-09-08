#pragma once

#include "config_structs.h"
#include "core/managers/manager.h"

#include <string>

class Scene;
class Camera3D;

#define SIMULATION_MANAGER SimulationManager::get_singleton()

class SimulationManager : public Manager {
    MANAGER_DECLARE(SimulationManager)

    EnginePostInitializeFunc engine_post_initialize = nullptr;
    EngineUpdateFunc engine_pre_update = nullptr;
    EngineUpdateFunc engine_post_update = nullptr;
    EngineRenderFunc engine_render = nullptr;

public:
    void set_main_scene(Scene* scene);
    Scene* get_main_scene();

    Camera3D* get_main_camera();

private:
    SimulationManager() = default;
    ~SimulationManager() = default;

    Error initialize() override;
    Error finalize() override;

    Error process_scene();

    void start_main_loop();

    void iteration();

    Scene* main_scene = nullptr;

    float delta_time = 0.0f;
    double current_time = 0.0;
};
