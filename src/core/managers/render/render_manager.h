#pragma once

#include "core/managers/manager.h"
#include "core/managers/render/render_api.h"
#include "core/managers/render/render_cull.h"
#include "core/managers/render/render_storage.h"

#include "glm/vec2.hpp"

class Light3D;

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

    void add_renderable(Mesh* mesh_instance, const glm::mat4x4& global_matrix);
    void add_light(Light3D* new_light);

    uint8_t timestamp(WGPUCommandEncoder encoder, const char* label = "");

    WGPUQuerySet get_query_set() { return timestamp_query_set; }
    void resolve_query_set(WGPUCommandEncoder encoder, uint8_t first_query);
    void get_timestamps();
    std::map<uint8_t, std::string>& get_queries_label_map() { return queries_label_map; }

private:
    RenderManager() = default;
    ~RenderManager() = default;

    Error initialize() override;
    Error finalize() override;

    Error initialize_webgpu();
    Error initialize_step();
    Error initialize_timestamp_queries();

    void render();

    void set_required_features(std::vector<WGPUFeatureName> new_required_features);

    void resize_swapchain();

    RenderAPI render_api;
    RenderCull render_cull;
    RenderStorage render_storage;

    int32_t render_width = 0;
    int32_t render_height = 0;

    uint32_t frame_counter = 0;

    std::vector<WGPUFeatureName> required_features = {};

    // Entities to be rendered this frame
    uint32_t current_render_list_size = 128;
    std::vector<sRenderableData> render_entity_list;

    // Timestamp queries
    WGPUQuerySet timestamp_query_set;
    uint8_t maximum_query_sets = 16;

    WGPUBuffer timestamp_query_buffer;
    uint8_t query_index = 0;
    std::map<uint8_t, std::string> queries_label_map;
    std::vector<float> last_frame_timestamps;
    bool timestamps_requested = false;

    std::vector<float>& get_last_frame_timestamps() { return last_frame_timestamps; }
};
