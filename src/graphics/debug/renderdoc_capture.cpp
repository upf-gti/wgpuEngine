#include "renderdoc_capture.h"

#ifndef __EMSCRIPTEN__

#if defined(_WIN32)

#include "windows.h"

#elif defined(__linux__)

#include <dlfcn.h>

#endif

#include "spdlog/spdlog.h"

RENDERDOC_API_1_6_0* RenderdocCapture::rdoc_api = nullptr;

void RenderdocCapture::init()
{
    bool loaded = false;

    std::string library_name;

#if defined(_WIN32)

    library_name = "renderdoc.dll";

    // At init, on windows
    if (HMODULE mod = GetModuleHandleA(library_name.c_str()))
    {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI =
            (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
        loaded = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_6_0, (void**)&rdoc_api) == 1;
    }

#elif defined(__linux__)

    library_name = "librenderdoc.so";

    // At init, on linux/android.
    // For android replace librenderdoc.so with libVkLayer_GLES_RenderDoc.so
    if (void* mod = dlopen(library_name.c_str(), RTLD_NOW | RTLD_NOLOAD))
    {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
        loaded = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_6_0, (void**)&rdoc_api) == 1;
    }

#endif

    if (!loaded) {
        spdlog::warn("Could not initialize Renderdoc Capture. To capture, start the program from Renderdoc");
    }
    else {
        spdlog::info("Renderdoc Capture initialized");
    }
}

void RenderdocCapture::start_capture_frame()
{
    if (rdoc_api) {
        rdoc_api->StartFrameCapture(NULL, NULL);
    }
    else {
        //spdlog::error("Can not start frame capture, Renderdoc Capture not initialized");
    }
}

void RenderdocCapture::end_capture_frame()
{
    if (rdoc_api) {
        rdoc_api->EndFrameCapture(NULL, NULL);
    }
}

#endif
