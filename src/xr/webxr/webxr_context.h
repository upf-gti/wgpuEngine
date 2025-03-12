#pragma once

#include "xr/xr_context.h"

#include "includes.h"

#ifdef WEBXR_SUPPORT

#include "webxr.h"

struct WebXRContext : public XRContext {

    virtual ~WebXRContext();

    /*
    * XR General
    */

    bool initialized = false;

    bool init(WebGPUContext* webgpu_context) override;
    void clean() override;

    /*
    * XR Input
    */

    //sInputState input_state;

    //void init_actions(XrInputData& data);
    //void poll_actions(XrInputData& data);

    //void apply_haptics(uint8_t controller, float amplitude, float duration);
    //void stop_haptics(uint8_t controller);

    /*
    * XR Session
    */

    bool begin_session() override;
    bool end_session() override;

    /*
    * Render
    */

    void update_views(WebXRView views[2], WGPUTextureView texture_view_left, WGPUTextureView texture_view_right);

    WGPUTextureView swapchain_views[2];

    WGPUTextureView get_swapchain_view(uint8_t eye_idx) override;

    // inverted for reverse-z
    float z_near = 1000.0f;
    float z_far = 0.01f;

    void update() override;

    /*
    * Debug & Errors
    */

    void print_viewconfig_view_info() override;

    void print_reference_spaces() override;

    void print_error(int error);

private:

};

#else

struct WebXRContext {

};

#endif
