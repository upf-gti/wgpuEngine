#include "render_manager.h"

#include "core/managers/engine/engine_manager.h"

#include "graphics/shader.h"

#include "spdlog/spdlog.h"

WGPUTextureFormat RenderManager::get_surface_format()
{
    return render_api.surface_format;
}

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

void RenderManager::render()
{
    frame_counter++;
}

void RenderManager::set_required_features(std::vector<WGPUFeatureName> new_required_features)
{
    required_features = new_required_features;
}

void RenderManager::set_render_size(int32_t width, int32_t height)
{
    render_width = width;
    render_height = height;
    spdlog::info("Render size: {}x{}", render_width, render_height);
}
