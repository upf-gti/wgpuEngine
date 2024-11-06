#pragma once

#if !defined(__EMSCRIPTEN__)
// NOTE: Uncomment this in order to force the XR Support
// Otherwise only available on Windows
//#define XR_SUPPORT
//#define USE_MIRROR_WINDOW
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5_webgpu.h>
#else
#include <webgpu/webgpu.h>
#endif

enum eEYE {
    EYE_LEFT,
    EYE_RIGHT,
    EYE_COUNT // Let's assume this will never be different to 2...
};
