#include "openxr_context.h"

#ifdef OPENXR_SUPPORT

#include <cstdarg>

#include "framework/input.h"
#include "framework/math/transform.h"

#include "graphics/webgpu_context.h"

#include "spdlog/spdlog.h"

// we need an identity pose for creating spaces without offsets
static XrPosef identity_pose = { .orientation = {.x = 0, .y = 0, .z = 0, .w = 1.0},
                                 .position = {.x = 0, .y = 0, .z = 0} };

// Helper functions for pose to GLM
glm::mat4x4 OpenXRProjection_to_glm(const XrFovf& fov, float nearZ, float farZ);
glm::mat4x4 XrInputPose_to_glm(const XrPosef& p);
inline XrInputPose OpenXRPose_to_XrInputPose(const XrPosef& xrPosef);
glm::quat slerp(const glm::quat& start, const glm::quat& end, float percent);
glm::vec3 slerp(const glm::vec3& start, const glm::vec3& end, float percent);

OpenXRContext::~OpenXRContext() {
}

bool OpenXRContext::init(WebGPUContext* webgpu_context)
{
    XrResult result;

    uint32_t blend_modes_count = 0;
    result = xrEnumerateEnvironmentBlendModes(instance, system_id, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &blend_modes_count, NULL);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Failed to enumerate number of blend modes");
        return false;
    }

    std::vector<XrEnvironmentBlendMode> blendModes(blend_modes_count);
    result = xrEnumerateEnvironmentBlendModes(instance, system_id, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, blend_modes_count, &blend_modes_count, blendModes.data());

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Failed to enumerate blend modes");
        return false;
    }

    if (check_backend_requirements()) {
        return 1;
    }

    view_count = 0;
    result = xrEnumerateViewConfigurationViews(instance, system_id, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &view_count, NULL);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Failed to get view configuration count");
        return false;
    }

    views.resize(view_count, { XR_TYPE_VIEW });
    viewconfig_views.resize(view_count, { XR_TYPE_VIEW_CONFIGURATION_VIEW, nullptr });
    projection_views.resize(view_count);
    per_view_data.resize(view_count);

    result = xrEnumerateViewConfigurationViews(instance, system_id, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, view_count, &view_count, viewconfig_views.data());

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Failed to enumerate view configuration views!");
        return false;
    }

    print_viewconfig_view_info();

    dawnxr::GraphicsBindingDawn binding = { .device = webgpu_context->device };

    XrSessionCreateInfo xrCreateInfo = {
        .type = XR_TYPE_SESSION_CREATE_INFO,
        .next = &binding,
        .systemId = system_id
    };
    dawnxr::createSession(instance, &xrCreateInfo, &session);

    uint32_t swapchain_format_count;
    result = dawnxr::enumerateSwapchainFormats(session, 0, &swapchain_format_count, NULL);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Failed to get number of supported swapchain formats");
        return false;
    }

    printf("Runtime supports %d swapchain formats\n", swapchain_format_count);
    swapchain_formats.resize(swapchain_format_count);
    result = dawnxr::enumerateSwapchainFormats(session, swapchain_format_count, &swapchain_format_count, swapchain_formats.data());

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Failed to enumerate swapchain formats");
        return false;
    }

    //webgpu_context->swapchain_format = static_cast<WGPUTextureFormat>(swapchain_formats[0]);

    //config_render_pipeline();

    XrSwapchainCreateInfo swapchain_create_info{
        .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
        .next = NULL,
        .createFlags = 0,
        .usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
        .format = swapchain_formats[0],
        .sampleCount = viewconfig_views[0].recommendedSwapchainSampleCount,
        .width = viewconfig_views[0].recommendedImageRectWidth,
        .height = viewconfig_views[0].recommendedImageRectHeight,
        .faceCount = 1,
        .arraySize = 1,
        .mipCount = 1
    };

    swapchains.resize(view_count);

    for (uint32_t i = 0; i < view_count; i++) {
        dawnxr::createSwapchain(session, &swapchain_create_info, &swapchains[i].swapchain);

        result = dawnxr::enumerateSwapchainImages(swapchains[i].swapchain, 0, &swapchain_length, nullptr);

        if (!XR_SUCCEEDED(result))
        {
            spdlog::error("Failed to enumerate swapchains");
            return false;
        }

        // these are wrappers for the actual OpenGL texture id
        swapchains[i].images.resize(swapchain_length);
        result = dawnxr::enumerateSwapchainImages(swapchains[i].swapchain, swapchain_length, &swapchain_length, (XrSwapchainImageBaseHeader*)swapchains[i].images.data());

        if (!XR_SUCCEEDED(result))
        {
            spdlog::error("Failed to enumerate swapchain images");
            return false;
        }
    }

    XrReferenceSpaceType play_space_type = XR_REFERENCE_SPACE_TYPE_STAGE;
    // We could check if our ref space type is supported, but next call will error anyway if not
    print_reference_spaces();

    XrReferenceSpaceCreateInfo play_space_create_info = {
        .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
        .referenceSpaceType = play_space_type, //play_space_type,
        .poseInReferenceSpace = identity_pose
    };

    result = xrCreateReferenceSpace(session, &play_space_create_info, &play_space);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Failed to create play space!");
        return false;
    }

    projection_views.resize(view_count);
    for (uint32_t i = 0; i < view_count; i++) {
        projection_views[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        projection_views[i].next = NULL;
        projection_views[i].pose = { .orientation = { 0.0f, 0.0f, 0.0f, 1.0f }, .position = { 0.0f, 0.0f, 0.0f } };
        projection_views[i].subImage.swapchain = swapchains[i].swapchain;
        projection_views[i].subImage.imageArrayIndex = 0;
        projection_views[i].subImage.imageRect.offset.x = 0;
        projection_views[i].subImage.imageRect.offset.y = 0;
        projection_views[i].subImage.imageRect.extent.width = viewconfig_views[i].recommendedImageRectWidth;
        projection_views[i].subImage.imageRect.extent.height = viewconfig_views[i].recommendedImageRectHeight;

        // projection_views[i].pose (and fov) have to be filled every frame in frame loop
    };

    viewport = glm::ivec4(0, 0, viewconfig_views[0].recommendedImageRectWidth, viewconfig_views[0].recommendedImageRectHeight);

    if (Input::init_xr(this)) {
        spdlog::error("Can't initialize OpenXR input");
        return 1;
    }

    initialized = true;

    return true;
}

void OpenXRContext::clean()
{
    if (!initialized) {
        return;
    }

    for (uint32_t i = 0; i < view_count; ++i) {
        xrDestroySwapchain(swapchains[i].swapchain);
    }

    xrDestroySpace(play_space);
    xrDestroySession(session);
    xrDestroyInstance(instance);
}

bool OpenXRContext::create_instance()
{
    XrResult result;

    uint32_t extension_count = 0;
    result = xrEnumerateInstanceExtensionProperties(NULL, 0, &extension_count, NULL);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Could not initilize OpenXR: Failed to enumerate number of extension properties");
        return false;
    }

    std::vector<XrExtensionProperties> extensionProperties(extension_count, { XR_TYPE_EXTENSION_PROPERTIES, nullptr });
    result = xrEnumerateInstanceExtensionProperties(NULL, extension_count, &extension_count, extensionProperties.data());

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Failed to enumerate extension properties");
        return false;
    }

    uint32_t layer_count = 0;
    result = xrEnumerateApiLayerProperties(0, &layer_count, NULL);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Failed to enumerate layer properties");
        return false;
    }

    std::vector<XrApiLayerProperties> layerProperties(layer_count, { XR_TYPE_API_LAYER_PROPERTIES, nullptr });
    result = xrEnumerateApiLayerProperties(layer_count, &layer_count, layerProperties.data());

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Failed to enumerate extension properties");
        return false;
    }

    bool vulkan_ext = false;
    bool dx12_ext = false;

    std::string extensions = "OpenXR extensions:\n";
    for (const auto& ext : extensionProperties) {
        extensions += "\t " + std::string(ext.extensionName) + " " + std::to_string(ext.extensionVersion) + "\n";

        if (strcmp("XR_KHR_vulkan_enable2", ext.extensionName) == 0) {
            vulkan_ext = true;
        }

        if (strcmp("XR_KHR_D3D12_enable", ext.extensionName) == 0) {
            dx12_ext = true;
        }

        if (strcmp(XR_EXT_HAND_TRACKING_EXTENSION_NAME, ext.extensionName) == 0) {
            hand_tracking.supported = true;
        }

        //if (strcmp(XR_KHR_COMPOSITION_LAYER_CYLINDER_EXTENSION_NAME, ext.extensionName) == 0) {
        //    cylinder.supported = true;
        //}

        //if (strcmp(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME, ext.extensionName) == 0) {
        //    depth.supported = true;
        //}
    }

    spdlog::trace(extensions);

    // A graphics extension is required to draw anything in VR

    uint32_t enabled_ext_count = 1;

#if defined(BACKEND_VULKAN)
    if (!vulkan_ext) {
        spdlog::error("Runtime does not support Vulkan extension!");
        return false;
    }

    const char* enabled_exts[3] = { "XR_KHR_vulkan_enable2" };

#elif defined(BACKEND_DX12)
    if (!dx12_ext) {
        spdlog::error("Runtime does not support DX12 extension!");
        return false;
    }

    const char* enabled_exts[3] = { "XR_KHR_D3D12_enable" };

#endif

    if (hand_tracking.supported) {
        enabled_exts[enabled_ext_count++] = XR_EXT_HAND_TRACKING_EXTENSION_NAME;
    }
    //if (cylinder.supported) {
    //    enabled_exts[enabled_ext_count++] = XR_KHR_COMPOSITION_LAYER_CYLINDER_EXTENSION_NAME;
    //}

    XrInstanceCreateInfo instance_create_info = {
        .type = XR_TYPE_INSTANCE_CREATE_INFO,
        .next = nullptr,
        .createFlags = 0,
        .applicationInfo = {
            "wgpuEngine", 1,
            "Custom", 0,
            XR_API_VERSION_1_0,
        },
        .enabledApiLayerCount = 0,
        .enabledApiLayerNames = NULL,
        .enabledExtensionCount = enabled_ext_count,
        .enabledExtensionNames = enabled_exts
    };

    result = xrCreateInstance(&instance_create_info, &instance);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Failed to create XR instance");
        return false;
    }

    XrInstanceProperties instance_props = {
        .type = XR_TYPE_INSTANCE_PROPERTIES,
        .next = NULL,
    };

    result = xrGetInstanceProperties(instance, &instance_props);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Failed to get instance info");
        return false;
    }

    spdlog::info("Runtime Name: {}", instance_props.runtimeName);
    spdlog::info("Runtime Version: {}.{}.{}", XR_VERSION_MAJOR(instance_props.runtimeVersion),
        XR_VERSION_MINOR(instance_props.runtimeVersion),
        XR_VERSION_PATCH(instance_props.runtimeVersion));

    XrSystemGetInfo system_get_info = { .type = XR_TYPE_SYSTEM_GET_INFO, .next = NULL, .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY };
    result = xrGetSystem(instance, &system_get_info, &system_id);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::warn("Failed to get system for HMD form factor. Make sure the headset is properly connected");
        return false;
    }

    return true;
}

bool OpenXRContext::begin_session()
{
    // Start the session
    XrSessionBeginInfo session_begin_info = {
        .type = XR_TYPE_SESSION_BEGIN_INFO,
        .next = NULL,
        .primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO
    };
    const XrResult result = xrBeginSession(session, &session_begin_info);
    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Failed to begin session!");
        return false;
    }

    return true;
}

bool OpenXRContext::end_session()
{
    // End the session
    const XrResult result = xrEndSession(session);
    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Failed to end session!");
        return false;
    }

    return true;
}

WGPUTextureView OpenXRContext::get_swapchain_view(uint8_t eye_idx, uint32_t image_idx)
{
    const sSwapchainData& swapchainData = swapchains[eye_idx];
    return swapchainData.images[image_idx].textureView;
}

WGPUTextureView OpenXRContext::get_swapchain_view(uint8_t eye_idx)
{
    const sSwapchainData& swapchainData = swapchains[eye_idx];
    return swapchainData.images[swapchainData.image_index].textureView;
}

uint32_t OpenXRContext::get_swapchain_image_index(uint8_t eye_idx)
{
    return swapchains[eye_idx].image_index;
}

void OpenXRContext::print_viewconfig_view_info()
{
    for (uint32_t i = 0; i < viewconfig_views.size(); i++) {
        printf("View Configuration View %d:\n", i);
        printf("\tResolution       : Recommended %dx%d, Max: %dx%d\n",
            viewconfig_views[0].recommendedImageRectWidth,
            viewconfig_views[0].recommendedImageRectHeight,
            viewconfig_views[0].maxImageRectWidth,
            viewconfig_views[0].maxImageRectHeight);
        printf("\tSwapchain Samples: Recommended: %d, Max: %d)\n",
            viewconfig_views[0].recommendedSwapchainSampleCount,
            viewconfig_views[0].maxSwapchainSampleCount);
    }
}

int OpenXRContext::check_backend_requirements()
{
    XrResult result;

#if defined(BACKEND_VULKAN)
    XrGraphicsRequirementsVulkanKHR vulkan_reqs = { .type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR };

    PFN_xrGetVulkanGraphicsRequirements2KHR pfnGetVulkanGraphicsRequirements2KHR = NULL;
    {
        result = xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsRequirements2KHR", (PFN_xrVoidFunction*)&pfnGetVulkanGraphicsRequirements2KHR);
        if (!XR_SUCCEEDED(result))
        {
            spdlog::error("Failed to get Vulkan graphics requirements function!");
            return false;
        }
    }

    pfnGetVulkanGraphicsRequirements2KHR(instance, system_id, &vulkan_reqs);

    check_vulkan_version(&vulkan_reqs);
#elif defined(BACKEND_DX12)

    XrGraphicsRequirementsD3D12KHR dx12_reqs = { .type = XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR };

    PFN_xrGetD3D12GraphicsRequirementsKHR pfnGetD3D12GraphicsRequirements2KHR = NULL;
    {
        result = xrGetInstanceProcAddr(instance, "xrGetD3D12GraphicsRequirementsKHR", (PFN_xrVoidFunction*)&pfnGetD3D12GraphicsRequirements2KHR);
        if (!XR_SUCCEEDED(result))
        {
            spdlog::error("Failed to get DX12 graphics requirements function!");
            return false;
        }
    }

    pfnGetD3D12GraphicsRequirements2KHR(instance, system_id, &dx12_reqs);

    check_dx12_version(&dx12_reqs);
#endif

    return 0;
}

#if defined(BACKEND_VULKAN)
bool OpenXRContext::check_vulkan_version(XrGraphicsRequirementsVulkanKHR* vulkan_reqs)
{
    XrVersion desired_vulkan_version = XR_MAKE_VERSION(1, 0, 0);
    if (desired_vulkan_version > vulkan_reqs->maxApiVersionSupported ||
        desired_vulkan_version < vulkan_reqs->minApiVersionSupported) {
        printf(
            "We want Vulkan %d.%d.%d, but runtime only supports Vulkan %d.%d.%d - %d.%d.%d!\n",
            XR_VERSION_MAJOR(desired_vulkan_version), XR_VERSION_MINOR(desired_vulkan_version),
            XR_VERSION_PATCH(desired_vulkan_version),
            XR_VERSION_MAJOR(vulkan_reqs->minApiVersionSupported),
            XR_VERSION_MINOR(vulkan_reqs->minApiVersionSupported),
            XR_VERSION_PATCH(vulkan_reqs->minApiVersionSupported),
            XR_VERSION_MAJOR(vulkan_reqs->maxApiVersionSupported),
            XR_VERSION_MINOR(vulkan_reqs->maxApiVersionSupported),
            XR_VERSION_PATCH(vulkan_reqs->maxApiVersionSupported));
        return false;
    }
    return true;
}
#endif

#if defined(BACKEND_DX12)
bool OpenOpenXRContext::check_dx12_version(XrGraphicsRequirementsD3D12KHR* dx12_reqs)
{
    spdlog::info("### D3D12 graphics requirements minFeatureLevel: {}", static_cast<uint32_t>(dx12_reqs->minFeatureLevel));
    spdlog::info("### D3D12 graphics requirements adapterLuid: {} {}", dx12_reqs->adapterLuid.HighPart, dx12_reqs->adapterLuid.LowPart);

    return true;
}
#endif

void OpenXRContext::print_reference_spaces()
{
    XrResult result;

    uint32_t ref_space_count;
    result = xrEnumerateReferenceSpaces(session, 0, &ref_space_count, NULL);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Getting number of reference spaces failed!");
        return;
    }

    std::vector<XrReferenceSpaceType> ref_spaces(ref_space_count);
    result = xrEnumerateReferenceSpaces(session, ref_space_count, &ref_space_count, ref_spaces.data());

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Enumerating reference spaces failed!");
        return;
    }

    spdlog::info("Runtime supports {} reference spaces:", ref_space_count);
    for (uint32_t i = 0; i < ref_space_count; i++) {
        if (ref_spaces[i] == XR_REFERENCE_SPACE_TYPE_LOCAL) {
            spdlog::info("\tXR_REFERENCE_SPACE_TYPE_LOCAL");
        }
        else if (ref_spaces[i] == XR_REFERENCE_SPACE_TYPE_STAGE) {
            spdlog::info("\tXR_REFERENCE_SPACE_TYPE_STAGE");
        }
        else if (ref_spaces[i] == XR_REFERENCE_SPACE_TYPE_VIEW) {
            spdlog::info("\tXR_REFERENCE_SPACE_TYPE_VIEW");
        }
        else {
            spdlog::info("\tOther (extension?) refspace %{}", static_cast<uint32_t>(ref_spaces[i]));
        }
    }
}

void OpenXRContext::init_actions()
{
    XrResult result;

    // Create an action set.
    {
        XrActionSetCreateInfo actionSetInfo{
            .type = XR_TYPE_ACTION_SET_CREATE_INFO,
            .actionSetName = "gameplay",
            .localizedActionSetName = "Gameplay",
            .priority = 0
        };
        result = xrCreateActionSet(instance, &actionSetInfo, &input_state.actionSet);

        if (!XR_SUCCEEDED(result))
        {
            spdlog::error("Cannot create Action Set");
            return;
        }
    }

    // Get the XrPath for the left and right hands - we will use them as subaction paths.
    xrStringToPath(instance, "/user/hand/left", &input_state.handSubactionPath[HAND_LEFT]);
    xrStringToPath(instance, "/user/hand/right", &input_state.handSubactionPath[HAND_RIGHT]);

    // Create actions.
    {
        bool is_ok = true;

        // Create an input action getting the left and right hand poses (aim)
        is_ok &= create_action(input_state.actionSet, input_state.handSubactionPath, HAND_COUNT,
            "hand_pose", "Hand Pose", XR_ACTION_TYPE_POSE_INPUT, input_state.aimPoseAction);

        // (grip)
        is_ok &= create_action(input_state.actionSet, input_state.handSubactionPath, HAND_COUNT,
            "grip_pose", "Grip Pose", XR_ACTION_TYPE_POSE_INPUT, input_state.gripPoseAction);

        // Create output actions for vibrating the left and right controller.
        is_ok &= create_action(input_state.actionSet, input_state.handSubactionPath, HAND_COUNT,
            "haptic_action", "Haptic Action", XR_ACTION_TYPE_VIBRATION_OUTPUT, input_state.vibrateAction);

        // Create an input action for grabbing objects with the left and right hands.
        is_ok &= create_action(input_state.actionSet, input_state.handSubactionPath, HAND_COUNT,
            "grab_action", "Grab Action", XR_ACTION_TYPE_FLOAT_INPUT, input_state.grabAction);

        // Create an input action getting the left and right thumbsticks
        is_ok &= create_action(input_state.actionSet, input_state.handSubactionPath, HAND_COUNT,
            "thumbstick_action", "Thumbstick Action", XR_ACTION_TYPE_VECTOR2F_INPUT, input_state.thumbstickValueAction);

        // thumbsticks (click)
        is_ok &= create_action(input_state.actionSet, input_state.handSubactionPath, HAND_COUNT,
            "thumbstick_click_action", "Thumbstick Click Action", XR_ACTION_TYPE_BOOLEAN_INPUT, input_state.thumbstickClickAction);

        // thumbsticks (touch)
        is_ok &= create_action(input_state.actionSet, input_state.handSubactionPath, HAND_COUNT,
            "thumbstick_touch_action", "Thumbstick Touch Action", XR_ACTION_TYPE_BOOLEAN_INPUT, input_state.thumbstickTouchAction);

        // Create an input action getting the left and right triggers
        is_ok &= create_action(input_state.actionSet, input_state.handSubactionPath, HAND_COUNT,
            "trigger_action", "Trigger Action", XR_ACTION_TYPE_FLOAT_INPUT, input_state.triggerValueAction);

        // triggers (touch)
        is_ok &= create_action(input_state.actionSet, input_state.handSubactionPath, HAND_COUNT,
            "trigger_touch_action", "Trigger Touch Action", XR_ACTION_TYPE_BOOLEAN_INPUT, input_state.triggerTouchAction);

        // Create an input actions getting button states

        for (auto& mb : buttonsState) {
            // Click action
            is_ok &= create_action(input_state.actionSet, &input_state.handSubactionPath[mb.hand], 1,
                mb.name + "_click_action", mb.name + " Click Action", XR_ACTION_TYPE_BOOLEAN_INPUT, mb.click.action);

            if (mb.touch.active) {
                // Touch action
                is_ok &= create_action(input_state.actionSet, &input_state.handSubactionPath[mb.hand], 1,
                    mb.name + "_touch_action", mb.name + " Touch Action", XR_ACTION_TYPE_BOOLEAN_INPUT, mb.touch.action);
            }
        }

        // Create input actions for quitting the session using the left and right controller.
        // Since it doesn't matter which hand did this, we do not specify subaction paths for it.
        // We will just suggest bindings for both hands, where possible.
        /*create_action(input_state.actionSet, nullptr, 0,
            "quit_session", "Quit Session", XR_ACTION_TYPE_BOOLEAN_INPUT, input_state.quitAction);*/

        if (!is_ok)
        {
            spdlog::error("ERROR creating XR actions!");
            return;
        }
    }

    XrPath selectPath[HAND_COUNT];
    XrPath squeezeValuePath[HAND_COUNT];
    XrPath squeezeForcePath[HAND_COUNT];
    XrPath squeezeClickPath[HAND_COUNT];
    XrPath aimPosePath[HAND_COUNT];
    XrPath gripPosePath[HAND_COUNT];
    XrPath hapticPath[HAND_COUNT];
    XrPath menuClickPath[HAND_COUNT];
    XrPath triggerValuePath[HAND_COUNT];
    XrPath triggerTouchPath[HAND_COUNT];
    XrPath thumbstickValuePath[HAND_COUNT];
    XrPath thumbstickClickPath[HAND_COUNT];
    XrPath thumbstickTouchPath[HAND_COUNT];

    xrStringToPath(instance, "/user/hand/left/input/select/click", &selectPath[HAND_LEFT]);
    xrStringToPath(instance, "/user/hand/right/input/select/click", &selectPath[HAND_RIGHT]);
    xrStringToPath(instance, "/user/hand/left/input/squeeze/value", &squeezeValuePath[HAND_LEFT]);
    xrStringToPath(instance, "/user/hand/right/input/squeeze/value", &squeezeValuePath[HAND_RIGHT]);
    xrStringToPath(instance, "/user/hand/left/input/squeeze/force", &squeezeForcePath[HAND_LEFT]);
    xrStringToPath(instance, "/user/hand/right/input/squeeze/force", &squeezeForcePath[HAND_RIGHT]);
    xrStringToPath(instance, "/user/hand/left/input/squeeze/click", &squeezeClickPath[HAND_LEFT]);
    xrStringToPath(instance, "/user/hand/right/input/squeeze/click", &squeezeClickPath[HAND_RIGHT]);
    xrStringToPath(instance, "/user/hand/left/input/aim/pose", &aimPosePath[HAND_LEFT]);
    xrStringToPath(instance, "/user/hand/right/input/aim/pose", &aimPosePath[HAND_RIGHT]);
    xrStringToPath(instance, "/user/hand/left/input/grip/pose", &gripPosePath[HAND_LEFT]);
    xrStringToPath(instance, "/user/hand/right/input/grip/pose", &gripPosePath[HAND_RIGHT]);
    xrStringToPath(instance, "/user/hand/left/output/haptic", &hapticPath[HAND_LEFT]);
    xrStringToPath(instance, "/user/hand/right/output/haptic", &hapticPath[HAND_RIGHT]);
    xrStringToPath(instance, "/user/hand/left/input/menu/click", &menuClickPath[HAND_LEFT]);
    xrStringToPath(instance, "/user/hand/right/input/menu/click", &menuClickPath[HAND_RIGHT]);
    xrStringToPath(instance, "/user/hand/left/input/trigger/value", &triggerValuePath[HAND_LEFT]);
    xrStringToPath(instance, "/user/hand/right/input/trigger/value", &triggerValuePath[HAND_RIGHT]);
    xrStringToPath(instance, "/user/hand/left/input/trigger/touch", &triggerTouchPath[HAND_LEFT]);
    xrStringToPath(instance, "/user/hand/right/input/trigger/touch", &triggerTouchPath[HAND_RIGHT]);
    xrStringToPath(instance, "/user/hand/left/input/thumbstick", &thumbstickValuePath[HAND_LEFT]);
    xrStringToPath(instance, "/user/hand/right/input/thumbstick", &thumbstickValuePath[HAND_RIGHT]);
    xrStringToPath(instance, "/user/hand/left/input/thumbstick/click", &thumbstickClickPath[HAND_LEFT]);
    xrStringToPath(instance, "/user/hand/right/input/thumbstick/click", &thumbstickClickPath[HAND_RIGHT]);
    xrStringToPath(instance, "/user/hand/left/input/thumbstick/touch", &thumbstickTouchPath[HAND_LEFT]);
    xrStringToPath(instance, "/user/hand/right/input/thumbstick/touch", &thumbstickTouchPath[HAND_RIGHT]);

    // Suggest bindings for KHR Simple.
    {
        XrPath khrSimpleInteractionProfilePath;
        xrStringToPath(instance, "/interaction_profiles/khr/simple_controller", &khrSimpleInteractionProfilePath);
        std::vector<XrActionSuggestedBinding> bindings = {// Fall back to a click input for the grab action.
                {input_state.grabAction,        selectPath[HAND_LEFT]},
                {input_state.grabAction,        selectPath[HAND_RIGHT]},
                {input_state.aimPoseAction,     aimPosePath[HAND_LEFT]},
                {input_state.aimPoseAction,     aimPosePath[HAND_RIGHT]},
                {input_state.gripPoseAction,    gripPosePath[HAND_LEFT]},
                {input_state.gripPoseAction,    gripPosePath[HAND_RIGHT]},
                /* {input_state.quitAction,      menuClickPath[HAND_LEFT]},
                 {input_state.quitAction,        menuClickPath[HAND_RIGHT]},*/
                 {input_state.vibrateAction,     hapticPath[HAND_LEFT]},
                 {input_state.vibrateAction,     hapticPath[HAND_RIGHT]} };

        XrInteractionProfileSuggestedBinding suggestedBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
        suggestedBindings.interactionProfile = khrSimpleInteractionProfilePath;
        suggestedBindings.suggestedBindings = bindings.data();
        suggestedBindings.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
        xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
    }
    // Suggest bindings for the Oculus Touch.
    {
        XrPath oculusTouchInteractionProfilePath;
        xrStringToPath(instance, "/interaction_profiles/oculus/touch_controller", &oculusTouchInteractionProfilePath);
        std::vector<XrActionSuggestedBinding> bindings = {
            {input_state.grabAction,            squeezeValuePath[HAND_LEFT]},
            {input_state.grabAction,            squeezeValuePath[HAND_RIGHT]},
            {input_state.aimPoseAction,         aimPosePath[HAND_LEFT]},
            {input_state.aimPoseAction,         aimPosePath[HAND_RIGHT]},
            {input_state.gripPoseAction,        gripPosePath[HAND_LEFT]},
            {input_state.gripPoseAction,        gripPosePath[HAND_RIGHT]},
            //{input_state.quitAction,          menuClickPath[HAND_LEFT]},
            {input_state.thumbstickValueAction, thumbstickValuePath[HAND_LEFT]},
            {input_state.thumbstickValueAction, thumbstickValuePath[HAND_RIGHT]},
            {input_state.thumbstickClickAction, thumbstickClickPath[HAND_LEFT]},
            {input_state.thumbstickClickAction, thumbstickClickPath[HAND_RIGHT]},
            {input_state.thumbstickTouchAction, thumbstickTouchPath[HAND_LEFT]},
            {input_state.thumbstickTouchAction, thumbstickTouchPath[HAND_RIGHT]},
            {input_state.triggerValueAction,    triggerValuePath[HAND_LEFT]},
            {input_state.triggerValueAction,    triggerValuePath[HAND_RIGHT]},
            {input_state.triggerTouchAction,    triggerTouchPath[HAND_LEFT]},
            {input_state.triggerTouchAction,    triggerTouchPath[HAND_RIGHT]},
            {input_state.vibrateAction,         hapticPath[HAND_LEFT]},
            {input_state.vibrateAction,         hapticPath[HAND_RIGHT]}
        };

        // Add button mappings.
        for (auto& mb : buttonsState)
        {
            bindings.push_back({ mb.click.action, mb.click.path });
            if (mb.touch.active) bindings.push_back({ mb.touch.action, mb.touch.path });
        }

        XrInteractionProfileSuggestedBinding suggestedBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
        suggestedBindings.interactionProfile = oculusTouchInteractionProfilePath;
        suggestedBindings.suggestedBindings = bindings.data();
        suggestedBindings.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
        result = xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
    }
    // Suggest bindings for the Vive Controller.
    /*{
        XrPath viveControllerInteractionProfilePath;
        (
                xrStringToPath(instance, "/interaction_profiles/htc/vive_controller", &viveControllerInteractionProfilePath));
        std::vector<XrActionSuggestedBinding> bindings{{{input_state.grabAction, triggerValuePath[LEFT]},
                                                        {input_state.grabAction, triggerValuePath[RIGHT]},
                                                        {input_state.poseAction, posePath[LEFT]},
                                                        {input_state.poseAction, posePath[RIGHT]},
                                                        {input_state.quitAction, menuClickPath[LEFT]},
                                                        {input_state.quitAction, menuClickPath[RIGHT]},
                                                        {input_state.vibrateAction, hapticPath[LEFT]},
                                                        {input_state.vibrateAction, hapticPath[RIGHT]}}};
        XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        suggestedBindings.interactionProfile = viveControllerInteractionProfilePath;
        suggestedBindings.suggestedBindings = bindings.data();
        suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
        (xrSuggestInteractionProfileBindings(instance, &suggestedBindings));
    }

    // Suggest bindings for the Valve Index Controller.
    {
        XrPath indexControllerInteractionProfilePath;
        (
                xrStringToPath(instance, "/interaction_profiles/valve/index_controller", &indexControllerInteractionProfilePath));
        std::vector<XrActionSuggestedBinding> bindings{{{input_state.grabAction, squeezeForcePath[LEFT]},
                                                        {input_state.grabAction, squeezeForcePath[RIGHT]},
                                                        {input_state.poseAction, posePath[LEFT]},
                                                        {input_state.poseAction, posePath[RIGHT]},
                                                        {input_state.quitAction, bClickPath[LEFT]},
                                                        {input_state.quitAction, bClickPath[RIGHT]},
                                                        {input_state.vibrateAction, hapticPath[LEFT]},
                                                        {input_state.vibrateAction, hapticPath[RIGHT]}}};
        XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        suggestedBindings.interactionProfile = indexControllerInteractionProfilePath;
        suggestedBindings.suggestedBindings = bindings.data();
        suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
        (xrSuggestInteractionProfileBindings(instance, &suggestedBindings));
    }

    // Suggest bindings for the Microsoft Mixed Reality Motion Controller.
    {
        XrPath microsoftMixedRealityInteractionProfilePath;
        (xrStringToPath(instance, "/interaction_profiles/microsoft/motion_controller",
                                    &microsoftMixedRealityInteractionProfilePath));
        std::vector<XrActionSuggestedBinding> bindings{{{input_state.grabAction, squeezeClickPath[LEFT]},
                                                        {input_state.grabAction, squeezeClickPath[RIGHT]},
                                                        {input_state.poseAction, posePath[LEFT]},
                                                        {input_state.poseAction, posePath[RIGHT]},
                                                        {input_state.quitAction, menuClickPath[LEFT]},
                                                        {input_state.quitAction, menuClickPath[RIGHT]},
                                                        {input_state.vibrateAction, hapticPath[LEFT]},
                                                        {input_state.vibrateAction, hapticPath[RIGHT]}}};
        XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        suggestedBindings.interactionProfile = microsoftMixedRealityInteractionProfilePath;
        suggestedBindings.suggestedBindings = bindings.data();
        suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
        (xrSuggestInteractionProfileBindings(instance, &suggestedBindings));
    }*/

    // Action Spaces for each controller. They will contain the controller poses.
    for (int ci = 0u; ci < HAND_COUNT; ci++)
    {
        XrActionSpaceCreateInfo actionSpaceInfo{
            .type = XR_TYPE_ACTION_SPACE_CREATE_INFO,
            .action = input_state.aimPoseAction,
            .subactionPath = input_state.handSubactionPath[ci]
        };
        actionSpaceInfo.poseInActionSpace.orientation = { .x = 0, .y = 0, .z = 0, .w = 1.0 };
        actionSpaceInfo.poseInActionSpace.position = { .x = 0, .y = 0, .z = 0 };

        XrResult result = xrCreateActionSpace(session, &actionSpaceInfo, &input_state.aimHandSpace[ci]);

        if (!XR_SUCCEEDED(result))
        {
            spdlog::error("Can't create aim action space for controller {}", ci);
            return;
        }

        actionSpaceInfo.action = input_state.gripPoseAction;
        result = xrCreateActionSpace(session, &actionSpaceInfo, &input_state.gripHandSpace[ci]);

        if (!XR_SUCCEEDED(result))
        {
            spdlog::error("Can't create grip action space for controller {}", ci);
            return;
        }
    }

    // Attach the controller action set
    XrSessionActionSetsAttachInfo xrSessionAttachInfo{
        .type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO,
        .countActionSets = 1,
        .actionSets = &input_state.actionSet
    };
    result = xrAttachSessionActionSets(session, &xrSessionAttachInfo);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Cannot attach Action Sets to session");
        return;
    }
}

void OpenXRContext::poll_actions()
{
    // Sync the actions 
    std::vector<XrActiveActionSet> activeActionSets = {
        { input_state.actionSet, XR_NULL_PATH }
    };

    XrActionsSyncInfo actionsSyncInfo = { .type = XR_TYPE_ACTIONS_SYNC_INFO,
                                        .countActiveActionSets = 1u,
                                        .activeActionSets = activeActionSets.data() };

    XrResult result = xrSyncActions(session, &actionsSyncInfo);
    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Cannot sync actions");
        return;
    }

    // [tdbe] Head action poses
    eyePoseMatrices[EYE_LEFT] = XrInputPose_to_glm(projection_views[EYE_LEFT].pose);
    eyePoses[EYE_LEFT] = OpenXRPose_to_XrInputPose(projection_views[EYE_LEFT].pose);
    eyePoseMatrices[EYE_RIGHT] = XrInputPose_to_glm(projection_views[EYE_RIGHT].pose);
    eyePoses[EYE_RIGHT] = OpenXRPose_to_XrInputPose(projection_views[EYE_RIGHT].pose);
    // [tdbe] TODO: figure out what/how head poses/joints work in openxr https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html
    headPose = {
        .orientation = slerp(eyePoses[EYE_LEFT].orientation, eyePoses[EYE_RIGHT].orientation, .5),
        .position = slerp(eyePoses[EYE_LEFT].position, eyePoses[EYE_RIGHT].position, .5)
    };
    headPoseMatrix = XrInputPose_to_glm(headPose);

    // [tdbe] Headset State. Use to detect status / user proximity / user presence / user engagement https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#session-lifecycle
    headsetActivityState = session_state;

    for (int i = 0u; i < HAND_COUNT; i++) {

        // Aim Pose
        XrActionStatePose aimPoseState = get_action_pose_state(input_state.aimPoseAction, i);
        if (aimPoseState.isActive)
        {
            XrSpaceLocation spaceLocation{ .type = XR_TYPE_SPACE_LOCATION };
            result = xrLocateSpace(input_state.aimHandSpace[i], play_space, frame_state.predictedDisplayTime, &spaceLocation);

            // Check that the position and orientation are valid and tracked
            constexpr XrSpaceLocationFlags checkFlags =
                XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT |
                XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;
            if ((spaceLocation.locationFlags & checkFlags) == checkFlags)
            {
                controllerAimPoseMatrices[i] = XrInputPose_to_glm(spaceLocation.pose);
                controllerAimPoses[i] = OpenXRPose_to_XrInputPose(spaceLocation.pose);
            }
        }

        // Grip Pose
        XrActionStatePose gripPoseState = get_action_pose_state(input_state.gripPoseAction, i);
        if (gripPoseState.isActive)
        {
            XrSpaceLocation spaceLocation{ .type = XR_TYPE_SPACE_LOCATION };
            result = xrLocateSpace(input_state.gripHandSpace[i], play_space, frame_state.predictedDisplayTime, &spaceLocation);

            // Check that the position and orientation are valid and tracked
            constexpr XrSpaceLocationFlags checkFlags =
                XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT |
                XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;
            if ((spaceLocation.locationFlags & checkFlags) == checkFlags)
            {
                controllerGripPoseMatrices[i] = XrInputPose_to_glm(spaceLocation.pose);
                controllerGripPoses[i] = OpenXRPose_to_XrInputPose(spaceLocation.pose);
            }
        }

        // Grab State
        {
            XrActionStateFloat _grabState = get_action_float_state(input_state.grabAction, i);
            grabState[i] = _grabState;
        }

        // Thumbstick State
        {
            XrActionStateVector2f _thumbStickValueState = get_action_vector2f_State(input_state.thumbstickValueAction, i);
            thumbStickValueState[i] = _thumbStickValueState;

            XrActionStateBoolean _thumbStickClickState = get_action_boolean_state(input_state.thumbstickClickAction, i);
            thumbStickClickState[i] = _thumbStickClickState;

            XrActionStateBoolean _thumbStickTouchState = get_action_boolean_state(input_state.thumbstickTouchAction, i);
            thumbStickTouchState[i] = _thumbStickTouchState;
        }

        // Triggers State
        {
            XrActionStateFloat _triggerValueState = get_action_float_state(input_state.triggerValueAction, i);
            triggerValueState[i] = _triggerValueState;

            XrActionStateBoolean _triggerTouchState = get_action_boolean_state(input_state.triggerTouchAction, i);
            triggerTouchState[i] = _triggerTouchState;
        }
    }

    // State (Buttons).
    for (auto& mb : buttonsState)
    {
        mb.click.state = get_action_boolean_state(mb.click.action, mb.hand);
        if (mb.touch.active) mb.touch.state = get_action_boolean_state(mb.touch.action, mb.hand);
    }
}


void OpenXRContext::apply_haptics(uint8_t controller, float amplitude, float duration)
{
    XrHapticVibration vibration = { XR_TYPE_HAPTIC_VIBRATION };
    vibration.amplitude = amplitude;
    vibration.duration = duration * 1e9; // Convert duration to nanoseconds
    vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

    XrHapticActionInfo hapticActionInfo = { XR_TYPE_HAPTIC_ACTION_INFO };
    hapticActionInfo.action = input_state.vibrateAction;
    hapticActionInfo.subactionPath = input_state.handSubactionPath[controller];

    XrResult result = xrApplyHapticFeedback(session, &hapticActionInfo, reinterpret_cast<XrHapticBaseHeader*>(&vibration));
    if (result != XR_SUCCESS) {
        spdlog::error("Vibration Error: OPENXR Error {}", static_cast<int>(result));
    }
}

void OpenXRContext::stop_haptics(uint8_t controller)
{
    XrHapticActionInfo hapticActionInfo = { XR_TYPE_HAPTIC_ACTION_INFO };
    hapticActionInfo.action = input_state.vibrateAction;
    hapticActionInfo.subactionPath = input_state.handSubactionPath[controller];

    xrStopHapticFeedback(session, &hapticActionInfo);
}

void OpenXRContext::init_frame()
{
    XrResult result;

    // Poll OpenXR events
    XrEventDataBuffer buffer{ XR_TYPE_EVENT_DATA_BUFFER };
    while (xrPollEvent(instance, &buffer) == XR_SUCCESS)
    {
        switch (buffer.type)
        {
        case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
            /*exitRequested = true;
            return BeginFrameResult::SkipFully;*/
            break;
        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
        {
            XrEventDataSessionStateChanged* event = reinterpret_cast<XrEventDataSessionStateChanged*>(&buffer);
            session_state = event->state;

            if (event->state == XR_SESSION_STATE_READY)
            {
                if (!begin_session())
                {
                    return;
                }
            }
            else if (event->state == XR_SESSION_STATE_STOPPING)
            {
                if (!end_session())
                {
                    return;
                }
            }
            else if (event->state == XR_SESSION_STATE_LOSS_PENDING || event->state == XR_SESSION_STATE_EXITING)
            {
                /*exitRequested = true;
                return BeginFrameResult::SkipFully;*/
            }

            break;
        }
        }

        buffer.type = XR_TYPE_EVENT_DATA_BUFFER;
    }

    // Wait for new frame
    frame_state.type = XR_TYPE_FRAME_STATE;

    XrFrameWaitInfo frameWaitInfo = {
      .type = XR_TYPE_FRAME_WAIT_INFO,
    };

    result = xrWaitFrame(session, &frameWaitInfo, &frame_state);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::trace("xrWaitFrame() was not successful");
        return;
    }

    XrFrameBeginInfo frameBeginInfo = {
      .type = XR_TYPE_FRAME_BEGIN_INFO,
    };

    result = xrBeginFrame(session, &frameBeginInfo);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::trace("Failed to begin frame!");
        return;
    }
}

void OpenXRContext::acquire_swapchain(int swapchain_index)
{
    XrResult result;

    XrSwapchainImageAcquireInfo acquire_info = {
        .type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO,
    };

    result = xrAcquireSwapchainImage(swapchains[swapchain_index].swapchain, &acquire_info, &swapchains[swapchain_index].image_index);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Failed to acquire swapchain image!");
        return;
    }

    XrSwapchainImageWaitInfo wait_info = {
      .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
      .timeout = INT64_MAX,
    };

    result = xrWaitSwapchainImage(swapchains[swapchain_index].swapchain, &wait_info);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Failed to wait for swapchain image!");
        return;
    }
}

void OpenXRContext::release_swapchain(int swapchain_index)
{
    XrResult result;

    XrSwapchainImageReleaseInfo info = {
    .type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
    };

    result = xrReleaseSwapchainImage(swapchains[swapchain_index].swapchain, &info);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Failed to release swapchain image!");
        return;
    }
}

void OpenXRContext::end_frame()
{
    XrResult result;

    XrCompositionLayerProjection projection_layer = {
            .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
            .next = NULL,
            .layerFlags = 0,
            .space = play_space,
            .viewCount = view_count,
            .views = projection_views.data(),
    };

    const XrCompositionLayerBaseHeader* submittedLayers[1] = {
            (const XrCompositionLayerBaseHeader* const)&projection_layer };

    XrFrameEndInfo frameEndInfo = {
        .type = XR_TYPE_FRAME_END_INFO,
        .displayTime = frame_state.predictedDisplayTime,
        .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
        .layerCount = 1,
        .layers = submittedLayers,
    };

    result = xrEndFrame(session, &frameEndInfo);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::trace("Failed to end frame!");
        return;
    }
}

uint32_t OpenXRContext::get_num_images_per_swapchain()
{
    return swapchains[0].images.size();
}

void OpenXRContext::update()
{
    XrViewState viewState{ XR_TYPE_VIEW_STATE };
    uint32_t viewCapacityInput = (uint32_t)views.size();

    XrViewLocateInfo viewLocateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
    viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    viewLocateInfo.displayTime = frame_state.predictedDisplayTime;
    viewLocateInfo.space = play_space;
    XrResult result = xrLocateViews(session, &viewLocateInfo, &viewState, viewCapacityInput, &viewCapacityInput, views.data());

    //Set the projection view to the pose and FOV for each eye
    for (uint16_t i = 0; i < views.size(); i++) {
        projection_views[i].pose = views[i].pose;
        projection_views[i].fov = views[i].fov;

        per_view_data[i].position = glm::vec3(views[i].pose.position.x, views[i].pose.position.y, views[i].pose.position.z);

        if (root_transform) {
            const glm::mat4 root_model = root_transform->get_model();
            per_view_data[i].position = root_model * glm::vec4(per_view_data[i].position, 1.0);
            per_view_data[i].view_matrix = glm::inverse(root_model * XrInputPose_to_glm(views[i].pose));
        }
        else {
            per_view_data[i].view_matrix = glm::inverse(XrInputPose_to_glm(views[i].pose));
        }

        per_view_data[i].projection_matrix = OpenXRProjection_to_glm(views[i].fov, z_near, z_far);

        per_view_data[i].view_projection_matrix = per_view_data[i].projection_matrix * per_view_data[i].view_matrix;
    }

    if ((viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 ||
        (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) {
        return;  // There is no valid tracking poses for the views.
    }
}

bool OpenXRContext::create_action(XrActionSet actionSet,
    XrPath* paths,
    uint32_t num_paths,
    const std::string& actionName,
    const std::string& localizedActionName,
    XrActionType type,
    XrAction& action)
{
    XrActionCreateInfo actionCreateInfo{
        .type = XR_TYPE_ACTION_CREATE_INFO,
        .actionType = type,
        .countSubactionPaths = num_paths,
        .subactionPaths = paths
    };

    //strcpy_s(actionCreateInfo.actionName, actionName.c_str());
    //strcpy_s(actionCreateInfo.localizedActionName, localizedActionName.c_str());
    memcpy(actionCreateInfo.actionName, actionName.data(), actionName.length() + 1u);
    memcpy(actionCreateInfo.localizedActionName, localizedActionName.data(), localizedActionName.length() + 1u);

    XrResult result = xrCreateAction(actionSet, &actionCreateInfo, &action);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Can't create XrAction");
        return false;
    }

    return true;
}

XrActionStatePose OpenXRContext::get_action_pose_state(XrAction targetAction, uint8_t controller)
{
    XrPath path = XR_NULL_PATH;// Wildcard for all
    if (controller != HAND_COUNT) {
        path = input_state.handSubactionPath[controller];
    }
    XrActionStateGetInfo getInfo = { .type = XR_TYPE_ACTION_STATE_GET_INFO,
                                    .action = targetAction,
                                    .subactionPath = path };

    XrActionStatePose poseState{ .type = XR_TYPE_ACTION_STATE_POSE };
    XrResult result = xrGetActionStatePose(session, &getInfo, &poseState);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Cannot get action pose state");
    }

    return poseState;
}

XrActionStateBoolean OpenXRContext::get_action_boolean_state(XrAction targetAction, uint8_t controller)
{
    XrPath path = XR_NULL_PATH;// Wildcard for all
    if (controller != HAND_COUNT) {
        path = input_state.handSubactionPath[controller];
    }
    XrActionStateGetInfo getInfo = { .type = XR_TYPE_ACTION_STATE_GET_INFO,
                                    .action = targetAction,
                                    .subactionPath = path };

    XrActionStateBoolean poseState{ .type = XR_TYPE_ACTION_STATE_BOOLEAN };
    XrResult result = xrGetActionStateBoolean(session, &getInfo, &poseState);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Cannot get action boolean state");
    }

    return poseState;
}

XrActionStateFloat OpenXRContext::get_action_float_state(XrAction targetAction, uint8_t controller)
{
    XrPath path = XR_NULL_PATH;// Wildcard for all
    if (controller != HAND_COUNT) {
        path = input_state.handSubactionPath[controller];
    }
    XrActionStateGetInfo getInfo = { .type = XR_TYPE_ACTION_STATE_GET_INFO,
                                    .action = targetAction,
                                    .subactionPath = path };

    XrActionStateFloat poseState{ .type = XR_TYPE_ACTION_STATE_FLOAT };
    XrResult result = xrGetActionStateFloat(session, &getInfo, &poseState);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Cannot get action float state");
    }

    return poseState;
}

XrActionStateVector2f OpenXRContext::get_action_vector2f_State(XrAction targetAction, uint8_t controller)
{
    XrPath path = XR_NULL_PATH;// Wildcard for all
    if (controller != HAND_COUNT) {
        path = input_state.handSubactionPath[controller];
    }
    XrActionStateGetInfo getInfo = { .type = XR_TYPE_ACTION_STATE_GET_INFO,
                                    .action = targetAction,
                                    .subactionPath = path };

    XrActionStateVector2f poseState{ .type = XR_TYPE_ACTION_STATE_VECTOR2F };
    XrResult result = xrGetActionStateVector2f(session, &getInfo, &poseState);

    if (!XR_SUCCEEDED(result))
    {
        spdlog::error("Cannot get action vector2f state");
    }

    return poseState;
}

/*
    
    HELPER FUNCTIONS
    
*/

// From: https://github.com/jherico/OpenXR-Samples/blob/master/src/examples/sdl2_gl_single_file_example.cpp
inline glm::mat4x4 OpenXRProjection_to_glm(const XrFovf& fov, float nearZ, float farZ) {
    const auto& tanAngleRight = tanf(fov.angleRight);
    const auto& tanAngleLeft = tanf(fov.angleLeft);
    const auto& tanAngleUp = tanf(fov.angleUp);
    const auto& tanAngleDown = tanf(fov.angleDown);

    const float tanAngleWidth = tanAngleRight - tanAngleLeft;
    const float tanAngleHeight = (tanAngleUp - tanAngleDown); // For vulkan projection
    const float offsetZ = 0;

    glm::mat4 resultm{};
    float* result = &resultm[0][0];
    // normal projection
    result[0] = 2 / tanAngleWidth;
    result[4] = 0;
    result[8] = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
    result[12] = 0;

    result[1] = 0;
    result[5] = 2 / tanAngleHeight;
    result[9] = (tanAngleUp + tanAngleDown) / tanAngleHeight;
    result[13] = 0;

    result[2] = 0;
    result[6] = 0;
    result[10] = -(farZ + offsetZ) / (farZ - nearZ);
    result[14] = -(farZ * (nearZ + offsetZ)) / (farZ - nearZ);

    result[3] = 0;
    result[7] = 0;
    result[11] = -1;
    result[15] = 0;

    return resultm;
}

inline glm::mat4x4 XrInputPose_to_glm(const XrPosef& p) {
    glm::mat4 translation = glm::translate(glm::mat4{ 1.f }, glm::vec3(p.position.x, p.position.y, p.position.z));
    glm::mat4 orientation = glm::mat4_cast(glm::quat(p.orientation.x, p.orientation.y, p.orientation.z, p.orientation.w));
    return translation * orientation;
}

inline XrInputPose OpenXRPose_to_XrInputPose(const XrPosef& xrPosef) {
    
    return XrInputPose{
        .orientation = glm::quat(xrPosef.orientation.x, xrPosef.orientation.y, xrPosef.orientation.z, xrPosef.orientation.w),
        .position = glm::vec3(xrPosef.position.x, xrPosef.position.y, xrPosef.position.z)
    };
}

glm::quat slerp(const glm::quat& start, const glm::quat& end, float percent)
{
    float cosTheta = glm::dot(start, end);
    glm::quat temp(end);


    if (cosTheta < 0.0f) {
        cosTheta *= -1.0f;
        temp = temp * -1.0f;
    }

    float theta = glm::acos(cosTheta);
    float sinThetaDenom = 1.0f / glm::sin(theta);

    glm::quat res = (
        ((glm::quat)(start * glm::sin(theta * (1.0f - percent)))) +
        ((glm::quat)(temp * glm::sin(percent * theta)))
        ) / sinThetaDenom;

    return res;
}

glm::vec3 slerp(const glm::vec3& start, const glm::vec3& end, float percent)
{
    // Dot product - the cosine of the angle between 2 vectors.
    float dot = glm::dot(start, end);

    // Clamp it to be in the range of Acos()
    // This may be unnecessary, but floating point
    // precision can be a fickle mistress.
    dot = glm::clamp(dot, -1.0f, 1.0f);

    // Acos(dot) returns the angle between start and end,
    // And multiplying that by percent returns the angle between
    // start and the final result.
    float theta = glm::acos(dot) * percent;
    glm::vec3 startDotted = start * dot;

    glm::vec3 relativeVec = {};
    relativeVec = end - startDotted;

    relativeVec = glm::normalize(relativeVec);

    // Orthonormal basis
    // The final result.
    return ((start * glm::cos(theta)) + (relativeVec * glm::sin(theta)));
}

#endif
