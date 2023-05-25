#include "webgpu_context.h"

#include <cassert>

#define XR_USE_GRAPHICS_API_VULKAN
#include "dawnxr/dawnxr_internal.h"
#include <dawn/native/VulkanBackend.h>
#include "dawn/dawn_proc.h"

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

void PrintDeviceError(WGPUErrorType errorType, const char* message, void*) {
    const char* errorTypeName = "";
    switch (errorType) {
    case WGPUErrorType_Validation:
        errorTypeName = "Validation";
        break;
    case WGPUErrorType_OutOfMemory:
        errorTypeName = "Out of memory";
        break;
    case WGPUErrorType_Unknown:
        errorTypeName = "Unknown";
        break;
    case WGPUErrorType_DeviceLost:
        errorTypeName = "Device lost";
        break;
    default:
        return;
    }
    std::cout << errorTypeName << " error: " << message << std::endl;
}

void DeviceLostCallback(WGPUDeviceLostReason reason, const char* message, void*) {
    std::cout << "Device lost: " << message << std::endl;
}

void PrintGLFWError(int code, const char* message) {
    std::cout << "GLFW error: " << code << " - " << message << std::endl;
}

void DeviceLogCallback(WGPULoggingType type, const char* message, void*) {
    std::cout << "Device log: " << message << std::endl;
}

int WebGPUContext::initialize(OpenXRContext* xr_context, GLFWwindow* window)
{
    instance = new dawn::native::Instance();

    options = new dawn::native::AdapterDiscoveryOptionsBase * ();

    if (xr_context->initialized) {
        createVulkanAdapterDiscoveryOptions(xr_context->instance, xr_context->system_id, options);

        // This call creates internal vulkan instance
        instance->DiscoverAdapters(*options);
    }
    else {
        instance->DiscoverDefaultAdapters();
    }

    // Get an adapter for the backend to use, and create the device.
    dawn::native::Adapter backendAdapter;
    {
        std::vector<dawn::native::Adapter> adapters = instance->GetAdapters();
        auto adapterIt = std::find_if(adapters.begin(), adapters.end(),
            [](const dawn::native::Adapter adapter) -> bool {
                wgpu::AdapterProperties properties;
        adapter.GetProperties(&properties);
        return properties.backendType == wgpu::BackendType::Vulkan;
            });

        if (adapterIt == adapters.end()) {
            std::cout << "Could not find backend adapters" << std::endl;
            return 1;
        }
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

    device = wgpu::Device::Acquire(backendAdapter.CreateDevice(&deviceDesc));
    DawnProcTable backendProcs = dawn::native::GetProcs();

    dawnProcSetProcs(&backendProcs);

    backendProcs.deviceSetUncapturedErrorCallback(device.Get(), PrintDeviceError, nullptr);
    backendProcs.deviceSetDeviceLostCallback(device.Get(), DeviceLostCallback, nullptr);
    backendProcs.deviceSetLoggingCallback(device.Get(), DeviceLogCallback, nullptr);

    device_queue = device.GetQueue();

    // Create uniform buffer
    uniform_buffer.data = createBuffer(sizeof(glm::mat4x4), wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform, nullptr);
    uniform_buffer.binding = 0;

#ifdef USE_MIRROR_WINDOW
    // Create the swapchain for mirror mode
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    auto surfaceChainedDesc = wgpu::glfw::SetupWindowAndGetSurfaceDescriptor(window);
    wgpu::SurfaceDescriptor surfaceDesc;
    surfaceDesc.nextInChain = surfaceChainedDesc.get();
    surface = get_surface(window);

    wgpu::SwapChainDescriptor swapChainDesc = {};
    swapChainDesc.usage = wgpu::TextureUsage::RenderAttachment;
    swapChainDesc.format = wgpu::TextureFormat::BGRA8Unorm;
    swapChainDesc.width = width;
    swapChainDesc.height = height;
    swapChainDesc.presentMode = wgpu::PresentMode::Mailbox;
    mirror_swapchain = device.CreateSwapChain(surface, &swapChainDesc);

    mirror_swapchain_format = wgpu::TextureFormat::BGRA8Unorm;
    config_mirror_render_pipeline();
#endif

    dawn::native::InstanceProcessEvents(instance->Get());

    // Don't init xr stuff if not loaded
    if (!xr_context->initialized) {
        is_initialized = true;
        return 0;
    }

    dawnxr::GraphicsBindingDawn binding = { .device = device };

    XrSessionCreateInfo xrCreateInfo = { .type = XR_TYPE_SESSION_CREATE_INFO, .next = &binding, .systemId = xr_context->system_id };
    dawnxr::createSession(xr_context->instance, &xrCreateInfo, &xr_context->session);

    XrResult result;

    uint32_t swapchain_format_count;
    result = dawnxr::enumerateSwapchainFormats(xr_context->session, 0, &swapchain_format_count, NULL);
    if (!xr_context->xr_result(xr_context->instance, result, "Failed to get number of supported swapchain formats"))
        return 1;

    printf("Runtime supports %d swapchain formats\n", swapchain_format_count);
    std::vector<int64_t> swapchain_formats(swapchain_format_count);
    result = dawnxr::enumerateSwapchainFormats(xr_context->session, swapchain_format_count, &swapchain_format_count, swapchain_formats.data());
    if (!xr_context->xr_result(xr_context->instance, result, "Failed to enumerate swapchain formats"))
        return 1;

    swapchain_format = static_cast<wgpu::TextureFormat>(swapchain_formats[0]);

    config_render_pipeline();

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

    xr_context->swapchains.resize(xr_context->view_count);

    for (uint32_t i = 0; i < xr_context->view_count; i++) {
        dawnxr::createSwapchain(xr_context->session, &swapchain_create_info, &xr_context->swapchains[i].swapchain);

        uint32_t swapchain_length;
        result = dawnxr::enumerateSwapchainImages(xr_context->swapchains[i].swapchain, 0, &swapchain_length, nullptr);
        if (!xr_context->xr_result(xr_context->instance, result, "Failed to enumerate swapchains"))
            return 1;

        // these are wrappers for the actual OpenGL texture id
        xr_context->swapchains[i].images.resize(swapchain_length);
        result = dawnxr::enumerateSwapchainImages(xr_context->swapchains[i].swapchain, swapchain_length, &swapchain_length, (XrSwapchainImageBaseHeader*)xr_context->swapchains[i].images.data());
        if (!xr_context->xr_result(xr_context->instance, result, "Failed to enumerate swapchain images"))
            return 1;
    }


    XrReferenceSpaceType play_space_type = XR_REFERENCE_SPACE_TYPE_STAGE;
    // We could check if our ref space type is supported, but next call will error anyway if not
    xr_context->print_reference_spaces();

    XrReferenceSpaceCreateInfo play_space_create_info = { .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
                                                         .next = NULL,
                                                         .referenceSpaceType = play_space_type, //play_space_type,
                                                         .poseInReferenceSpace = identity_pose };

    result = xrCreateReferenceSpace(xr_context->session, &play_space_create_info, &xr_context->play_space);
    if (!xr_context->xr_result(xr_context->instance, result, "Failed to create play space!"))
        return 1;

    xr_context->projection_views.resize(xr_context->view_count);
    for (uint32_t i = 0; i < xr_context->view_count; i++) {
        xr_context->projection_views[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        xr_context->projection_views[i].next = NULL;
        xr_context->projection_views[i].pose = { .orientation = { 0.0f, 0.0f, 0.0f, 1.0f }, .position = { 0.0f, 0.0f, 0.0f } };

        xr_context->projection_views[i].subImage.swapchain = xr_context->swapchains[i].swapchain;
        xr_context->projection_views[i].subImage.imageArrayIndex = 0;
        xr_context->projection_views[i].subImage.imageRect.offset.x = 0;
        xr_context->projection_views[i].subImage.imageRect.offset.y = 0;
        xr_context->projection_views[i].subImage.imageRect.extent.width = xr_context->viewconfig_views[i].recommendedImageRectWidth;
        xr_context->projection_views[i].subImage.imageRect.extent.height = xr_context->viewconfig_views[i].recommendedImageRectHeight;

        // projection_views[i].pose (and fov) have to be filled every frame in frame loop
    };

    XrSessionBeginInfo session_begin_info = { .type = XR_TYPE_SESSION_BEGIN_INFO, .next = NULL, .primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO };
    result = xrBeginSession(xr_context->session, &session_begin_info);
    if (!xr_context->xr_result(xr_context->instance, result, "Failed to begin session!"))
        return 1;

    xr_context->init_actions();

    is_initialized = true;

    return 0;
}

void WebGPUContext::config_mirror_render_pipeline()
{
    // Load the shader module https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/hello-triangle.html
    {
        wgpu::ShaderModuleWGSLDescriptor shader_code_desc;
        shader_code_desc.code = RAW_SHADERS::simple_shaders;

        wgpu::ShaderModuleDescriptor shader_descr;
        shader_descr.nextInChain = &shader_code_desc;

        mirror_shader_module = device.CreateShaderModule(&shader_descr);
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
            .format = mirror_swapchain_format,
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

        mirror_render_pipeline_layout = device.CreatePipelineLayout(&layout_descr);
    }

    // Config the render pipeline
    {
        wgpu::FragmentState fragment_state = {
            .module = mirror_shader_module,
            .entryPoint = "fs_main",
            .constantCount = 0,
            .constants = NULL,
            .targetCount = 1,
            .targets = &color_target
        };

        wgpu::RenderPipelineDescriptor pipeline_descr = {
            .nextInChain = NULL,
            .layout = mirror_render_pipeline_layout,
            .vertex = {
                .module = mirror_shader_module,
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

        mirror_render_pipeline = device.CreateRenderPipeline(&pipeline_descr);
    }
}

wgpu::Buffer WebGPUContext::createBuffer(uint64_t size, wgpu::BufferUsage usage, const void* data)
{
    wgpu::BufferDescriptor bufferDesc{};

    bufferDesc.size = size;
    bufferDesc.usage = usage;
    bufferDesc.mappedAtCreation = false;

    wgpu::Buffer buffer = device.CreateBuffer(&bufferDesc);

    if (data != nullptr) {
        device_queue.WriteBuffer(buffer, 0, data, size);
    }

    return buffer;
}

wgpu::BindGroupLayout WebGPUContext::create_bind_group_layout(const std::vector<Uniform>& uniforms)
{
    std::vector<wgpu::BindGroupLayoutEntry> entries(uniforms.size());

    for (int i = 0; i < uniforms.size(); ++i) {
        entries[i] = uniforms[i].get_bind_group_layout_entry();
    }

    // Create a bind group layout
    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.entryCount = entries.size();
    bindGroupLayoutDesc.entries = entries.data();

    return device.CreateBindGroupLayout(&bindGroupLayoutDesc);
}

wgpu::BindGroup WebGPUContext::create_bind_group(const std::vector<Uniform>& uniforms)
{
    std::vector<wgpu::BindGroupEntry> entries(uniforms.size());

    for (int i = 0; i < uniforms.size(); ++i) {
        entries[i] = uniforms[i].get_bind_group_entry();
    }

    // A bind group contains one or multiple bindings
    wgpu::BindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.layout = render_bind_group_layout;
    // There must be as many bindings as declared in the layout!
    bindGroupDesc.entryCount = entries.size();
    bindGroupDesc.entries = entries.data();

    return device.CreateBindGroup(&bindGroupDesc);
}

wgpu::PipelineLayout WebGPUContext::create_pipeline_layout(const std::vector<wgpu::BindGroupLayout>& bind_group_layouts)
{
    wgpu::PipelineLayoutDescriptor layout_descr = {
        .nextInChain = NULL,
        .bindGroupLayoutCount = static_cast<uint32_t>(bind_group_layouts.size()),
        .bindGroupLayouts = bind_group_layouts.data()
    };

    return device.CreatePipelineLayout(&layout_descr);
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
        std::vector<Uniform> uniforms = { uniform_buffer };

        render_bind_group_layout = create_bind_group_layout(uniforms);
        render_pipeline_layout = create_pipeline_layout({ render_bind_group_layout });
        render_bind_group = create_bind_group(uniforms);
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

    shader_module.Release();
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

    wgpu::Surface surface = wgpu::Instance::Acquire(instance->Get()).CreateSurface(&descriptor);
#endif

    assert_msg(surface, "Error creating surface");
    return surface;
}

wgpu::BindGroupLayoutEntry Uniform::get_bind_group_layout_entry() const
{
    wgpu::BindGroupLayoutEntry bindingLayout = {};

    // The binding index as used in the @binding attribute in the shader
    bindingLayout.binding = binding;

    // The stage that needs to access this resource
    bindingLayout.visibility = visibility;

    bindingLayout.buffer.type = wgpu::BufferBindingType::Uniform;
    //bindingLayout.buffer.minBindingSize = sizeof(glm::mat4x4);

    return bindingLayout;
}

wgpu::BindGroupEntry Uniform::get_bind_group_entry() const
{
    // Create a binding
    wgpu::BindGroupEntry binding{};

    // The index of the binding (the entries in bindGroupDesc can be in any order)
    binding.binding = 0;
    // The buffer it is actually bound to
    binding.buffer = std::get<wgpu::Buffer>(data);
    // We can specify an offset within the buffer, so that a single buffer can hold
    // multiple uniform blocks.
    binding.offset = 0;
    // And we specify again the size of the buffer.
    binding.size = binding.buffer.GetSize();

    return binding;
}
