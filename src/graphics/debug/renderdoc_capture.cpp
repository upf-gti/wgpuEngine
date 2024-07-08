#include "renderdoc_capture.h"

#if defined(_WIN32)

#include "windows.h"

#elif defined(__linux__)

#include <dlfcn.h>

#endif

//#include "spdlog/spdlog.h"

#ifndef __EMSCRIPTEN__
RENDERDOC_API_1_6_0* RenderdocCapture::rdoc_api = nullptr;
#endif

bool RenderdocCapture::capture_started = false;

void RenderdocCapture::init()
{
    bool loaded = false;

#if defined(_WIN32)

    // At init, on windows
    if (HMODULE mod = GetModuleHandleA("renderdoc.dll"))
    {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI =
            (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
        loaded = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_6_0, (void**)&rdoc_api) == 1;
    }

#elif defined(__linux__)

    // At init, on linux/android.
    // For android replace librenderdoc.so with libVkLayer_GLES_RenderDoc.so
    if (void* mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD))
    {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
        loaded = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_6_0, (void**)&rdoc_api) == 1;
    }

#endif

    //if (!loaded) {
    //    spdlog::warn("Could not initialize Renderdoc Capture. To capture, start the program from Renderdoc");
    //}
    //else {
    //    spdlog::info("Renderdoc Capture initialized");
    //}
}

void RenderdocCapture::start_capture_frame()
{
#ifndef __EMSCRIPTEN__
    if (rdoc_api) {
        rdoc_api->StartFrameCapture(NULL, NULL);
        capture_started = true;
    }
    else {
        //spdlog::error("Can not start frame capture, Renderdoc Capture not initialized");
    }
#endif
}

void RenderdocCapture::end_capture_frame()
{
#ifndef __EMSCRIPTEN__
    if (rdoc_api) {
        rdoc_api->EndFrameCapture(NULL, NULL);
        capture_started = false;
    }
#endif
}

bool RenderdocCapture::is_capture_started()
{
    return capture_started;
}
