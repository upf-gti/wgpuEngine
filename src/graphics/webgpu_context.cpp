#include "webgpu_context.h"

#include <cassert>

#define XR_USE_GRAPHICS_API_VULKAN
#include "dawnxr/dawnxr_internal.h"
#include <dawn/native/VulkanBackend.h>
#include "dawn/dawn_proc.h"

#include "webgpu/webgpu_glfw.h"

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include "GLFW/glfw3native.h"

#include "utils.h"

using namespace dawnxr::internal;

wgpu::TextureFormat WebGPUContext::swapchain_format = wgpu::TextureFormat::BGRA8Unorm;
wgpu::TextureFormat WebGPUContext::xr_swapchain_format = wgpu::TextureFormat::BGRA8UnormSrgb;

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

int WebGPUContext::initialize(GLFWwindow* window, bool create_screen_swapchain)
{
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

    if (create_screen_swapchain) {
        // Create the swapchain for mirror mode
        glfwGetWindowSize(window, &screen_width, &screen_height);

        auto surfaceChainedDesc = wgpu::glfw::SetupWindowAndGetSurfaceDescriptor(window);
        wgpu::SurfaceDescriptor surfaceDesc;
        surfaceDesc.nextInChain = surfaceChainedDesc.get();
        surface = get_surface(window);

        wgpu::SwapChainDescriptor swapChainDesc = {};
        swapChainDesc.usage = wgpu::TextureUsage::RenderAttachment;
        swapChainDesc.format = swapchain_format;
        swapChainDesc.width = screen_width;
        swapChainDesc.height = screen_height;
        swapChainDesc.presentMode = wgpu::PresentMode::Mailbox;
        screen_swapchain = device.CreateSwapChain(surface, &swapChainDesc);
    }

    dawn::native::InstanceProcessEvents(instance->Get());

    is_initialized = true;

    return 0;
}

void WebGPUContext::create_instance()
{
    instance = new dawn::native::Instance();
}

wgpu::ShaderModule WebGPUContext::create_shader_module(char const* code)
{
    // Load the shader module https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/hello-triangle.html
    wgpu::ShaderModuleWGSLDescriptor shader_code_desc;
    shader_code_desc.code = code;

    wgpu::ShaderModuleDescriptor shader_descr;
    shader_descr.nextInChain = &shader_code_desc;

    return device.CreateShaderModule(&shader_descr);
}

wgpu::Buffer WebGPUContext::create_buffer(uint64_t size, wgpu::BufferUsage usage, const void* data)
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

wgpu::Texture WebGPUContext::create_texture(wgpu::TextureDimension dimension, wgpu::TextureFormat format, wgpu::Extent3D size, wgpu::TextureUsage usage, uint32_t mipmaps)
{
    wgpu::TextureDescriptor textureDesc;
    textureDesc.dimension = dimension;
    textureDesc.format = format;
    textureDesc.size = size;
    textureDesc.sampleCount = 1;
    textureDesc.viewFormatCount = 0;
    textureDesc.viewFormats = nullptr;
    textureDesc.usage = usage;
    textureDesc.mipLevelCount = mipmaps;

    return device.CreateTexture(&textureDesc);
}

wgpu::TextureView WebGPUContext::create_texture_view(wgpu::Texture texture, wgpu::TextureViewDimension dimension, wgpu::TextureFormat format)
{
    wgpu::TextureViewDescriptor textureViewDesc;
    textureViewDesc.aspect = wgpu::TextureAspect::All;
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.arrayLayerCount = 1;
    textureViewDesc.dimension = dimension;
    textureViewDesc.format = format;
    textureViewDesc.mipLevelCount = 1;
    textureViewDesc.baseMipLevel = 0;
    textureViewDesc.label = "Input View";

    return texture.CreateView(&textureViewDesc);
}

wgpu::BindGroupLayout WebGPUContext::create_bind_group_layout(const std::vector<Uniform*>& uniforms)
{
    std::vector<wgpu::BindGroupLayoutEntry> entries(uniforms.size());

    for (int i = 0; i < uniforms.size(); ++i) {
        entries[i] = uniforms[i]->get_bind_group_layout_entry();
    }

    // Create a bind group layout
    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.entryCount = entries.size();
    bindGroupLayoutDesc.entries = entries.data();

    return device.CreateBindGroupLayout(&bindGroupLayoutDesc);
}

wgpu::BindGroup WebGPUContext::create_bind_group(const std::vector<Uniform*>& uniforms, wgpu::BindGroupLayout bind_group_layout)
{
    std::vector<wgpu::BindGroupEntry> entries(uniforms.size());

    for (int i = 0; i < uniforms.size(); ++i) {
        entries[i] = uniforms[i]->get_bind_group_entry();
    }

    // A bind group contains one or multiple bindings
    wgpu::BindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.layout = bind_group_layout;
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

wgpu::RenderPipeline WebGPUContext::create_render_pipeline(const std::vector<wgpu::VertexBufferLayout>& vertex_attributes, wgpu::ColorTargetState color_target, wgpu::ShaderModule render_shader_module, wgpu::PipelineLayout pipeline_layout)
{    
    wgpu::VertexState vertex_state = {
        .module = render_shader_module,
        .entryPoint = "vs_main",
        .constantCount = 0,
        .constants = NULL,
        .bufferCount = static_cast<uint32_t>(vertex_attributes.size()),
        .buffers = vertex_attributes.data(),
    };

    wgpu::FragmentState fragment_state = {
        .module = render_shader_module,
        .entryPoint = "fs_main",
        .constantCount = 0,
        .constants = NULL,
        .targetCount = 1,
        .targets = &color_target
    };

    wgpu::RenderPipelineDescriptor pipeline_descr = {
        .nextInChain = NULL,
        .layout = pipeline_layout,
        .vertex = vertex_state,
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

    return device.CreateRenderPipeline(&pipeline_descr);
}

wgpu::ComputePipeline WebGPUContext::create_compute_pipeline(wgpu::ShaderModule compute_shader_module, wgpu::PipelineLayout pipeline_layout)
{
    wgpu::ComputePipelineDescriptor computePipelineDesc;
    computePipelineDesc.compute.constantCount = 0;
    computePipelineDesc.compute.constants = nullptr;
    computePipelineDesc.compute.entryPoint = "compute";
    computePipelineDesc.compute.module = compute_shader_module;
    computePipelineDesc.layout = pipeline_layout;
    return device.CreateComputePipeline(&computePipelineDesc);
}

wgpu::VertexBufferLayout WebGPUContext::create_vertex_buffer_layout(const std::vector<wgpu::VertexAttribute>& vertex_attributes, uint64_t stride, wgpu::VertexStepMode step_mode)
{
    wgpu::VertexBufferLayout vertexBufferLayout;
    vertexBufferLayout.attributeCount = vertex_attributes.size();
    vertexBufferLayout.attributes = vertex_attributes.data();
    vertexBufferLayout.arrayStride = stride;
    vertexBufferLayout.stepMode = step_mode;

    return vertexBufferLayout;
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

    if (std::holds_alternative<wgpu::Buffer>(data)) {
        bindingLayout.buffer.type = buffer_binding_type;
    } else 
    if (std::holds_alternative<wgpu::TextureView>(data)) {
        if (is_storage_texture) {
            bindingLayout.storageTexture = storage_texture_binding_layout;
        }
        else {
            bindingLayout.texture = texture_binding_layout;
        }
    }

    return bindingLayout;
}

wgpu::BindGroupEntry Uniform::get_bind_group_entry() const
{
    // Create a binding
    wgpu::BindGroupEntry bindingGroup{};

    // The index of the binding (the entries in bindGroupDesc can be in any order)
    bindingGroup.binding = binding;

    if (std::holds_alternative<wgpu::Buffer>(data)) {
        // The buffer it is actually bound to
        bindingGroup.buffer = std::get<wgpu::Buffer>(data);
        // We can specify an offset within the buffer, so that a single buffer can hold
        // multiple uniform blocks.
        bindingGroup.offset = 0;
        // And we specify again the size of the buffer.
        bindingGroup.size = bindingGroup.buffer.GetSize();
    } else
    if (std::holds_alternative<wgpu::TextureView>(data)) {
        bindingGroup.textureView = std::get<wgpu::TextureView>(data);
    }

    return bindingGroup;
}
