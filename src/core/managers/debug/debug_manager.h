#pragma once

#include "core/managers/manager.h"

class Node;
class RenderdocCapture;
struct RENDERDOC_API_1_6_0;

class DebugManager : public Manager {
    MANAGER_DECLARE(DebugManager)
    friend class SimulationManager;

public:
    void render_default_gui();

    Error renderdoc_start_capture();
    Error renderdoc_end_capture();

private:
    DebugManager() = default;
    ~DebugManager() = default;

    Error initialize() override;
    Error finalize() override;

    void start_render_overlay();
    bool render_scene_tree_recursive(Node* entity);

    Error initialize_renderdoc();
    Error initialize_imgui();

    Node* selected_scene_node = nullptr; // imgui scene selected node

#ifndef __EMSCRIPTEN__
    RENDERDOC_API_1_6_0* rdoc_api = nullptr;
#endif
};
