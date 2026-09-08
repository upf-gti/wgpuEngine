#include "render_manager.h"

#include "core/managers/engine/engine_manager.h"

#include "scene/3d/light_3d.h"

#include "graphics/shader.h"

#include "spdlog/spdlog.h"

Error RenderManager::initialize()
{
    singleton_instance = this;

    render_api.api_instance_create();

    const sEngineConfig& config = EngineManager::get_singleton()->get_configuration();
    required_features = config.render_config.required_features;

#ifdef __EMSCRIPTEN__
    spdlog::info("Initializing Renderer and WASM module...");

    while (initialize_step() || !wasm_module_initialized) {
        renderer->process_events();
        emscripten_sleep(1); // Allows browser to run events
    }

    spdlog::info("WASM module initialized");
#else
    while (initialize_step()) {
        render_api.process_events();
    }
#endif

    initialize_timestamp_queries();

    //Shader::set_custom_define("MAX_LIGHTS", MAX_LIGHTS);

    //multisample_textures = new Texture[EYE_COUNT];

    return Error::OK;
}

Error RenderManager::finalize()
{
    singleton_instance = nullptr;
    return Error::OK;
}

Error RenderManager::initialize_step()
{
    static WGPUFuture adapter_future = { 0 };
    static WGPUFuture device_future = { 0 };

    if (!render_api.adapter) {
        if (adapter_future.id == 0) {
            adapter_future = render_api.request_adapter();
        }
        render_api.process_events();
        return Error::IN_PROGRESS;
    }

    if (render_api.adapter && !render_api.device) {
        if (device_future.id == 0) {
            // The engine needs FloatFilterable as a default
            required_features.push_back(WGPUFeatureName_Float32Filterable);
            const sEngineConfig& config = EngineManager::get_singleton()->get_configuration();
            device_future = render_api.request_device(required_features, config.render_config.required_limits);
        }
        render_api.process_events();
        return Error::IN_PROGRESS;
    }

    // NOTE: breakpoint here for initial compute debugging in Metal
    if (render_api.adapter && render_api.device) {
        if (render_api.initialize()) {
            spdlog::error("Could not initialize WebGPU context");
            return Error::IN_PROGRESS;
        }
    }

    spdlog::info("Renderer initialized");

    return Error::OK;
}

Error RenderManager::initialize_timestamp_queries()
{
    timestamp_query_set = RenderAPI::get_singleton()->create_query_set(maximum_query_sets);
    timestamp_query_buffer = RenderAPI::get_singleton()->buffer_create(sizeof(uint64_t) * maximum_query_sets, WGPUBufferUsage_QueryResolve | WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, nullptr);

    return Error::OK;
}

void RenderManager::render()
{
    frame_counter++;
}

void RenderManager::set_required_features(std::vector<WGPUFeatureName> new_required_features)
{
    required_features = new_required_features;
}

void RenderManager::resize_swapchain()
{
    RenderAPI::get_singleton()->surface_configure_swapchain(render_width, render_height);

    render_cull.resize_swapchain();
}

void RenderManager::set_render_size(int32_t width, int32_t height)
{
    render_width = width;
    render_height = height;
    spdlog::info("Render size: {}x{}", render_width, render_height);
}

void RenderManager::resolve_query_set(WGPUCommandEncoder encoder, uint8_t first_query)
{
    wgpuCommandEncoderResolveQuerySet(encoder, timestamp_query_set, first_query, query_index, timestamp_query_buffer, 0);
}

void RenderManager::get_timestamps()
{
    auto read_callback = [&](const void* output_buffer, void* user_data) {
        const uint64_t* timestamps_buffer = reinterpret_cast<const uint64_t*>(output_buffer);
        uint8_t* query_index_cpy = static_cast<uint8_t*>(user_data);

        std::vector<float> time_diffs;
        for (int i = 0; i < *query_index_cpy; i += 2) {
            uint64_t diff = timestamps_buffer[i + 1] - timestamps_buffer[i];
            float milliseconds = (float)diff * 1e-6f;
            time_diffs.push_back(milliseconds);
        }

        last_frame_timestamps = time_diffs;

        delete query_index_cpy;
    };

    // copy query_index, otherwise it'd have been already modified when reading
    uint8_t* query_index_cpy = new uint8_t();
    *query_index_cpy = query_index;
    RenderAPI::get_singleton()->buffer_read_async(timestamp_query_buffer, sizeof(uint64_t) * maximum_query_sets, read_callback, query_index_cpy);
}

WGPUTextureFormat RenderManager::get_surface_format()
{
    return render_api.surface_format;
}

void RenderManager::add_renderable(Mesh* mesh_instance, const glm::mat4x4& global_matrix)
{
    if ((render_entity_list.size() + 1) >= current_render_list_size) {
        current_render_list_size <<= 1;
        render_entity_list.reserve(current_render_list_size);
    }

    render_entity_list.push_back({ mesh, global_matrix });
}

void RenderManager::add_light(Light3D* new_light)
{
    sLightUniformData data;
    new_light->get_uniform_data(data);
    lights_uniform_data[num_lights] = data;
    num_lights++;

    const LightType light_type = new_light->get_type();

    if (!new_light->get_cast_shadows()) {
        return;
    }

    if (light_type == LIGHT_DIRECTIONAL) {
        lights_with_shadow.push_back(new_light);
    }
}

uint8_t RenderManager::timestamp(WGPUCommandEncoder encoder, const char* label)
{
    queries_label_map[query_index] = std::string(label);

    assert(query_index + 1 < maximum_query_sets);

    return query_index++;
}
