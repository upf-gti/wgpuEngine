#pragma once

#include <vector>
#include <array>

#include "includes.h"

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/quaternion.hpp"

// Small helper so we don't forget whether we treat 0 as left or right hand
enum XR_HANDS
{
    HAND_LEFT = 0,
    HAND_RIGHT = 1,
    HAND_COUNT
};

enum XR_POSES {
    POSE_GRIP = 0,
    POSE_AIM
};

enum XR_BUTTONS {
    XR_BUTTON_A = 0,
    XR_BUTTON_B,
    XR_BUTTON_X,
    XR_BUTTON_Y,
    XR_BUTTON_MENU,
    XR_BUTTON_COUNT,
};

enum XR_THUMBSTICK_AXIS : uint8_t {
    XR_THUMBSTICK_NO_AXIS = 0,
    XR_THUMBSTICK_AXIS_X,
    XR_THUMBSTICK_AXIS_Y
};

#ifdef XR_SUPPORT

struct WebGPUContext;

class Transform;

struct XrInputPose {
    glm::quat orientation = { 0.0, 0.0, 0.0, 1.0 };
    glm::vec3 position = {};
};

glm::mat4x4 XrInputPose_to_glm(const XrInputPose& p);

struct XRContext {

    virtual ~XRContext();

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

    // Poses
    glm::mat4x4 headPoseMatrix = glm::identity<glm::mat4x4>();
    XrInputPose headPose = {};
    glm::mat4x4 controllerAimPoseMatrices[HAND_COUNT] = { glm::identity<glm::mat4x4>(), glm::identity<glm::mat4x4>() };
    XrInputPose controllerAimPoses[HAND_COUNT] = {};
    glm::mat4x4 controllerGripPoseMatrices[HAND_COUNT] = { glm::identity<glm::mat4x4>(), glm::identity<glm::mat4x4>() };
    XrInputPose controllerGripPoses[HAND_COUNT] = {};

    //sInputState input_state;

    //virtual void init_actions(XrInputData& data) {};
    virtual void poll_actions() {};

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

    virtual WGPUTextureView get_swapchain_view(uint8_t eye_idx, uint32_t image_idx) = 0;
    virtual WGPUTextureView get_swapchain_view(uint8_t eye_idx) = 0;
    virtual uint32_t get_swapchain_image_index(uint8_t eye_idx);

    // inverted for reverse-z
    float z_near = 1000.0f;
    float z_far = 0.01f;

    glm::ivec4 viewport;

    struct sViewData {
        glm::vec3   position;
        glm::mat4x4 projection_matrix;
        glm::mat4x4 view_matrix;
        glm::mat4x4 view_projection_matrix;
    };

    std::vector<sViewData> per_view_data;

    virtual void init_frame();
    virtual void acquire_swapchain(int swapchain_index);
    virtual void release_swapchain(int swapchain_index);
    virtual void end_frame();

    virtual uint32_t get_num_images_per_swapchain();

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
