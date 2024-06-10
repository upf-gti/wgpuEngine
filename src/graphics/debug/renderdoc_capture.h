#ifndef __EMSCRIPTEN__
#include "renderdoc_app.h"
#endif

class RenderdocCapture {

#ifndef __EMSCRIPTEN__
    static RENDERDOC_API_1_6_0* rdoc_api;
#endif

    static bool capture_started;

public:

    static void init();
    static void start_capture_frame();
    static void end_capture_frame();
    static bool is_capture_started();
};

