#include "openxr_context.h"

#include <iostream>
#include <cstdarg>

int OpenXRContext::initialize()
{
    XrResult result;

    uint32_t ext_count = 0;
    result = xrEnumerateInstanceExtensionProperties(NULL, 0, &ext_count, NULL);

    if (!xr_result(NULL, result, "Failed to enumerate number of extension properties"))
        return 1;

    std::vector<XrExtensionProperties> extensionProperties(ext_count, { XR_TYPE_EXTENSION_PROPERTIES, nullptr });
    result = xrEnumerateInstanceExtensionProperties(NULL, ext_count, &ext_count, extensionProperties.data());
    if (!xr_result(NULL, result, "Failed to enumerate extension properties"))
        return 1;

    uint32_t layer_count = 0;
    result = xrEnumerateApiLayerProperties(0, &layer_count, NULL);

    if (!xr_result(NULL, result, "Failed to enumerate layer properties"))
        return 1;

    std::vector<XrApiLayerProperties> layerProperties(layer_count, { XR_TYPE_API_LAYER_PROPERTIES, nullptr });
    result = xrEnumerateApiLayerProperties(layer_count, &layer_count, layerProperties.data());
    if (!xr_result(NULL, result, "Failed to enumerate extension properties"))
        return 1;

    bool vulkan_ext = false;

    std::cout << "OpenXR extensions:" << std::endl;
    for (auto ext : extensionProperties) {
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
        return 1;
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

    result = xrCreateInstance(&instance_create_info, &xr_instance);
    if (!xr_result(NULL, result, "Failed to create XR instance."))
        return 1;

    XrInstanceProperties instance_props = {
        .type = XR_TYPE_INSTANCE_PROPERTIES,
        .next = NULL,
    };

    result = xrGetInstanceProperties(xr_instance, &instance_props);
    if (!xr_result(NULL, result, "Failed to get instance info"))
        return 1;

    std::cout
        << "Runtime Name: " << instance_props.runtimeName << "\n"
        << "Runtime Version: " << XR_VERSION_MAJOR(instance_props.runtimeVersion) << "." << XR_VERSION_MINOR(instance_props.runtimeVersion) << "." << XR_VERSION_PATCH(instance_props.runtimeVersion) << std::endl;

    XrSystemGetInfo system_get_info = { .type = XR_TYPE_SYSTEM_GET_INFO, .next = NULL, .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY };
    result = xrGetSystem(xr_instance, &system_get_info, &xr_system_id);

    if (!xr_result(xr_instance, result, "Failed to get system for HMD form factor."))
        return 1;

    uint32_t blend_modes_count = 0;
    result = xrEnumerateEnvironmentBlendModes(xr_instance, xr_system_id, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &blend_modes_count, NULL);
    if (!xr_result(NULL, result, "Failed to enumerate number of blend modes"))
        return 1;

    std::vector<XrEnvironmentBlendMode> blendModes(blend_modes_count);
    result = xrEnumerateEnvironmentBlendModes(xr_instance, xr_system_id, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, blend_modes_count, &blend_modes_count, blendModes.data());
    if (!xr_result(NULL, result, "Failed to enumerate blend modes"))
        return 1;

    XrGraphicsRequirementsVulkanKHR vulkan_reqs = { .type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR };

    PFN_xrGetVulkanGraphicsRequirements2KHR pfnGetVulkanGraphicsRequirements2KHR = NULL;
    {
        result = xrGetInstanceProcAddr(xr_instance, "xrGetVulkanGraphicsRequirements2KHR", (PFN_xrVoidFunction*)&pfnGetVulkanGraphicsRequirements2KHR);
        if (!xr_result(xr_instance, result, "Failed to get Vulkan graphics requirements function!"))
            return 1;
    }

    pfnGetVulkanGraphicsRequirements2KHR(xr_instance, xr_system_id, &vulkan_reqs);

    check_vulkan_version(&vulkan_reqs);

    xr_view_count = 0;
    result = xrEnumerateViewConfigurationViews(xr_instance, xr_system_id, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &xr_view_count, NULL);
    if (!xr_result(xr_instance, result, "Failed to get view configuration count"))
        return 1;

    viewconfig_views.resize(xr_view_count, { XR_TYPE_VIEW_CONFIGURATION_VIEW, nullptr });
    last_acquired.resize(xr_view_count);

    result = xrEnumerateViewConfigurationViews(xr_instance, xr_system_id, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, xr_view_count, &xr_view_count, viewconfig_views.data());
    if (!xr_result(xr_instance, result, "Failed to enumerate view configuration views!"))
        return 1;

    print_viewconfig_view_info();

    initialized = true;

    return 0;
}

void OpenXRContext::clean()
{
    xrDestroySwapchain(xr_swapchain);
    xrDestroySpace(play_space);
    xrDestroySession(xr_session);
    xrDestroyInstance(xr_instance);
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
    result = xrEnumerateReferenceSpaces(xr_session, 0, &ref_space_count, NULL);
    if (!xr_result(xr_instance, result, "Getting number of reference spaces failed!"))
        return;

    std::vector<XrReferenceSpaceType> ref_spaces(ref_space_count);
    result = xrEnumerateReferenceSpaces(xr_session, ref_space_count, &ref_space_count, ref_spaces.data());
    if (!xr_result(xr_instance, result, "Enumerating reference spaces failed!"))
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
        XrActionSetCreateInfo actionSetInfo{ XR_TYPE_ACTION_SET_CREATE_INFO };
        strcpy(actionSetInfo.actionSetName, "gameplay");
        strcpy(actionSetInfo.localizedActionSetName, "Gameplay");
        actionSetInfo.priority = 0;
        (xrCreateActionSet(xr_instance, &actionSetInfo, &input_state.actionSet));
    }

    // Get the XrPath for the left and right hands - we will use them as subaction paths.
    (xrStringToPath(xr_instance, "/user/hand/left", &input_state.handSubactionPath[HAND_LEFT]));
    (xrStringToPath(xr_instance, "/user/hand/right",
        &input_state.handSubactionPath[HAND_RIGHT]));

    // Create actions.
    {
        // Create an input action for grabbing objects with the left and right hands.
        XrActionCreateInfo actionInfo{ XR_TYPE_ACTION_CREATE_INFO };
        actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
        strcpy(actionInfo.actionName, "grab_object");
        strcpy(actionInfo.localizedActionName, "Grab Object");
        actionInfo.countSubactionPaths = HAND_COUNT;
        actionInfo.subactionPaths = input_state.handSubactionPath;
        (xrCreateAction(input_state.actionSet, &actionInfo, &input_state.grabAction));

        // Create an input action getting the left and right hand poses.
        actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
        strcpy(actionInfo.actionName, "hand_pose");
        strcpy(actionInfo.localizedActionName, "Hand Pose");
        actionInfo.countSubactionPaths = HAND_COUNT;
        actionInfo.subactionPaths = input_state.handSubactionPath;
        (xrCreateAction(input_state.actionSet, &actionInfo, &input_state.poseAction));

        // Create output actions for vibrating the left and right controller.
        actionInfo.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
        strcpy(actionInfo.actionName, "vibrate_hand");
        strcpy(actionInfo.localizedActionName, "Vibrate Hand");
        actionInfo.countSubactionPaths = HAND_COUNT;
        actionInfo.subactionPaths = input_state.handSubactionPath;
        (xrCreateAction(input_state.actionSet, &actionInfo, &input_state.vibrateAction));

        // Create input actions for quitting the session using the left and right controller.
        // Since it doesn't matter which hand did this, we do not specify subaction paths for it.
        // We will just suggest bindings for both hands, where possible.
        actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
        strcpy(actionInfo.actionName, "quit_session");
        strcpy(actionInfo.localizedActionName, "Quit Session");
        actionInfo.countSubactionPaths = 0;
        actionInfo.subactionPaths = nullptr;
        (xrCreateAction(input_state.actionSet, &actionInfo, &input_state.quitAction));
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
    (xrStringToPath(xr_instance, "/user/hand/left/input/select/click", &selectPath[HAND_LEFT]));
    (xrStringToPath(xr_instance, "/user/hand/right/input/select/click",
        &selectPath[HAND_RIGHT]));
    (xrStringToPath(xr_instance, "/user/hand/left/input/squeeze/value",
        &squeezeValuePath[HAND_LEFT]));
    (xrStringToPath(xr_instance, "/user/hand/right/input/squeeze/value",
        &squeezeValuePath[HAND_RIGHT]));
    (xrStringToPath(xr_instance, "/user/hand/left/input/squeeze/force",
        &squeezeForcePath[HAND_LEFT]));
    (xrStringToPath(xr_instance, "/user/hand/right/input/squeeze/force",
        &squeezeForcePath[HAND_RIGHT]));
    (xrStringToPath(xr_instance, "/user/hand/left/input/squeeze/click",
        &squeezeClickPath[HAND_LEFT]));
    (xrStringToPath(xr_instance, "/user/hand/right/input/squeeze/click",
        &squeezeClickPath[HAND_RIGHT]));
    (xrStringToPath(xr_instance, "/user/hand/left/input/grip/pose", &posePath[HAND_LEFT]));
    (xrStringToPath(xr_instance, "/user/hand/right/input/grip/pose", &posePath[HAND_RIGHT]));
    (xrStringToPath(xr_instance, "/user/hand/left/output/haptic", &hapticPath[HAND_LEFT]));
    (xrStringToPath(xr_instance, "/user/hand/right/output/haptic", &hapticPath[HAND_RIGHT]));
    (xrStringToPath(xr_instance, "/user/hand/left/input/menu/click", &menuClickPath[HAND_LEFT]));
    (xrStringToPath(xr_instance, "/user/hand/right/input/menu/click",
        &menuClickPath[HAND_RIGHT]));
    (xrStringToPath(xr_instance, "/user/hand/left/input/b/click", &bClickPath[HAND_LEFT]));
    (xrStringToPath(xr_instance, "/user/hand/right/input/b/click", &bClickPath[HAND_RIGHT]));
    (xrStringToPath(xr_instance, "/user/hand/left/input/trigger/value",
        &triggerValuePath[HAND_LEFT]));
    (xrStringToPath(xr_instance, "/user/hand/right/input/trigger/value",
        &triggerValuePath[HAND_RIGHT]));
    // Suggest bindings for KHR Simple.
    {
        XrPath khrSimpleInteractionProfilePath;
        (
            xrStringToPath(xr_instance, "/interaction_profiles/khr/simple_controller",
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
        (xrSuggestInteractionProfileBindings(xr_instance, &suggestedBindings));
    }
    // Suggest bindings for the Oculus Touch.
    {
        XrPath oculusTouchInteractionProfilePath;
        (
            xrStringToPath(xr_instance, "/interaction_profiles/oculus/touch_controller",
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
        (xrSuggestInteractionProfileBindings(xr_instance, &suggestedBindings));
    }
    // Suggest bindings for the Vive Controller.
    /*{
        XrPath viveControllerInteractionProfilePath;
        (
                xrStringToPath(xr_instance, "/interaction_profiles/htc/vive_controller", &viveControllerInteractionProfilePath));
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
        (xrSuggestInteractionProfileBindings(xr_instance, &suggestedBindings));
    }

    // Suggest bindings for the Valve Index Controller.
    {
        XrPath indexControllerInteractionProfilePath;
        (
                xrStringToPath(xr_instance, "/interaction_profiles/valve/index_controller", &indexControllerInteractionProfilePath));
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
        (xrSuggestInteractionProfileBindings(xr_instance, &suggestedBindings));
    }

    // Suggest bindings for the Microsoft Mixed Reality Motion Controller.
    {
        XrPath microsoftMixedRealityInteractionProfilePath;
        (xrStringToPath(xr_instance, "/interaction_profiles/microsoft/motion_controller",
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
        (xrSuggestInteractionProfileBindings(xr_instance, &suggestedBindings));
    }*/
    XrActionSpaceCreateInfo actionSpaceInfo{ XR_TYPE_ACTION_SPACE_CREATE_INFO };
    actionSpaceInfo.action = input_state.poseAction;
    actionSpaceInfo.poseInActionSpace.orientation.w = 1.f;
    actionSpaceInfo.subactionPath = input_state.handSubactionPath[HAND_LEFT];
    (xrCreateActionSpace(xr_session, &actionSpaceInfo, &input_state.handSpace[HAND_LEFT]));
    actionSpaceInfo.subactionPath = input_state.handSubactionPath[HAND_RIGHT];
    (xrCreateActionSpace(xr_session, &actionSpaceInfo, &input_state.handSpace[HAND_RIGHT]));

    XrSessionActionSetsAttachInfo attachInfo{ XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
    attachInfo.countActionSets = 1;
    attachInfo.actionSets = &input_state.actionSet;
    (xrAttachSessionActionSets(xr_session, &attachInfo));
}

void OpenXRContext::initFrame()
{
    XrResult result;

    xr_frame_state.type = XR_TYPE_FRAME_STATE;

    XrFrameWaitInfo frameWaitInfo = {
      .type = XR_TYPE_FRAME_WAIT_INFO,
    };

    result = xrWaitFrame(xr_session, &frameWaitInfo, &xr_frame_state);
    if (!xr_result(xr_instance, result, "xrWaitFrame() was not successful, exiting..."))
        return;

    XrFrameBeginInfo frameBeginInfo = {
      .type = XR_TYPE_FRAME_BEGIN_INFO,
    };

    result = xrBeginFrame(xr_session, &frameBeginInfo);
    if (!xr_result(xr_instance, result, "failed to begin frame!"))
        return;

    XrViewState viewState{ XR_TYPE_VIEW_STATE };
    uint32_t viewCapacityInput = (uint32_t)m_views.size();
    uint32_t viewCountOutput;

    XrViewLocateInfo viewLocateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
    viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    viewLocateInfo.displayTime = xr_frame_state.predictedDisplayTime;
    viewLocateInfo.space = play_space;

    result = xrLocateViews(xr_session, &viewLocateInfo, &viewState, viewCapacityInput, &viewCountOutput, m_views.data());

    if ((viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 ||
        (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) {
        return;  // There is no valid tracking poses for the views.
    }

    std::vector<XrCompositionLayerProjectionView> projectionLayerViews;
    projectionLayerViews.resize(viewCountOutput);

    XrSwapchainImageAcquireInfo acquire_info = {
        .type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO,
    };

    result = xrAcquireSwapchainImage(xr_swapchain, &acquire_info, last_acquired.data());
    if (!xr_result(xr_instance, result, "failed to acquire swapchain image!"))
        return;

    XrSwapchainImageWaitInfo wait_info = {
      .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
      .timeout = INT64_MAX,
    };
    result = xrWaitSwapchainImage(xr_swapchain, &wait_info);
    if (!xr_result(xr_instance, result, "failed to wait for swapchain image!"))
        return;

}

void OpenXRContext::endFrame()
{
    XrResult result;

    XrSwapchainImageReleaseInfo info = {
    .type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
    };

    result = xrReleaseSwapchainImage(xr_swapchain, &info);
    if (!xr_result(xr_instance, result, "failed to release swapchain image!"))
        return;

    XrCompositionLayerProjection projection_layer = {
            .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
            .next = NULL,
            .layerFlags = 0,
            .space = play_space,
            .viewCount = xr_view_count,
            .views = projection_views.data(),
    };

    const XrCompositionLayerBaseHeader* submittedLayers[1] = {
            (const XrCompositionLayerBaseHeader* const)&projection_layer };

    XrFrameEndInfo frameEndInfo = {
        .type = XR_TYPE_FRAME_END_INFO,
        .displayTime = xr_frame_state.predictedDisplayTime,
        .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
        .layerCount = 1,
        .layers = submittedLayers,
    };

    result = xrEndFrame(xr_session, &frameEndInfo);
    if (!xr_result(xr_instance, result, "failed to end frame!"))
        return;
}
