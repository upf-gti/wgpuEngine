#include "renderdoc_app.h"

class RenderdocCapture {

    static RENDERDOC_API_1_6_0* rdoc_api;

public:

    static void init();
    static void start_capture_frame();
    static void end_capture_frame();

};
