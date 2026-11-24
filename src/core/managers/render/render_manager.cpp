#include "render_manager.h"

#include "core/managers/display/display_manager.h"
#include "core/managers/engine/engine_manager.h"
#include "core/managers/render/render_methods/render_forward.h"
#include "core/managers/xr/xr_manager.h"

#include "scene/3d/light_3d.h"

#include "graphics/shader.h"

#include "core/managers/debug/debug_manager.h"

Error RenderManager::initialize()
{
    singleton_instance = this;

    return Error::OK;
}

Error RenderManager::finalize()
{
    render_storage.finalize();
    render_compositor.finalize();

    singleton_instance = nullptr;
    return Error::OK;
}

Error RenderManager::initialize_webgpu()
{
    render_api.api_instance_create();
    render_storage.initialize();

    const sEngineConfig& config = EngineManager::get_singleton()->get_configuration();
    required_features = config.render_config.required_features;

#ifdef __EMSCRIPTEN__
    LOG_INFO("Initializing Renderer and WASM module...");

    while (initialize_step() || !wasm_module_initialized) {
        renderer->process_events();
        emscripten_sleep(1); // Allows browser to run events
    }

    LOG_INFO("WASM module initialized");
#else
    while (initialize_step()) {
        render_api.process_events();
    }
#endif

    Shader::set_custom_define("MAX_LIGHTS", MAX_LIGHTS);

    initialize_timestamp_queries();

    //multisample_textures = new Texture[EYE_COUNT];

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
            LOG_ERROR("Could not initialize WebGPU context");
            return Error::IN_PROGRESS;
        }
    }

    LOG_INFO("Renderer initialized");

    return Error::OK;
}

Error RenderManager::initialize_timestamp_queries()
{
    timestamp_query_set = RenderAPI::get_singleton()->create_query_set(maximum_query_sets);
    timestamp_query_buffer = RenderAPI::get_singleton()->buffer_create(sizeof(uint64_t) * maximum_query_sets, WGPUBufferUsage_QueryResolve | WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, nullptr);

    return Error::OK;
}

Error RenderManager::initialize_renderer()
{
    Error error = Error::OK;

    const sEngineConfig& config = EngineManager::get_singleton()->get_configuration();

    if (config.render_config.render_method == FORWARD_RENDERER) {
        RenderForward* render_forward = new RenderForward();
        error = render_forward->initialize();
        render_compositor.set_render_method(render_forward);
    } else {
        // TODO
        assert(0);
    }

    if (!XR_MANAGER->is_xr_available()) {
        glm::ivec2 framebuffer_size = DISPLAY_MANAGER->window_get_framebuffer_size();
        set_render_size(framebuffer_size.x, framebuffer_size.y);
    } else {
        glm::uvec2 swapchain_size = XR_MANAGER->get_swapchain_size();
        set_render_size(static_cast<int>(swapchain_size.x), static_cast<int>(swapchain_size.y));
    }

    render_compositor.initialize();

    return error;
}

void RenderManager::render()
{
    WGPUCommandEncoderDescriptor encoder_desc = {};
    command_encoder = RENDER_API->command_encoder_create(&encoder_desc);

    render_compositor.render(renderables_list, lights_list, environment_data, render_frame_flags);

    frame_counter++;
    query_index = 0;
    render_frame_flags = 0;

    RENDER_API->command_encoder_release(command_encoder);
}

void RenderManager::set_required_features(std::vector<WGPUFeatureName> new_required_features)
{
    required_features = new_required_features;
}

void RenderManager::resize_swapchain()
{
    glm::ivec2 framebuffer_size = DISPLAY_MANAGER->window_get_framebuffer_size();
    RenderAPI::get_singleton()->surface_configure_swapchain(framebuffer_size.x, framebuffer_size.y);
}

void RenderManager::environment_data_set_irradiance_texture(Texture* irradiance_texture_panorama)
{
    environment_data.irradiance_texture_panorama = irradiance_texture_panorama;
}

void RenderManager::set_render_size(int32_t width, int32_t height)
{
    LOG_INFO("Render size: {}x{}", width, height);

    render_width = width;
    render_height = height;

    render_compositor.set_render_size(width, height);
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

WGPUCommandEncoder RenderManager::get_main_command_encoder()
{
    return command_encoder;
}

void RenderManager::add_renderable(RenderableDataRef* renderable_data_ref)
{
    if ((renderables_list.size() + 1) >= reserved_render_list_size) {
        reserved_render_list_size <<= 1;
        renderables_list.reserve(reserved_render_list_size);
    }

    renderables_list.push_back(renderable_data_ref);

    renderable_data_ref->idx = static_cast<uint32_t>(renderables_list.size() - 1);

    set_renderables_dirty();
}

void RenderManager::remove_renderable(uint32_t idx)
{
    if (idx >= renderables_list.size()) {
        LOG_ERROR("Can not remove renderable with index ({})", idx);
        assert(0);
        return;
    }

    renderables_list.erase(renderables_list.begin() + idx);

    // Reassign indices
    for (uint32_t i = idx; i < renderables_list.size(); ++i) {
        renderables_list[i]->idx = i;
    }

    set_renderables_dirty();
}

void RenderManager::set_renderables_dirty()
{
    render_frame_flags |= RENDER_MESHES_DIRTY;
}

void RenderManager::add_light(LightDataRef* light_data_ref)
{
    if ((lights_list.size() + 1) >= reserved_light_list_size) {
        reserved_light_list_size <<= 1;
        lights_list.reserve(reserved_light_list_size);
    }

    lights_list.push_back(light_data_ref);

    light_data_ref->idx = static_cast<uint32_t>(lights_list.size() - 1);

    set_lights_dirty();
}

void RenderManager::remove_light(uint32_t idx)
{
    if (idx >= lights_list.size()) {
        LOG_ERROR("Can not remove light with index ({})", idx);
        assert(0);
        return;
    }

    lights_list.erase(lights_list.begin() + idx);

    // Reassign indices
    for (uint32_t i = idx; i < lights_list.size(); ++i) {
        lights_list[i]->idx = i;
    }

    set_lights_dirty();
}

void RenderManager::set_lights_dirty()
{
    render_frame_flags |= RENDER_LIGHTS_DIRTY;
}

void RenderManager::set_clear_color(const glm::vec4& clear_color)
{
    RenderMethod* render_method = render_compositor.get_render_method();

    if (!render_method) {
        LOG_WARN("Can not set clear color without render method");
        return;
    }

    render_method->set_clear_color(clear_color);
}

uint8_t RenderManager::timestamp(WGPUCommandEncoder encoder, const char* label)
{
    queries_label_map[query_index] = std::string(label);

    assert(query_index + 1 < maximum_query_sets);

    return query_index++;
}
