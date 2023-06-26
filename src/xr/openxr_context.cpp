#include "openxr_context.h"

#ifdef XR_SUPPORT

#include <iostream>
#include <cstdarg>

// we need an identity pose for creating spaces without offsets
static XrPosef identity_pose = { .orientation = {.x = 0, .y = 0, .z = 0, .w = 1.0},
                                 .position = {.x = 0, .y = 0, .z = 0} };

// Helper functions for pose to GLM
glm::mat4x4 parse_OpenXR_projection_to_glm(const XrFovf& fov, float nearZ = 0.01f, float farZ = 10000.0f);
glm::mat4x4 parse_OpenXR_pose_to_glm(const XrPosef& p);

int OpenXRContext::initialize(WebGPUContext* webgpu_context)
{
    XrResult result;

    uint32_t blend_modes_count = 0;
    result = xrEnumerateEnvironmentBlendModes(instance, system_id, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &blend_modes_count, NULL);
    if (!xr_result(NULL, result, "Failed to enumerate number of blend modes"))
        return 1;

    std::vector<XrEnvironmentBlendMode> blendModes(blend_modes_count);
    result = xrEnumerateEnvironmentBlendModes(instance, system_id, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, blend_modes_count, &blend_modes_count, blendModes.data());
    if (!xr_result(NULL, result, "Failed to enumerate blend modes"))
        return 1;

    XrGraphicsRequirementsVulkanKHR vulkan_reqs = { .type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR };

    PFN_xrGetVulkanGraphicsRequirements2KHR pfnGetVulkanGraphicsRequirements2KHR = NULL;
    {
        result = xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsRequirements2KHR", (PFN_xrVoidFunction*)&pfnGetVulkanGraphicsRequirements2KHR);
        if (!xr_result(instance, result, "Failed to get Vulkan graphics requirements function!"))
            return 1;
    }

    pfnGetVulkanGraphicsRequirements2KHR(instance, system_id, &vulkan_reqs);

    check_vulkan_version(&vulkan_reqs);

    view_count = 0;
    result = xrEnumerateViewConfigurationViews(instance, system_id, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &view_count, NULL);
    if (!xr_result(instance, result, "Failed to get view configuration count"))
        return 1;

    views.resize(view_count, { XR_TYPE_VIEW });
    viewconfig_views.resize(view_count, { XR_TYPE_VIEW_CONFIGURATION_VIEW, nullptr });
    projection_views.resize(view_count);
    per_view_data.resize(view_count);

    result = xrEnumerateViewConfigurationViews(instance, system_id, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, view_count, &view_count, viewconfig_views.data());
    if (!xr_result(instance, result, "Failed to enumerate view configuration views!"))
        return 1;

    print_viewconfig_view_info();

    dawnxr::GraphicsBindingDawn binding = { .device = webgpu_context->device };

    XrSessionCreateInfo xrCreateInfo = { .type = XR_TYPE_SESSION_CREATE_INFO, .next = &binding, .systemId = system_id };
    dawnxr::createSession(instance, &xrCreateInfo, &session);

    uint32_t swapchain_format_count;
    result = dawnxr::enumerateSwapchainFormats(session, 0, &swapchain_format_count, NULL);
    if (!xr_result(instance, result, "Failed to get number of supported swapchain formats"))
        return 1;

    printf("Runtime supports %d swapchain formats\n", swapchain_format_count);
    swapchain_formats.resize(swapchain_format_count);
    result = dawnxr::enumerateSwapchainFormats(session, swapchain_format_count, &swapchain_format_count, swapchain_formats.data());
    if (!xr_result(instance, result, "Failed to enumerate swapchain formats"))
        return 1;

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
        if (!xr_result(instance, result, "Failed to enumerate swapchains"))
            return 1;

        // these are wrappers for the actual OpenGL texture id
        swapchains[i].images.resize(swapchain_length);
        result = dawnxr::enumerateSwapchainImages(swapchains[i].swapchain, swapchain_length, &swapchain_length, (XrSwapchainImageBaseHeader*)swapchains[i].images.data());
        if (!xr_result(instance, result, "Failed to enumerate swapchain images"))
            return 1;
    }


    XrReferenceSpaceType play_space_type = XR_REFERENCE_SPACE_TYPE_STAGE;
    // We could check if our ref space type is supported, but next call will error anyway if not
    print_reference_spaces();

    XrReferenceSpaceCreateInfo play_space_create_info = { .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
                                                         .next = NULL,
                                                         .referenceSpaceType = play_space_type, //play_space_type,
                                                         .poseInReferenceSpace = identity_pose };

    result = xrCreateReferenceSpace(session, &play_space_create_info, &play_space);
    if (!xr_result(instance, result, "Failed to create play space!"))
        return 1;

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

    XrSessionBeginInfo session_begin_info = { .type = XR_TYPE_SESSION_BEGIN_INFO, .next = NULL, .primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO };
    result = xrBeginSession(session, &session_begin_info);
    if (!xr_result(instance, result, "Failed to begin session!"))
        return 1;

    init_actions();

    initialized = true;

    return 0;
}

void OpenXRContext::clean()
{
    if (!initialized) {
        return;
    }

    for (int i = 0; i < view_count; ++i) {
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

    if (!xr_result(NULL, result, "Could not initilize OpenXR: Failed to enumerate number of extension properties"))
        return false;

    std::vector<XrExtensionProperties> extensionProperties(extension_count, { XR_TYPE_EXTENSION_PROPERTIES, nullptr });
    result = xrEnumerateInstanceExtensionProperties(NULL, extension_count, &extension_count, extensionProperties.data());
    if (!xr_result(NULL, result, "Failed to enumerate extension properties"))
        return false;

    uint32_t layer_count = 0;
    result = xrEnumerateApiLayerProperties(0, &layer_count, NULL);

    if (!xr_result(NULL, result, "Failed to enumerate layer properties"))
        return false;

    std::vector<XrApiLayerProperties> layerProperties(layer_count, { XR_TYPE_API_LAYER_PROPERTIES, nullptr });
    result = xrEnumerateApiLayerProperties(layer_count, &layer_count, layerProperties.data());
    if (!xr_result(NULL, result, "Failed to enumerate extension properties"))
        return false;

    bool vulkan_ext = false;

    std::cout << "OpenXR extensions:" << std::endl;
    for (const auto &ext : extensionProperties) {
        std::cout << "\t" << ext.extensionName << " " << ext.extensionVersion << std::endl;

        if (strcmp("XR_KHR_vulkan_enable2", ext.extensionName) == 0) {
            vulkan_ext = true;
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

    // A graphics extension is required to draw anything in VR
    if (!vulkan_ext) {
        std::cout << "Runtime does not support Vulkan extension!" << std::endl;
        return false;
    }

    // --- Create XrInstance
    uint32_t enabled_ext_count = 1;
    const char* enabled_exts[3] = { "XR_KHR_vulkan_enable2" };

    if (hand_tracking.supported) {
        enabled_exts[enabled_ext_count++] = XR_EXT_HAND_TRACKING_EXTENSION_NAME;
    }
    //if (cylinder.supported) {
    //    enabled_exts[enabled_ext_count++] = XR_KHR_COMPOSITION_LAYER_CYLINDER_EXTENSION_NAME;
    //}

    XrInstanceCreateInfo instance_create_info = {
        XR_TYPE_INSTANCE_CREATE_INFO,
        nullptr,
        0,
        {
            "wgpuEngine", 1,
            "Custom", 0,
            XR_CURRENT_API_VERSION,
        },
        0,
        NULL,
        enabled_ext_count,
        enabled_exts
    };

    result = xrCreateInstance(&instance_create_info, &instance);
    if (!xr_result(NULL, result, "Failed to create XR instance."))
        return false;

    XrInstanceProperties instance_props = {
        .type = XR_TYPE_INSTANCE_PROPERTIES,
        .next = NULL,
    };

    result = xrGetInstanceProperties(instance, &instance_props);
    if (!xr_result(NULL, result, "Failed to get instance info"))
        return false;

    std::cout
        << "Runtime Name: " << instance_props.runtimeName << "\n"
        << "Runtime Version: " << XR_VERSION_MAJOR(instance_props.runtimeVersion) << "." << XR_VERSION_MINOR(instance_props.runtimeVersion) << "." << XR_VERSION_PATCH(instance_props.runtimeVersion) << std::endl;

    XrSystemGetInfo system_get_info = { .type = XR_TYPE_SYSTEM_GET_INFO, .next = NULL, .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY };
    result = xrGetSystem(instance, &system_get_info, &system_id);

    if (!xr_result(instance, result, "Failed to get system for HMD form factor."))
        return false;

    return true;
}

bool OpenXRContext::xr_result(XrInstance xrInstance, XrResult result, const char* format, ...)
{
    if (XR_SUCCEEDED(result))
        return true;

    if (!xrInstance)
        std::cout << format << std::endl;
        return false;

    char resultString[XR_MAX_RESULT_STRING_SIZE];
    xrResultToString(xrInstance, result, resultString);

    size_t len1 = strlen(format);
    size_t len2 = strlen(resultString) + 1;
    char* formatRes = new  char[len1 + len2 + 4]; // + " []\n"
    sprintf(formatRes, "%s [%s]\n", format, resultString);

    va_list args;
    va_start(args, format);
    vprintf(formatRes, args);
    va_end(args);

    delete[] formatRes;
    return false;
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

void OpenXRContext::print_reference_spaces()
{
    XrResult result;

    uint32_t ref_space_count;
    result = xrEnumerateReferenceSpaces(session, 0, &ref_space_count, NULL);
    if (!xr_result(instance, result, "Getting number of reference spaces failed!"))
        return;

    std::vector<XrReferenceSpaceType> ref_spaces(ref_space_count);
    result = xrEnumerateReferenceSpaces(session, ref_space_count, &ref_space_count, ref_spaces.data());
    if (!xr_result(instance, result, "Enumerating reference spaces failed!"))
        return;

    printf("Runtime supports %d reference spaces:\n", ref_space_count);
    for (uint32_t i = 0; i < ref_space_count; i++) {
        if (ref_spaces[i] == XR_REFERENCE_SPACE_TYPE_LOCAL) {
            printf("\tXR_REFERENCE_SPACE_TYPE_LOCAL\n");
        }
        else if (ref_spaces[i] == XR_REFERENCE_SPACE_TYPE_STAGE) {
            printf("\tXR_REFERENCE_SPACE_TYPE_STAGE\n");
        }
        else if (ref_spaces[i] == XR_REFERENCE_SPACE_TYPE_VIEW) {
            printf("\tXR_REFERENCE_SPACE_TYPE_VIEW\n");
        }
        else {
            printf("\tOther (extension?) refspace %u\\n", ref_spaces[i]);
        }
    }
}

void OpenXRContext::init_actions()
{
    // Create an action set.
    {
        XrActionSetCreateInfo actionSetInfo{
            .type = XR_TYPE_ACTION_SET_CREATE_INFO,
            .actionSetName = "gameplay",
            .localizedActionSetName = "Gameplay",
            .priority = 0
        };
        xrCreateActionSet(instance, &actionSetInfo, &input_state.actionSet);
    }

    // Get the XrPath for the left and right hands - we will use them as subaction paths.
    xrStringToPath(instance, "/user/hand/left", &input_state.handSubactionPath[HAND_LEFT]);
    xrStringToPath(instance, "/user/hand/right", &input_state.handSubactionPath[HAND_RIGHT]);

    // Create actions.
    {
        // Create an input action for grabbing objects with the left and right hands.
        create_action(input_state.actionSet, input_state.handSubactionPath, HAND_COUNT,
            "grab_object", "Grab Object", XR_ACTION_TYPE_FLOAT_INPUT, input_state.grabAction);

        // Create an input action getting the left and right hand poses.
        create_action(input_state.actionSet, input_state.handSubactionPath, HAND_COUNT,
            "hand_pose", "Hand Pose", XR_ACTION_TYPE_POSE_INPUT, input_state.poseAction);

        // Create output actions for vibrating the left and right controller.
        create_action(input_state.actionSet, input_state.handSubactionPath, HAND_COUNT,
            "vibrate_hand", "Vibrate Hand", XR_ACTION_TYPE_VIBRATION_OUTPUT, input_state.vibrateAction);

        // Create input actions for quitting the session using the left and right controller.
        // Since it doesn't matter which hand did this, we do not specify subaction paths for it.
        // We will just suggest bindings for both hands, where possible.
        create_action(input_state.actionSet, nullptr, 0,
            "quit_session", "Quit Session", XR_ACTION_TYPE_BOOLEAN_INPUT, input_state.quitAction);
    }

    XrPath selectPath[HAND_COUNT];
    XrPath squeezeValuePath[HAND_COUNT];
    XrPath squeezeForcePath[HAND_COUNT];
    XrPath squeezeClickPath[HAND_COUNT];
    XrPath posePath[HAND_COUNT];
    XrPath hapticPath[HAND_COUNT];
    XrPath menuClickPath[HAND_COUNT];
    XrPath bClickPath[HAND_COUNT];
    XrPath triggerValuePath[HAND_COUNT];
    (xrStringToPath(instance, "/user/hand/left/input/select/click", &selectPath[HAND_LEFT]));
    (xrStringToPath(instance, "/user/hand/right/input/select/click",
        &selectPath[HAND_RIGHT]));
    (xrStringToPath(instance, "/user/hand/left/input/squeeze/value",
        &squeezeValuePath[HAND_LEFT]));
    (xrStringToPath(instance, "/user/hand/right/input/squeeze/value",
        &squeezeValuePath[HAND_RIGHT]));
    (xrStringToPath(instance, "/user/hand/left/input/squeeze/force",
        &squeezeForcePath[HAND_LEFT]));
    (xrStringToPath(instance, "/user/hand/right/input/squeeze/force",
        &squeezeForcePath[HAND_RIGHT]));
    (xrStringToPath(instance, "/user/hand/left/input/squeeze/click",
        &squeezeClickPath[HAND_LEFT]));
    (xrStringToPath(instance, "/user/hand/right/input/squeeze/click",
        &squeezeClickPath[HAND_RIGHT]));
    (xrStringToPath(instance, "/user/hand/left/input/grip/pose", &posePath[HAND_LEFT]));
    (xrStringToPath(instance, "/user/hand/right/input/grip/pose", &posePath[HAND_RIGHT]));
    (xrStringToPath(instance, "/user/hand/left/output/haptic", &hapticPath[HAND_LEFT]));
    (xrStringToPath(instance, "/user/hand/right/output/haptic", &hapticPath[HAND_RIGHT]));
    (xrStringToPath(instance, "/user/hand/left/input/menu/click", &menuClickPath[HAND_LEFT]));
    (xrStringToPath(instance, "/user/hand/right/input/menu/click",
        &menuClickPath[HAND_RIGHT]));
    (xrStringToPath(instance, "/user/hand/left/input/b/click", &bClickPath[HAND_LEFT]));
    (xrStringToPath(instance, "/user/hand/right/input/b/click", &bClickPath[HAND_RIGHT]));
    (xrStringToPath(instance, "/user/hand/left/input/trigger/value",
        &triggerValuePath[HAND_LEFT]));
    (xrStringToPath(instance, "/user/hand/right/input/trigger/value",
        &triggerValuePath[HAND_RIGHT]));

    // Suggest bindings for KHR Simple.
    {
        XrPath khrSimpleInteractionProfilePath;
        (
            xrStringToPath(instance, "/interaction_profiles/khr/simple_controller",
                &khrSimpleInteractionProfilePath));
        XrActionSuggestedBinding bindings[8] = {// Fall back to a click input for the grab action.
                {input_state.grabAction,    selectPath[HAND_LEFT]},
                {input_state.grabAction,    selectPath[HAND_RIGHT]},
                {input_state.poseAction,    posePath[HAND_LEFT]},
                {input_state.poseAction,    posePath[HAND_RIGHT]},
                {input_state.quitAction,    menuClickPath[HAND_LEFT]},
                {input_state.quitAction,    menuClickPath[HAND_RIGHT]},
                {input_state.vibrateAction, hapticPath[HAND_LEFT]},
                {input_state.vibrateAction, hapticPath[HAND_RIGHT]} };
        XrInteractionProfileSuggestedBinding suggestedBindings{
                XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
        suggestedBindings.interactionProfile = khrSimpleInteractionProfilePath;
        suggestedBindings.suggestedBindings = bindings;
        suggestedBindings.countSuggestedBindings = 8;
        (xrSuggestInteractionProfileBindings(instance, &suggestedBindings));
    }
    // Suggest bindings for the Oculus Touch.
    {
        XrPath oculusTouchInteractionProfilePath;
        (
            xrStringToPath(instance, "/interaction_profiles/oculus/touch_controller",
                &oculusTouchInteractionProfilePath));
        XrActionSuggestedBinding bindings[7] = { {input_state.grabAction,    squeezeValuePath[HAND_LEFT]},
                                                {input_state.grabAction,    squeezeValuePath[HAND_RIGHT]},
                                                {input_state.poseAction,    posePath[HAND_LEFT]},
                                                {input_state.poseAction,    posePath[HAND_RIGHT]},
                                                {input_state.quitAction,    menuClickPath[HAND_LEFT]},
                                                {input_state.vibrateAction, hapticPath[HAND_LEFT]},
                                                {input_state.vibrateAction, hapticPath[HAND_RIGHT]} };
        XrInteractionProfileSuggestedBinding suggestedBindings{
                XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
        suggestedBindings.interactionProfile = oculusTouchInteractionProfilePath;
        suggestedBindings.suggestedBindings = bindings;
        suggestedBindings.countSuggestedBindings = 7;
        (xrSuggestInteractionProfileBindings(instance, &suggestedBindings));
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
    for (size_t ci = 0u; ci < HAND_COUNT; ci++)
    {
        XrActionSpaceCreateInfo actionSpaceInfo{
            .type = XR_TYPE_ACTION_SPACE_CREATE_INFO,
            .action = input_state.poseAction,
            .subactionPath = input_state.handSubactionPath[ci]
        };
        actionSpaceInfo.poseInActionSpace.orientation = { .x = 0, .y = 0, .z = 0, .w = 1.0 };
        actionSpaceInfo.poseInActionSpace.position = { .x = 0, .y = 0, .z = 0 };

        XrResult result = xrCreateActionSpace(session, &actionSpaceInfo, &input_state.handSpace[ci]);
        if (!xr_result(instance, result, "Can't create controller  subspace for controller %d", ci))
            return;
    }

    // Attach the controller action set
    XrSessionActionSetsAttachInfo xrSessionAttachInfo{
        .type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO,
        .countActionSets = 1,
        .actionSets = &input_state.actionSet
    };
    xrAttachSessionActionSets(session, &xrSessionAttachInfo);
}

void OpenXRContext::sync()
{
    // Sync the actions 
    const XrActiveActionSet activeActionSet{ input_state.actionSet, XR_NULL_PATH };// Wildcard for all

    XrActionsSyncInfo actionsSyncInfo = { .type = XR_TYPE_ACTIONS_SYNC_INFO,
                                        .countActiveActionSets = 1u,
                                        .activeActionSets = &activeActionSet };

    XrResult result = xrSyncActions(session, &actionsSyncInfo);
    if (!xr_result(instance, result, "Cannot sync actions for XR controllers"))
        return;

    // Update the actions
    /*
    inputData.SizeVectors(ControllerEnum::COUNT, SideEnum::COUNT);

    // [tdbe] Head action poses
    inputData.eyePoseMatrixes[(int)SideEnum::LEFT] = util::poseToMatrix(eyePoses[(int)SideEnum::LEFT].pose);
    inputData.eyePoses[(int)SideEnum::LEFT] = util::xrPosefToGlmPosef(eyePoses[(int)SideEnum::LEFT].pose);
    inputData.eyePoseMatrixes[(int)SideEnum::RIGHT] = util::poseToMatrix(eyePoses[(int)SideEnum::RIGHT].pose);
    inputData.eyePoses[(int)SideEnum::RIGHT] = util::xrPosefToGlmPosef(eyePoses[(int)SideEnum::RIGHT].pose);
    // [tdbe] TODO: figure out what/how head poses/joints work in openxr https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html
    inputData.headPose = {
        .orientation = util::slerp(inputData.eyePoses[(int)SideEnum::LEFT].orientation, inputData.eyePoses[(int)SideEnum::RIGHT].orientation, .5),
        .position = util::slerp(inputData.eyePoses[(int)SideEnum::LEFT].position, inputData.eyePoses[(int)SideEnum::RIGHT].position, .5)
    };
    inputData.headPoseMatrix = util::poseToMatrix(inputData.headPose);

    // [tdbe] Headset State. Use to detect status / user proximity / user presence / user engagement https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#session-lifecycle
    inputData.headsetActivityState = sessionState;
    */

    /*XrSpace xrReferenceSpace;
    std::vector<XrView> eyePoses;
    XrSessionState sessionState;*/

    for (size_t i = 0u; i < HAND_COUNT; i++) {
        
        // Pose
        XrActionStatePose grabPoseState = get_action_pose_state(input_state.grabAction, i);
        if (grabPoseState.isActive)
        {
            XrSpaceLocation spaceLocation{ .type = XR_TYPE_SPACE_LOCATION };
            result = xrLocateSpace(input_state.handSpace[i], xrReferenceSpace, frame_state.predictedDisplayTime, &spaceLocation);
            
            // Check that the position and orientation are valid and tracked
            constexpr XrSpaceLocationFlags checkFlags =
                XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT |
                XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;
            if ((spaceLocation.locationFlags & checkFlags) == checkFlags)
            {
                // inputData.controllerAimPoseMatrixes.at(i) = util::poseToMatrix(spaceLocation.pose);
                // inputData.controllerAimPoses.at(i) = util::xrPosefToGlmPosef(spaceLocation.pose);
            }
        }

        /*
        // [tdbe] State (input value)
        XrActionStateFloat grabState = Input::GetActionFloatState(actionSetData.grabAction, ci);
        inputData.grabState.at(i) = grabState;

        // [tdbe] State
        XrActionStateVector2f thumbStickState = Input::GetActionVector2fState(actionSetData.thumbstickAction, ci);
        inputData.thumbStickState.at(i) = thumbStickState;

        // [tdbe] State
        XrActionStateBoolean menuClickState = Input::GetActionBooleanState(actionSetData.menuClickAction, ci);
        inputData.menuClickState.at(i) = menuClickState;

        // [tdbe] State
        XrActionStateBoolean selectClickState = Input::GetActionBooleanState(actionSetData.selectClickAction, ci);
        inputData.selectClickState.at(i) = selectClickState;
        */
    }
}

void OpenXRContext::init_frame()
{
    XrResult result;

    frame_state.type = XR_TYPE_FRAME_STATE;

    XrFrameWaitInfo frameWaitInfo = {
      .type = XR_TYPE_FRAME_WAIT_INFO,
    };

    result = xrWaitFrame(session, &frameWaitInfo, &frame_state);
    if (!xr_result(instance, result, "xrWaitFrame() was not successful, exiting..."))
        return;

    XrFrameBeginInfo frameBeginInfo = {
      .type = XR_TYPE_FRAME_BEGIN_INFO,
    };

    result = xrBeginFrame(session, &frameBeginInfo);
    if (!xr_result(instance, result, "failed to begin frame!"))
        return;

    XrViewState viewState{ XR_TYPE_VIEW_STATE };
    uint32_t viewCapacityInput = (uint32_t)views.size();

    XrViewLocateInfo viewLocateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
    viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    viewLocateInfo.displayTime = frame_state.predictedDisplayTime;
    viewLocateInfo.space = play_space;
    result = xrLocateViews(session, &viewLocateInfo, &viewState, viewCapacityInput, &viewCapacityInput, views.data());

    //Set the projection view to the pose and FOV for each eye
    for (uint16_t i = 0; i < views.size(); i++) {
        projection_views[i].pose = views[i].pose;
        projection_views[i].fov = views[i].fov;

        per_view_data[i].position = glm::vec3(views[i].pose.position.x, views[i].pose.position.y, views[i].pose.position.z);
        per_view_data[i].view_matrix = glm::inverse(parse_OpenXR_pose_to_glm(views[i].pose));
        per_view_data[i].projection_matrix = parse_OpenXR_projection_to_glm(views[i].fov);

        per_view_data[i].view_projection_matrix = per_view_data[i].projection_matrix * per_view_data[i].view_matrix;
    }

    if ((viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 ||
        (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) {
        return;  // There is no valid tracking poses for the views.
    }
}

void OpenXRContext::acquire_swapchain(int swapchain_index)
{
    XrResult result;

    XrSwapchainImageAcquireInfo acquire_info = {
        .type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO,
    };

    result = xrAcquireSwapchainImage(swapchains[swapchain_index].swapchain, &acquire_info, &swapchains[swapchain_index].image_index);
    if (!xr_result(instance, result, "failed to acquire swapchain image!"))
        return;

    XrSwapchainImageWaitInfo wait_info = {
      .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
      .timeout = INT64_MAX,
    };

    result = xrWaitSwapchainImage(swapchains[swapchain_index].swapchain, &wait_info);
    if (!xr_result(instance, result, "failed to wait for swapchain image!"))
        return;
}

void OpenXRContext::release_swapchain(int swapchain_index)
{
    XrResult result;

    XrSwapchainImageReleaseInfo info = {
    .type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
    };

    result = xrReleaseSwapchainImage(swapchains[swapchain_index].swapchain, &info);
    if (!xr_result(instance, result, "failed to release swapchain image!"))
        return;
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
    if (!xr_result(instance, result, "failed to end frame!"))
        return;
}

bool OpenXRContext::create_action(XrActionSet actionSet,
    XrPath* paths,
    uint32_t num_paths,
    const std::string& actionName,
    const std::string& localizedActionName,
    XrActionType type,
    XrAction& action)
{
    XrActionCreateInfo actionCreateInfo{ XR_TYPE_ACTION_CREATE_INFO };
    actionCreateInfo.actionType = type;
    actionCreateInfo.countSubactionPaths = num_paths;
    actionCreateInfo.subactionPaths = paths;

    //strcpy_s(actionCreateInfo.actionName, actionName.c_str());
    //strcpy_s(actionCreateInfo.localizedActionName, localizedActionName.c_str());
    memcpy(actionCreateInfo.actionName, actionName.data(), actionName.length() + 1u);
    memcpy(actionCreateInfo.localizedActionName, localizedActionName.data(), localizedActionName.length() + 1u);

    XrResult result = xrCreateAction(actionSet, &actionCreateInfo, &action);
    if (!xr_result(instance, result, "Can't create XrAction"))
        return false;

    return true;
}

XrActionStatePose OpenXRContext::get_action_pose_state(XrAction targetAction, int controller)
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
    if (!xr_result(instance, result, "Cannot get action pose state" )) {
        
    }
    return poseState;
}

// HELPER FUNCTIONS
// From: https://github.com/jherico/OpenXR-Samples/blob/master/src/examples/sdl2_gl_single_file_example.cpp
inline glm::mat4x4 parse_OpenXR_projection_to_glm(const XrFovf& fov, float nearZ, float farZ) {
    const auto& tanAngleRight = tanf(fov.angleRight);
    const auto& tanAngleLeft = tanf(fov.angleLeft);
    const auto& tanAngleUp = tanf(fov.angleUp);
    const auto& tanAngleDown = tanf(fov.angleDown);

    const float tanAngleWidth = tanAngleRight - tanAngleLeft;
    const float tanAngleHeight = (tanAngleDown - tanAngleUp); // For vulkan projection
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

inline glm::mat4x4 parse_OpenXR_pose_to_glm(const XrPosef& p) {
    glm::mat4 orientation = glm::mat4_cast(glm::quat(p.orientation.w, p.orientation.x, p.orientation.y, p.orientation.z));
    glm::mat4 translation = glm::translate(glm::mat4{ 1 }, glm::vec3(p.position.x, p.position.y, p.position.z));
    return translation * orientation;
}

#endif