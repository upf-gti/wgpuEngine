#pragma once

#include "xr_context.h"

#ifdef XR_SUPPORT

struct WebXRContext : public XRContext {

    /*
    * XR General
    */

    bool initialized = false;

    virtual bool init(WebGPUContext* webgpu_context) override;
    virtual void clean() override;

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

    virtual bool begin_session() override;
    virtual bool end_session() override;

    /*
    * Render
    */

    virtual WGPUTextureView get_swapchain_view(uint8_t eye_idx);

    // inverted for reverse-z
    float z_near = 1000.0f;
    float z_far = 0.01f;

    virtual void update() override;

    /*
    * Debug & Errors
    */

    virtual void print_viewconfig_view_info() override;

    virtual void print_reference_spaces() override;

private:

};

#else

struct XRContext {

};

#endif
