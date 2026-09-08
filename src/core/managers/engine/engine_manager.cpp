#include "engine_manager.h"

//#include "core/managers/input/input_manager.h"
//#include "scene/3d/mesh_instance_3d.h"
//#include "framework/parsers/parse_scene.h"
//#include "framework/parsers/parser.h"
//#include "framework/ui/io.h"
//#include "framework/utils/file_watcher.h"
//#include "framework/utils/tinyfiledialogs.h"
//
//#include "graphics/primitives/box_mesh.h"
//#include "graphics/primitives/capsule_mesh.h"
//#include "graphics/primitives/cone_mesh.h"
//#include "graphics/primitives/cylinder_mesh.h"
//#include "graphics/primitives/sphere_mesh.h"
//#include "graphics/primitives/torus_mesh.h"
//#include "graphics/renderer.h"
//#include "graphics/renderer_storage.h"
//
//#include "framework/nodes/directional_light_3d.h"
//#include "framework/nodes/omni_light_3d.h"
//#include "framework/nodes/spot_light_3d.h"
//
//#if defined(OPENXR_SUPPORT)
//#include "xr/openxr/openxr_context.h"
//#elif defined(WEBXR_SUPPORT)
//#include "xr/webxr/webxr_context.h"
//#endif
//
//#include "shaders/mesh_forward.wgsl.gen.h"
//#include "shaders/mesh_grid.wgsl.gen.h"
//
//#include "backends/imgui_impl_glfw.h"
//#include "backends/imgui_impl_wgpu.h"
//#include "framework/utils/ImGuizmo.h"
//#include "imgui.h"
//#include "imgui_internal.h"
//
//#include "engine/scene.h"
//
#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>

EM_JS(void, on_engine_pre_initialized, (), {
    onEnginePreInitialized();
});

EM_JS(void, on_engine_initialized, (), {
    onEngineInitialized();
});

//EM_JS(void, on_end_frame, (), {
//    if (Module.Engine.onFrame) {
//        Module.Engine.onFrame();
//    }
//});
//
//EM_JS(void, on_render, (), {
//    if (Module.Engine.onRender) {
//        Module.Engine.onRender();
//    }
//});
//
//EM_JS(void, on_update, (float delta_time), {
//    if (Module.Engine.onUpdate) {
//        Module.Engine.onUpdate(delta_time);
//    }
//});
//
//EM_JS(int, canvas_get_width, (), {
//    return canvas.clientWidth;
//});
//
//EM_JS(int, canvas_get_height, (), {
//    return canvas.clientHeight;
//});
//
//EM_JS(const char*, get_html5_resize_target, (), {
//    var str = "html5ResizeTarget" in Module ? Module.html5ResizeTarget : "";
//    var lengthBytes = lengthBytesUTF8(str) + 1;
//    var ptr = _malloc(lengthBytes);
//    stringToUTF8(str, ptr, lengthBytes);
//    return ptr;
//});
//
#endif

#include "spdlog/spdlog.h"

// Called when callbacks are not provided by user
void dummy_engine_post_initialize() {}
void dummy_engine_pre_update(float delta_time) {}
void dummy_engine_post_update(float delta_time) {}
void dummy_engine_render() {}

Error EngineManager::initialize()
{
    singleton_instance = this;

    spdlog::set_pattern("[%^%l%$] %v");
    spdlog::set_level(spdlog::level::debug);

    engine_post_initialize = config.engine_post_initialize ? config.engine_post_initialize : dummy_engine_post_initialize;
    engine_pre_update = config.engine_pre_update ? config.engine_pre_update : dummy_engine_pre_update;
    engine_post_update = config.engine_post_update ? config.engine_post_update : dummy_engine_post_update;
    engine_render = config.engine_render ? config.engine_render : dummy_engine_render;

#ifdef __EMSCRIPTEN__
    on_engine_pre_initialized();

    show_imgui = false;
#endif

    spdlog::info("Engine initialized");

    // initialize completed

    //renderer->post_initialize();

    //renderer->set_camera_params(config.camera_type, config.camera_eye, config.camera_center);

#ifdef __EMSCRIPTEN__
    on_engine_initialized();
#endif

    // submit any initialization commands
    //renderer->submit_global_command_encoder();

    return Error::OK;
}

Error EngineManager::finalize()
{
    singleton_instance = nullptr;
    return Error::OK;
}
