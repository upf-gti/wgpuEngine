#include "engine_manager.h"

#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>

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

#include "core/managers/debug/debug_manager.h"

Error EngineManager::initialize()
{
    singleton_instance = this;

    return Error::OK;
}

Error EngineManager::finalize()
{
    singleton_instance = nullptr;
    return Error::OK;
}
