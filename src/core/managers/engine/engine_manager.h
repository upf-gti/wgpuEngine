#pragma once

#include "config_structs.h"

#include "core/managers/manager.h"

class Node;

class EngineManager : public Manager {
    MANAGER_DECLARE(EngineManager)

    EnginePostInitializeFunc engine_post_initialize = nullptr;
    EngineUpdateFunc engine_pre_update = nullptr;
    EngineUpdateFunc engine_post_update = nullptr;
    EngineRenderFunc engine_render = nullptr;

public:
    void set_configuration(const sEngineConfig& p_config) { config = p_config; }
    const sEngineConfig& get_configuration() const { return config; }

private:
    EngineManager() = default;
    ~EngineManager() = default;

    Error initialize() override;
    Error finalize() override;

    sEngineConfig config = {};
};
