#include "wgpu_environment.h"

#include "raw_shaders.h"

#include <GLFW/glfw3.h>
#include <stdint.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windef.h>

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

void WGPUEnv::sInstance::initialize(GLFWwindow *window, void *callback) {
    // Create the payload that stores the data during the async generation
    sWGPUInitializationPayload payload = {
        .wgpu_man_instance = this,
        .window = window, 
        .callback = callback
    };

    // Create instance
    {
        WGPUInstanceDescriptor instance_descr = {
            .nextInChain = NULL,
        };
        wgpuInstance = wgpuCreateInstance(&instance_descr);

        assert_msg(wgpuInstance, 
                   "Error creating WebGPU instance");
    }

    // Fetch the render surface
    {
        surface = get_surface(window, wgpuInstance);
    }

    // Request adapter
    {
        WGPURequestAdapterOptions options = {
            .nextInChain = NULL,
            .compatibleSurface = surface
        };
        wgpuInstanceRequestAdapter(wgpuInstance, 
                                   &options, 
                                   e_adapter_request_ended,
                                   (void *)&payload);
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

    result = xrCreateInstance(&instance_create_info, &openXRInstance);
    if (!xr_result(NULL, result, "Failed to create XR instance."))
        return 1;

    XrInstanceProperties instance_props = {
        .type = XR_TYPE_INSTANCE_PROPERTIES,
        .next = NULL,
    };

    result = xrGetInstanceProperties(openXRInstance, &instance_props);
    if (!xr_result(NULL, result, "Failed to get instance info"))
        return 1;

    std::cout
        << "Runtime Name: " << instance_props.runtimeName << "\n"
        << "Runtime Version: " << XR_VERSION_MAJOR(instance_props.runtimeVersion) << "." << XR_VERSION_MINOR(instance_props.runtimeVersion) << "." << XR_VERSION_PATCH(instance_props.runtimeVersion) << std::endl;

    XrSystemGetInfo system_get_info = { .type = XR_TYPE_SYSTEM_GET_INFO, .next = NULL, .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY };
    result = xrGetSystem(openXRInstance, &system_get_info, &systemId);

    if (!xr_result(openXRInstance, result, "Failed to get system for HMD form factor."))
        return 1;

    uint32_t blend_modes_count = 0;
    result = xrEnumerateEnvironmentBlendModes(openXRInstance, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &blend_modes_count, NULL);
    if (!xr_result(NULL, result, "Failed to enumerate number of blend modes"))
        return 1;

    std::vector<XrEnvironmentBlendMode> blendModes(blend_modes_count);
    result = xrEnumerateEnvironmentBlendModes(openXRInstance, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, blend_modes_count, &blend_modes_count, blendModes.data());
    if (!xr_result(NULL, result, "Failed to enumerate blend modes"))
        return 1;

    uint32_t vk_target_version = VK_MAKE_API_VERSION(0, 1, 0, 0);

    XrGraphicsRequirementsVulkanKHR vulkan_reqs = { .type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR };

    PFN_xrGetVulkanGraphicsRequirements2KHR pfnGetVulkanGraphicsRequirements2KHR = NULL;
    {
        result = xrGetInstanceProcAddr(openXRInstance, "xrGetVulkanGraphicsRequirements2KHR", (PFN_xrVoidFunction*)&pfnGetVulkanGraphicsRequirements2KHR);
        if (!xr_result(openXRInstance, result, "Failed to get Vulkan graphics requirements function!"))
            return 1;
    }

    pfnGetVulkanGraphicsRequirements2KHR(openXRInstance, systemId, &vulkan_reqs);

    check_vulkan_version(&vulkan_reqs);

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "wgpuEngine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "wgpuEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL,
    };

    XrVulkanInstanceCreateInfoKHR xr_vk_instance_info = {
        .type = XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR,
        .next = NULL,
        .systemId = systemId,
        .createFlags = 0,
        .pfnGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vulkanCreateInfo = &instance_info,
        .vulkanAllocator = NULL
    };

    PFN_xrCreateVulkanInstanceKHR CreateVulkanInstanceKHR = NULL;
    {
        XrResult result = xrGetInstanceProcAddr(openXRInstance, "xrCreateVulkanInstanceKHR", (PFN_xrVoidFunction*)&CreateVulkanInstanceKHR);
        if (!xr_result(openXRInstance, result, "Failed to load xrCreateVulkanInstanceKHR!"))
            return 1;
    }

    VkResult vk_result;
    result = CreateVulkanInstanceKHR(openXRInstance, &xr_vk_instance_info, &vulkanInstance, &vk_result);
    if (!xr_result(openXRInstance, result, "Failed to create Vulkan instance!"))
        return 1;
}

void WGPUEnv::sInstance::_config_render_pipeline() {
    // Load the shader module https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/hello-triangle.html
    {
        WGPUShaderModuleWGSLDescriptor shader_code_desc = {
            .chain = {
                .next = NULL,
                .sType = WGPUSType_ShaderModuleWGSLDescriptor
            },
            .code = RAW_SHADERS::simple_shaders
        };
        WGPUShaderModuleDescriptor shader_descr = {
            .nextInChain = &shader_code_desc.chain,
        };

        shader_module = wgpuDeviceCreateShaderModule(device, &shader_descr);
    }

    // Config the render target
    WGPUColorTargetState color_target;
    WGPUBlendState blend_state;
    {
        blend_state = {
            .color = {
                .operation = WGPUBlendOperation_Add,
                .srcFactor = WGPUBlendFactor_SrcAlpha,
                .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
            },
            .alpha = {
                .operation = WGPUBlendOperation_Add,
                .srcFactor = WGPUBlendFactor_Zero,
                .dstFactor = WGPUBlendFactor_One,
            }
        };

        color_target = {
            .format = swapchain_format,
            .blend = &blend_state,
            .writeMask = WGPUColorWriteMask_All
        };
    }

    // Layout descriptor (bind goups, buffers, uniforms)
    {
        WGPUPipelineLayoutDescriptor layout_descr = {
            .nextInChain = NULL,
            .bindGroupLayoutCount = 0,
            .bindGroupLayouts = NULL,
        };

        render_pipeline_layout = wgpuDeviceCreatePipelineLayout(device, &layout_descr);
    }

    // Config the render pipeline
    {   
        WGPUFragmentState fragment_state = {
            .module = shader_module,
            .entryPoint = "fs_main",
            .constantCount = 0,
            .constants = NULL,
            .targetCount = 1,
            .targets = &color_target
        };

        WGPURenderPipelineDescriptor pipeline_descr = {
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
                .topology = WGPUPrimitiveTopology_TriangleList,
                .stripIndexFormat = WGPUIndexFormat_Undefined, // order of the connected vertices
                .frontFace = WGPUFrontFace_CCW,
                .cullMode = WGPUCullMode_None
            },
            .depthStencil = NULL,
            .multisample = {
                .count = 1,
                .mask = ~0u,
                .alphaToCoverageEnabled = false
            },
            .fragment = &fragment_state,
        };
        render_pipeline = wgpuDeviceCreateRenderPipeline(device, &pipeline_descr);
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

std::vector<const char*> WGPUEnv::sInstance::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    //if (enableValidationLayers) {
    //    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    //}

    return extensions;
}

void WGPUEnv::sInstance::clean() {
    //wgpuInstanceRelease(wgpuInstance);
    //wgpuSwapChainDrop(swapchain);
    //wgpuDeviceDrop(device);
    //wgpuAdapterDrop(adapter);
}


void WGPUEnv::sInstance::render_frame() {
    // Get the current texture in the swapchain
    {
        current_texture_view = wgpuSwapChainGetCurrentTextureView(swapchain);
        assert_msg(current_texture_view != NULL, "Error, dont resize the window please!!");
    }

    // Create the command encoder
    {
        WGPUCommandEncoderDescriptor encoder_desc = {
            .nextInChain = NULL,
            .label = "Device command encoder"
        };
        device_command_encoder = wgpuDeviceCreateCommandEncoder(device, &encoder_desc);
    }

    // Create & fill the render pass (encoder)
    WGPURenderPassEncoder render_pass;
    {
        // Prepare the color attachment
        WGPURenderPassColorAttachment render_pass_color_attachment = {
            .view = current_texture_view,
            .resolveTarget = NULL, // for MS
            .loadOp = WGPULoadOp_Clear,
            .storeOp = WGPUStoreOp_Store,
            .clearValue = {0.0f,0.0f,1.0f,1.0f}
        };
        WGPURenderPassDescriptor render_pass_descr = {
            .nextInChain = NULL,
            .colorAttachmentCount = 1,
            .colorAttachments = &render_pass_color_attachment,
            .depthStencilAttachment = NULL,
            .timestampWriteCount = 0, // for measuing performance
            .timestampWrites = NULL
        };
        render_pass = wgpuCommandEncoderBeginRenderPass(device_command_encoder, &render_pass_descr);
        {
            // Bind Pipeline
            wgpuRenderPassEncoderSetPipeline(render_pass, render_pipeline);
            // Submit drawcall
            wgpuRenderPassEncoderDraw(render_pass, 3, 1, 0, 0);
        }
        wgpuRenderPassEncoderEnd(render_pass);

        wgpuTextureViewRelease(current_texture_view);
    }

    //
    {
        WGPUCommandBufferDescriptor cmd_buff_descriptor = {
            .nextInChain = NULL,
            .label = "Command buffer"
        };

        WGPUCommandBuffer commander = wgpuCommandEncoderFinish(device_command_encoder, &cmd_buff_descriptor);
        wgpuQueueSubmit(device_queue, 1, &commander);
    }

    // Submit frame
    {
        wgpuSwapChainPresent(swapchain);
    }
}


// Events =========================
void  WGPUEnv::sInstance::e_adapter_request_ended(WGPURequestAdapterStatus status, 
                                                  WGPUAdapter adapter, 
                                                  char const* message, 
                                                  void* user_data) {
    assert_msg(status == WGPURequestAdapterStatus_Success, "Error loading adapter");
    ((sWGPUInitializationPayload*) user_data)->wgpu_man_instance->adapter = adapter;
    
    // Inspect adapter features
    uint32_t function_count = wgpuAdapterEnumerateFeatures(adapter, NULL);

    WGPUFeatureName* features = (WGPUFeatureName*) malloc(sizeof(WGPUFeatureName) * function_count);

    wgpuAdapterEnumerateFeatures(adapter, features);
    
    //std::cout << "Features" << std::endl;
    //for(uint32_t i = 0; i < function_count; i++) {
    //    std::cout << features[i] << std::endl;
    //}
    //std::cout << "======" << std::endl;

    WGPUDeviceDescriptor descriptor = {
        .nextInChain = NULL,
        .label = "GPU",
        .requiredFeaturesCount = 0,
        .requiredLimits = NULL,
        .defaultQueue = {
            .nextInChain = NULL,
            .label = "Default queue"
        }
    };

    wgpuAdapterRequestDevice(adapter, &descriptor, e_device_request_ended, user_data);
}

void WGPUEnv::sInstance::e_device_request_ended(WGPURequestDeviceStatus status, 
                                                WGPUDevice device, 
                                                char const * message, 
                                                void *user_data) {
    //
    WGPUEnv::sInstance *instace = ((sWGPUInitializationPayload*) user_data)->wgpu_man_instance;
    assert_msg(status == WGPURequestDeviceStatus_Success, "Error loading the device");
    instace->device = device;

    // Set the error callback
    wgpuDeviceSetUncapturedErrorCallback(device, e_device_error, NULL);

    // Create the Device Queue
    {
        instace->device_queue = wgpuDeviceGetQueue(device);
    }

    // Create Swapchain
    {
        instace->swapchain_format = WGPUTextureFormat_BGRA8Unorm; // wgpuSurfaceGetPreferredFormat(instace->surface, instace->adapter);
        WGPUSwapChainDescriptor swapchain_descr = {
            .nextInChain = NULL,
            .usage = WGPUTextureUsage_RenderAttachment,
            .format = instace->swapchain_format,
            .width = 640,
            .height = 480,
            .presentMode = WGPUPresentMode_Fifo
        };

        instace->swapchain = wgpuDeviceCreateSwapChain(device, instace->surface, &swapchain_descr);
    }

    instace->is_initialized = true;
}

void WGPUEnv::sInstance::e_device_error(WGPUErrorType type, char const* message, void* user_data) {
    std::cout << "Error on Device: " << type;
    if (message) {
        std::cout << " - " << message;
    }

    std::cout  << std::endl;
}