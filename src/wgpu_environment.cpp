#include "wgpu_environment.h"

#include "raw_shaders.h"

#include <GLFW/glfw3.h>
#include <stdint.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windef.h>

#include "dawnxr/dawnxr_internal.h"
#include <dawn/native/VulkanBackend.h>

using namespace dawnxr::internal;

// we need an identity pose for creating spaces without offsets
static XrPosef identity_pose = { .orientation = {.x = 0, .y = 0, .z = 0, .w = 1.0},
                                 .position = {.x = 0, .y = 0, .z = 0} };

WGPUSurface get_surface(GLFWwindow *window, WGPUInstance wgpuInstance) {
#if WGPU_TARGET == WGPU_TARGET_WINDOWS
    HWND hwnd = glfwGetWin32Window(window);
    HINSTANCE hinstance = GetModuleHandle(NULL);

    const WGPUSurfaceDescriptorFromWindowsHWND chained = {
                    .chain = {
                            .next = NULL,
                            .sType = WGPUSType_SurfaceDescriptorFromWindowsHWND,
                        },
                    .hinstance = hinstance,
                    .hwnd = hwnd,
                };
    
    WGPUSurfaceDescriptor descriptor = {
            .nextInChain =
                (const WGPUChainedStruct *)&chained,
                .label = NULL,
    };

    WGPUSurface surface = wgpuInstanceCreateSurface(wgpuInstance, 
                                                    &descriptor);
#endif
    assert_msg(surface, "Error creating surface");

    return surface;
}

int WGPUEnv::sInstance::initialize(GLFWwindow *window, void *callback) {

    options = new dawn::native::AdapterDiscoveryOptionsBase * ();
    createVulkanAdapterDiscoveryOptions(xr_instance, xr_system_id, options);

    dawnInstance = new dawn::native::Instance();

    // This call creates internal vulkan instance
    dawnInstance->DiscoverAdapters(*options);

    // Get an adapter for the backend to use, and create the device.
    dawn::native::Adapter backendAdapter;
    {
        std::vector<dawn::native::Adapter> adapters = dawnInstance->GetAdapters();
        auto adapterIt = std::find_if(adapters.begin(), adapters.end(),
            [](const dawn::native::Adapter adapter) -> bool {
                wgpu::AdapterProperties properties;
        adapter.GetProperties(&properties);
        return properties.backendType == wgpu::BackendType::Vulkan;
            });
        assert(adapterIt != adapters.end());
        backendAdapter = *adapterIt;
    }

    WGPUDawnTogglesDescriptor toggles;
    toggles.chain.sType = WGPUSType_DawnTogglesDescriptor;
    toggles.chain.next = nullptr;
    toggles.enabledToggles = nullptr;
    toggles.enabledTogglesCount = 0;
    toggles.disabledToggles = nullptr;
    toggles.disabledTogglesCount = 0;

    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&toggles);

    device = static_cast<wgpu::Device>(backendAdapter.CreateDevice(&deviceDesc));
    DawnProcTable backendProcs = dawn::native::GetProcs();

    dawnxr::GraphicsBindingDawn binding = { .device = device };

    XrSessionCreateInfo xrCreateInfo = { .type = XR_TYPE_SESSION_CREATE_INFO, .next = &binding, .systemId = xr_system_id };
    dawnxr::createSession(xr_instance, &xrCreateInfo, &xr_session);

    XrResult result;

    uint32_t swapchain_format_count;
    result = dawnxr::enumerateSwapchainFormats(xr_session, 0, &swapchain_format_count, NULL);
    if (!xr_result(xr_instance, result, "Failed to get number of supported swapchain formats"))
        return 1;

    printf("Runtime supports %d swapchain formats\n", swapchain_format_count);
    std::vector<int64_t> swapchain_formats(swapchain_format_count);
    result = dawnxr::enumerateSwapchainFormats(xr_session, swapchain_format_count, &swapchain_format_count, swapchain_formats.data());
    if (!xr_result(xr_instance, result, "Failed to enumerate swapchain formats"))
        return 1;

    XrSwapchainCreateInfo swapchain_create_info;
    swapchain_create_info.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
    swapchain_create_info.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_create_info.createFlags = 0;
    swapchain_create_info.format = swapchain_formats[0];
    swapchain_create_info.sampleCount = viewconfig_views[0].recommendedSwapchainSampleCount;
    swapchain_create_info.width = viewconfig_views[0].recommendedImageRectWidth;
    swapchain_create_info.height = viewconfig_views[0].recommendedImageRectHeight;
    swapchain_create_info.faceCount = 1;
    swapchain_create_info.arraySize = 1;
    swapchain_create_info.mipCount = 1;
    swapchain_create_info.next = NULL;

    dawnxr::createSwapchain(xr_session, &swapchain_create_info, &xr_swapchain);

    uint32_t swapchain_length;
    result = dawnxr::enumerateSwapchainImages(xr_swapchain, 0, &swapchain_length, nullptr);
    if (!xr_result(xr_instance, result, "Failed to enumerate swapchains"))
        return 1;

    // these are wrappers for the actual OpenGL texture id
    images.resize(swapchain_length);
    result = dawnxr::enumerateSwapchainImages(xr_swapchain, swapchain_length, &swapchain_length, (XrSwapchainImageBaseHeader*)images.data());
    if (!xr_result(xr_instance, result, "Failed to enumerate swapchain images"))
        return 1;

    device_queue = device.GetQueue();

    XrReferenceSpaceType play_space_type = XR_REFERENCE_SPACE_TYPE_LOCAL;
    // We could check if our ref space type is supported, but next call will error anyway if not
    print_reference_spaces();

    XrReferenceSpaceCreateInfo play_space_create_info = { .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
                                                         .next = NULL,
                                                         .referenceSpaceType = play_space_type,
                                                         .poseInReferenceSpace = identity_pose };

    result = xrCreateReferenceSpace(xr_session, &play_space_create_info, &play_space);
    if (!xr_result(xr_instance, result, "Failed to create play space!"))
        return 1;

    projection_views.resize(xr_view_count);
    for (uint32_t i = 0; i < xr_view_count; i++) {
        projection_views[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        projection_views[i].next = NULL;
        projection_views[i].pose = { .orientation = { 0.0f, 0.0f, 0.0f, 1.0f }, .position = { 0.0f, 0.0f, 0.0f } };

        projection_views[i].subImage.swapchain = xr_swapchain;
        projection_views[i].subImage.imageArrayIndex = 0;
        projection_views[i].subImage.imageRect.offset.x = 0;
        projection_views[i].subImage.imageRect.offset.y = 0;
        projection_views[i].subImage.imageRect.extent.width = viewconfig_views[i].recommendedImageRectWidth;
        projection_views[i].subImage.imageRect.extent.height = viewconfig_views[i].recommendedImageRectHeight;

        // projection_views[i].pose (and fov) have to be filled every frame in frame loop
    };

    XrSessionBeginInfo session_begin_info = { .type = XR_TYPE_SESSION_BEGIN_INFO, .next = NULL, .primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO };
    result = xrBeginSession(xr_session, &session_begin_info);
    if (!xr_result(xr_instance, result, "Failed to begin session!"))
        return 1;

    is_initialized = true;
}

void WGPUEnv::sInstance::print_viewconfig_view_info()
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

void WGPUEnv::sInstance::print_reference_spaces()
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

int WGPUEnv::sInstance::initialize_openxr()
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

    uint32_t vk_target_version = VK_MAKE_API_VERSION(0, 1, 0, 0);

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

}

void WGPUEnv::sInstance::_config_render_pipeline() {
    // Load the shader module https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/hello-triangle.html
    {
        wgpu::ShaderModuleWGSLDescriptor shader_code_desc;
        shader_code_desc.code = RAW_SHADERS::simple_shaders;

        wgpu::ShaderModuleDescriptor shader_descr;
        shader_descr.nextInChain = &shader_code_desc;

        shader_module = device.CreateShaderModule(&shader_descr);
    }

    // Config the render target
    wgpu::ColorTargetState color_target;
    wgpu::BlendState blend_state;
    {
        blend_state = {
            .color = {
                .operation = wgpu::BlendOperation::Add,
                .srcFactor = wgpu::BlendFactor::SrcAlpha,
                .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha,
            },
            .alpha = {
                .operation = wgpu::BlendOperation::Add,
                .srcFactor = wgpu::BlendFactor::Zero,
                .dstFactor = wgpu::BlendFactor::One,
            }
        };

        color_target = {
            .format = swapchain_format,
            .blend = &blend_state,
            .writeMask = wgpu::ColorWriteMask::All
        };
    }

    // Layout descriptor (bind goups, buffers, uniforms)
    {
        wgpu::PipelineLayoutDescriptor layout_descr = {
            .nextInChain = NULL,
            .bindGroupLayoutCount = 0,
            .bindGroupLayouts = NULL,
        };

        render_pipeline_layout = device.CreatePipelineLayout(&layout_descr);
    }

    // Config the render pipeline
    {   
        wgpu::FragmentState fragment_state = {
            .module = shader_module,
            .entryPoint = "fs_main",
            .constantCount = 0,
            .constants = NULL,
            .targetCount = 1,
            .targets = &color_target
        };

        wgpu::RenderPipelineDescriptor pipeline_descr = {
            .nextInChain = NULL,
            .layout = render_pipeline_layout,
            .vertex = {
                .module = shader_module,
                .entryPoint = "vs_main",
                .constantCount = 0,
                .constants = NULL,
                .bufferCount = 0,
                .buffers = NULL,
                
            },
            .primitive = {
                .topology = wgpu::PrimitiveTopology::TriangleList,
                .stripIndexFormat = wgpu::IndexFormat::Undefined, // order of the connected vertices
                .frontFace = wgpu::FrontFace::CCW,
                .cullMode = wgpu::CullMode::None
            },
            .depthStencil = NULL,
            .multisample = {
                .count = 1,
                .mask = ~0u,
                .alphaToCoverageEnabled = false
            },
            .fragment = &fragment_state,
        };

        render_pipeline = device.CreateRenderPipeline(&pipeline_descr);
    }
}

bool WGPUEnv::sInstance::xr_result(XrInstance wgpuInstance, XrResult result, const char* format, ...)
{
    if (XR_SUCCEEDED(result))
        return true;

    char resultString[XR_MAX_RESULT_STRING_SIZE];
    xrResultToString(wgpuInstance, result, resultString);

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

bool WGPUEnv::sInstance::check_vulkan_version(XrGraphicsRequirementsVulkanKHR* vulkan_reqs)
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

void WGPUEnv::sInstance::clean() {
    //wgpuInstance.Release();
    //swapchain.Release();
    //device.Release();
    //adapter.Release();
}

void WGPUEnv::sInstance::render_frame() {

    // --- Begin frame
    XrResult result;

    XrEventDataBuffer runtimeEvent = {
        .type = XR_TYPE_EVENT_DATA_BUFFER,
        .next = NULL,
    };

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


    // Get the current texture in the swapchain
    {
        current_texture_view = images[0].textureView;
        assert_msg(current_texture_view != NULL, "Error, dont resize the window please!!");
    }

    // Create the command encoder
    {
        wgpu::CommandEncoderDescriptor encoder_desc;
        encoder_desc.label = "Device command encoder";

        device_command_encoder = device.CreateCommandEncoder(&encoder_desc);
    }

    // Create & fill the render pass (encoder)
    wgpu::RenderPassEncoder render_pass;
    {
        // Prepare the color attachment
        wgpu::RenderPassColorAttachment render_pass_color_attachment = {
            .view = current_texture_view,
            .resolveTarget = nullptr, // for MS
            .loadOp = wgpu::LoadOp::Clear,
            .storeOp = wgpu::StoreOp::Store,
            .clearValue = {0.0f,0.0f,1.0f,1.0f}
        };
        wgpu::RenderPassDescriptor render_pass_descr = {
            .nextInChain = NULL,
            .colorAttachmentCount = 1,
            .colorAttachments = &render_pass_color_attachment,
            .depthStencilAttachment = NULL,
            .timestampWriteCount = 0, // for measuing performance
            .timestampWrites = NULL
        };
        render_pass = device_command_encoder.BeginRenderPass(&render_pass_descr);
        {
            // Bind Pipeline
            render_pass.SetPipeline(render_pipeline);
            // Submit drawcall
            render_pass.Draw(3, 1, 0, 0);
        }
        render_pass.End();

        current_texture_view.Release();
    }

    //
    {
        wgpu::CommandBufferDescriptor cmd_buff_descriptor = {
            .nextInChain = NULL,
            .label = "Command buffer"
        };

        wgpu::CommandBuffer commander = device_command_encoder.Finish(&cmd_buff_descriptor);
        device_queue.Submit(1, &commander);
    }


    XrSwapchainImageReleaseInfo info = {
        .type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
    };

    XrResult res = xrReleaseSwapchainImage(xr_swapchain, &info);
    if (!xr_result(xr_instance, res, "failed to release swapchain image!"))
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

    // Submit frame
    {
        //wgpu::SwapChainPresent(swapchain);
    }
}

void WGPUEnv::sInstance::e_device_error(WGPUErrorType type, char const* message, void* user_data) {
    std::cout << "Error on Device: " << type;
    if (message) {
        std::cout << " - " << message;
    }

    std::cout  << std::endl;
}