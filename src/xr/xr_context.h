#pragma once

#include <vector>
#include <array>

#include "includes.h"

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"

#ifdef XR_SUPPORT

struct WebGPUContext;

class Transform;

struct XRContext {

    Transform* root_transform = nullptr;

    /*
    * XR General
    */

    bool initialized = false;

    virtual bool init(WebGPUContext* webgpu_context) = 0;
    virtual void clean() = 0;

    /*
    * XR Input
    */

    //sInputState input_state;

    //virtual void init_actions(XrInputData& data) {};
    //virtual void poll_actions(XrInputData& data) {};

    //virtual void apply_haptics(uint8_t controller, float amplitude, float duration) {};
    //virtual void stop_haptics(uint8_t controller) {};

    /*
    * XR Session
    */

    virtual bool begin_session() = 0;
    virtual bool end_session() = 0;

    /*
    * Render
    */

    virtual WGPUTextureView get_swapchain_view(uint8_t eye_idx) = 0;
    virtual uint32_t get_swapchain_image_index(uint8_t eye_idx) {};

    // inverted for reverse-z
    float z_near = 1000.0f;
    float z_far = 0.01f;

    glm::ivec4 viewport;

    //std::vector<sSwapchainData> swapchains;
    //std::vector<XrView> views;
    //std::vector<XrViewConfigurationView> viewconfig_views;
    //std::vector<XrCompositionLayerProjectionView> projection_views;
    //std::vector<int64_t> swapchain_formats;

    struct sViewData {
        glm::vec3   position;
        glm::mat4x4 projection_matrix;
        glm::mat4x4 view_matrix;
        glm::mat4x4 view_projection_matrix;
    };

    std::vector<sViewData> per_view_data;

    virtual void init_frame() {};
    virtual void acquire_swapchain(int swapchain_index) {};
    virtual void release_swapchain(int swapchain_index) {};
    virtual void end_frame() {};

    virtual void update() = 0;

    /*
    * Debug & Errors
    */

    virtual void print_viewconfig_view_info() = 0;

    virtual void print_reference_spaces() = 0;

private:

};

#else

struct XRContext {

};

#endif
