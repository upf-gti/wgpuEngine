#pragma once

#include "config_structs.h"

#include "core/managers/manager.h"

class Node;

class EngineManager : public Manager {
    MANAGER_DECLARE(EngineManager)

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
