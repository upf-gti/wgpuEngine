#pragma once

#include "core/managers/manager.h"
#include "core/managers/render/render_api.h"

#include "glm/vec2.hpp"

class RenderManager : public Manager {
    MANAGER_DECLARE(RenderManager)
    friend class DebugManager;
    friend class XRManager;
    friend class SimulationManager;
    friend struct OpenXRContext;
    friend struct WebXRContext;
    friend class Shader;

public:
    void set_render_size(int32_t width, int32_t height);

    int32_t get_render_width() const { return render_width; }
    int32_t get_render_height() const { return render_height; }

    inline uint32_t get_frame_counter() const { return frame_counter; }

    WGPUTextureFormat get_surface_format();

private:
    RenderManager() = default;
    ~RenderManager() = default;

    Error initialize() override;
    Error finalize() override;

    Error initialize_webgpu();
    Error initialize_step();

    void render();

    void set_required_features(std::vector<WGPUFeatureName> new_required_features);

    RenderAPI render_api;

    int32_t render_width = 0;
    int32_t render_height = 0;

    uint32_t frame_counter = 0;

    std::vector<WGPUFeatureName> required_features = {};
};
