#include "webgpu_context.h"

#include <cassert>

#define XR_USE_GRAPHICS_API_VULKAN
#include "dawnxr/dawnxr_internal.h"
#include <dawn/native/VulkanBackend.h>

#include "webgpu/webgpu_glfw.h"
#include "openxr_context.h"

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include "GLFW/glfw3native.h"

#include "raw_shaders.h"

#include "utils.h"

using namespace dawnxr::internal;

// we need an identity pose for creating spaces without offsets
static XrPosef identity_pose = { .orientation = {.x = 0, .y = 0, .z = 0, .w = 1.0},
                                 .position = {.x = 0, .y = 0, .z = 0} };

int WebGPUContext::initialize(OpenXRContext* xr_context, GLFWwindow* window)
{
    options = new dawn::native::AdapterDiscoveryOptionsBase * ();
    createVulkanAdapterDiscoveryOptions(xr_context->xr_instance, xr_context->xr_system_id, options);

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

    int width, height;
    glfwGetWindowSize(window, &width, &height);

#ifdef USE_MIRROR_WINDOW
    // Create the swapchain for mirror mode
    auto surfaceChainedDesc = wgpu::glfw::SetupWindowAndGetSurfaceDescriptor(window);
    wgpu::SurfaceDescriptor surfaceDesc;
    surfaceDesc.nextInChain = surfaceChainedDesc.get();
    wgpu::Surface surface = get_surface(window);

    wgpu::SwapChainDescriptor swapChainDesc = {};
    swapChainDesc.usage = wgpu::TextureUsage::RenderAttachment;
    swapChainDesc.format = wgpu::TextureFormat::BGRA8Unorm;
    swapChainDesc.width = width;
    swapChainDesc.height = height;
    swapChainDesc.presentMode = wgpu::PresentMode::Mailbox;
    mirror_swapchain = device.CreateSwapChain(surface, &swapChainDesc);
#endif

    dawnxr::GraphicsBindingDawn binding = { .device = device };

    XrSessionCreateInfo xrCreateInfo = { .type = XR_TYPE_SESSION_CREATE_INFO, .next = &binding, .systemId = xr_context->xr_system_id };
    dawnxr::createSession(xr_context->xr_instance, &xrCreateInfo, &xr_context->xr_session);

    XrResult result;

    uint32_t swapchain_format_count;
    result = dawnxr::enumerateSwapchainFormats(xr_context->xr_session, 0, &swapchain_format_count, NULL);
    if (!xr_context->xr_result(xr_context->xr_instance, result, "Failed to get number of supported swapchain formats"))
        return 1;

    printf("Runtime supports %d swapchain formats\n", swapchain_format_count);
    std::vector<int64_t> swapchain_formats(swapchain_format_count);
    result = dawnxr::enumerateSwapchainFormats(xr_context->xr_session, swapchain_format_count, &swapchain_format_count, swapchain_formats.data());
    if (!xr_context->xr_result(xr_context->xr_instance, result, "Failed to enumerate swapchain formats"))
        return 1;

    XrSwapchainCreateInfo swapchain_create_info;
    swapchain_create_info.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
    swapchain_create_info.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_create_info.createFlags = 0;
    swapchain_create_info.format = swapchain_formats[0];
    swapchain_create_info.sampleCount = xr_context->viewconfig_views[0].recommendedSwapchainSampleCount;
    swapchain_create_info.width = xr_context->viewconfig_views[0].recommendedImageRectWidth;
    swapchain_create_info.height = xr_context->viewconfig_views[0].recommendedImageRectHeight;
    swapchain_create_info.faceCount = 1;
    swapchain_create_info.arraySize = 1;
    swapchain_create_info.mipCount = 1;
    swapchain_create_info.next = NULL;

    dawnxr::createSwapchain(xr_context->xr_session, &swapchain_create_info, &xr_context->xr_swapchain);

    uint32_t swapchain_length;
    result = dawnxr::enumerateSwapchainImages(xr_context->xr_swapchain, 0, &swapchain_length, nullptr);
    if (!xr_context->xr_result(xr_context->xr_instance, result, "Failed to enumerate swapchains"))
        return 1;

    // these are wrappers for the actual OpenGL texture id
    images.resize(swapchain_length);
    result = dawnxr::enumerateSwapchainImages(xr_context->xr_swapchain, swapchain_length, &swapchain_length, (XrSwapchainImageBaseHeader*)images.data());
    if (!xr_context->xr_result(xr_context->xr_instance, result, "Failed to enumerate swapchain images"))
        return 1;

    device_queue = device.GetQueue();

    XrReferenceSpaceType play_space_type = XR_REFERENCE_SPACE_TYPE_LOCAL;
    // We could check if our ref space type is supported, but next call will error anyway if not
    xr_context->print_reference_spaces();

    XrReferenceSpaceCreateInfo play_space_create_info = { .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
                                                         .next = NULL,
                                                         .referenceSpaceType = play_space_type,
                                                         .poseInReferenceSpace = identity_pose };

    result = xrCreateReferenceSpace(xr_context->xr_session, &play_space_create_info, &xr_context->play_space);
    if (!xr_context->xr_result(xr_context->xr_instance, result, "Failed to create play space!"))
        return 1;

    xr_context->projection_views.resize(xr_context->xr_view_count);
    for (uint32_t i = 0; i < xr_context->xr_view_count; i++) {
        xr_context->projection_views[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        xr_context->projection_views[i].next = NULL;
        xr_context->projection_views[i].pose = { .orientation = { 0.0f, 0.0f, 0.0f, 1.0f }, .position = { 0.0f, 0.0f, 0.0f } };

        xr_context->projection_views[i].subImage.swapchain = xr_context->xr_swapchain;
        xr_context->projection_views[i].subImage.imageArrayIndex = 0;
        xr_context->projection_views[i].subImage.imageRect.offset.x = 0;
        xr_context->projection_views[i].subImage.imageRect.offset.y = 0;
        xr_context->projection_views[i].subImage.imageRect.extent.width = xr_context->viewconfig_views[i].recommendedImageRectWidth;
        xr_context->projection_views[i].subImage.imageRect.extent.height = xr_context->viewconfig_views[i].recommendedImageRectHeight;

        // projection_views[i].pose (and fov) have to be filled every frame in frame loop
    };

    XrSessionBeginInfo session_begin_info = { .type = XR_TYPE_SESSION_BEGIN_INFO, .next = NULL, .primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO };
    result = xrBeginSession(xr_context->xr_session, &session_begin_info);
    if (!xr_context->xr_result(xr_context->xr_instance, result, "Failed to begin session!"))
        return 1;

    xr_context->init_actions();

    config_render_pipeline();

    is_initialized = true;

    return 0;
}

void WebGPUContext::config_render_pipeline()
{
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

wgpu::Surface WebGPUContext::get_surface(GLFWwindow* window)
{
#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(window);
    HINSTANCE hinstance = GetModuleHandle(NULL);

    wgpu::SurfaceDescriptorFromWindowsHWND chained;
    chained.hinstance = hinstance;
    chained.hwnd = hwnd;

    wgpu::SurfaceDescriptor descriptor = {
        .nextInChain = &chained,
        .label = nullptr,
    };

    wgpu::Surface surface = wgpu::Instance::Acquire(dawnInstance->Get()).CreateSurface(&descriptor);
#endif

    assert_msg(surface, "Error creating surface");
    return surface;
}
