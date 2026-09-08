#pragma once

#include "core/managers/manager.h"

class Node;
class RenderdocCapture;
struct RENDERDOC_API_1_6_0;

class DebugManager : public Manager {
    MANAGER_DECLARE(DebugManager)
    friend class SimulationManager;

public:
private:
    DebugManager() = default;
    ~DebugManager() = default;

    Error initialize() override;
    Error finalize() override;

    Error initialize_renderdoc();

    Error renderdoc_start_capture();
    Error renderdoc_end_capture();

    void start_render_overlay();

    void render_default_gui();
    bool render_scene_tree_recursive(Node* entity);

    void init_imgui();

    Node* selected_scene_node = nullptr; // imgui scene selected node

#ifndef __EMSCRIPTEN__
    RENDERDOC_API_1_6_0* rdoc_api = nullptr;
#endif
};
