#pragma once

#include "core/managers/manager.h"

#include <string>

class Scene;
class Camera;

class SimulationManager : public Manager {
    MANAGER_DECLARE(SimulationManager)

public:
    void set_main_scene(Scene* scene);
    Scene* get_main_scene();

    Camera* get_main_camera();

    void set_main_scene(const std::string& scene_path);

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
