#pragma once

#include "core/managers/manager.h"

#include "spdlog/spdlog.h"

class Node;
class RenderdocCapture;
struct RENDERDOC_API_1_6_0;

#define LOG_INFO(...) DebugManager::get_singleton()->log_info(__VA_ARGS__)
#define LOG_WARN(...) DebugManager::get_singleton()->log_warn(__VA_ARGS__)
#define LOG_ERROR(...) DebugManager::get_singleton()->log_error(__VA_ARGS__)

class DebugManager : public Manager {
    MANAGER_DECLARE(DebugManager)
    friend class SimulationManager;

public:
    void render_default_gui();

    Error renderdoc_start_capture();
    Error renderdoc_end_capture();

    template <typename T>
    void log_info(const T& msg)
    {
        logger->info(msg);
    }

    template <typename T>
    void log_warn(const T& msg)
    {
        logger->warn(msg);
    }

    template <typename T>
    void log_error(const T& msg)
    {
        logger->error(msg);
    }

    template <typename... Args>
    void log_info(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        logger->info(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void log_warn(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        logger->warn(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void log_error(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        logger->error(fmt, std::forward<Args>(args)...);
    }

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

    spdlog::logger* logger;

#ifndef __EMSCRIPTEN__
    RENDERDOC_API_1_6_0* rdoc_api = nullptr;
#endif
};
