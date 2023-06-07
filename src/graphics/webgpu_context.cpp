#include "webgpu_context.h"

#include <cassert>

#ifdef XR_SUPPORT
//#include "dawnxr/dawnxr_internal.h"
//#include <dawn/native/VulkanBackend.h>
//using namespace dawnxr::internal;
#endif

#ifndef __EMSCRIPTEN__
//#define XR_USE_GRAPHICS_API_VULKAN
//#include "webgpu/webgpu_glfw.h"
#endif

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include "GLFW/glfw3native.h"
#include "glfw3webgpu.h"

#include "utils.h"

WGPUTextureFormat WebGPUContext::swapchain_format = WGPUTextureFormat_BGRA8Unorm;
WGPUTextureFormat WebGPUContext::xr_swapchain_format = WGPUTextureFormat_BGRA8UnormSrgb;

void PrintDeviceError(WGPUErrorType errorType, const char* message, void* userdata) {
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

void DeviceLogCallback(WGPULoggingType type, const char* message) {
    std::cout << "Device log: " << message << std::endl;
}

WGPUAdapter requestAdapter(WGPUInstance instance, WGPURequestAdapterOptions const* options) {

    struct UserData {
        WGPUAdapter adapter = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* message, void* pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestAdapterStatus_Success) {
            userData.adapter = adapter;
        }
        else {
            std::cout << "Could not get WebGPU adapter: " << message << std::endl;
        }
        userData.requestEnded = true;
    };

    // Call to the WebGPU request adapter procedure
    wgpuInstanceRequestAdapter(
        instance,
        options,
        onAdapterRequestEnded,
        (void*)&userData
    );

    assert(userData.requestEnded);

    return userData.adapter;
}

WGPUDevice requestDevice(WGPUAdapter adapter, WGPUDeviceDescriptor const* descriptor) {
    struct UserData {
        WGPUDevice device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const* message, void* pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestDeviceStatus_Success) {
            userData.device = device;
        }
        else {
            std::cout << "Could not get WebGPU adapter: " << message << std::endl;
        }
        userData.requestEnded = true;
    };

    wgpuAdapterRequestDevice(
        adapter,
        descriptor,
        onDeviceRequestEnded,
        (void*)&userData
    );

    assert(userData.requestEnded);

    return userData.device;
}

int WebGPUContext::initialize(GLFWwindow* window, bool create_screen_swapchain)
{
    // Get an adapter for the backend to use, and create the device.
    //WGPUAdapter backendAdapter;
    //{
    //    std::vector<dawn::native::Adapter> adapters = instance->GetAdapters();
    //    auto adapterIt = std::find_if(adapters.begin(), adapters.end(),
    //        [](const dawn::native::Adapter adapter) -> bool {
    //            WGPUAdapterProperties properties;
    //    adapter.GetProperties(&properties);
    //    return properties.backendType == WGPUBackendType::Vulkan;
    //        });

    //    if (adapterIt == adapters.end()) {
    //        std::cout << "Could not find backend adapters" << std::endl;
    //        return 1;
    //    }
    //}


    WGPURequestAdapterOptions adapterOpts = {};
    //adapterOpts.compatibleSurface = surface;
    WGPUAdapter adapter = requestAdapter(get_instance(), &adapterOpts);

    WGPUDawnTogglesDescriptor toggles = {};
    toggles.chain.sType = WGPUSType_DawnTogglesDescriptor;
    toggles.chain.next = nullptr;
    toggles.enabledToggles = nullptr;
    toggles.enabledTogglesCount = 0;
    toggles.disabledToggles = nullptr;
    toggles.disabledTogglesCount = 0;

    WGPUSupportedLimits supportedLimits;
    wgpuAdapterGetLimits(adapter, &supportedLimits);
  
    WGPURequiredLimits requiredLimits = {};
    requiredLimits.limits.maxVertexAttributes = 4;
    requiredLimits.limits.maxVertexBuffers = 1;
    requiredLimits.limits.maxBindGroups = 2;
    requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
    requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);
    requiredLimits.limits.minUniformBufferOffsetAlignment = 64;
    requiredLimits.limits.minStorageBufferOffsetAlignment = 16;

    // Create device
    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.label = "My Device";
    deviceDesc.requiredFeaturesCount = 0;
    deviceDesc.requiredLimits = &requiredLimits;
    deviceDesc.defaultQueue.label = "The default queue";
    deviceDesc.deviceLostCallback = DeviceLostCallback;
    device = requestDevice(adapter, &deviceDesc);

    //device.setLoggingCallback(DeviceLogCallback);
    auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */) {
        std::cout << "Uncaptured device error: type " << type;
        if (message) std::cout << " (" << message << ")";
        std::cout << std::endl;
    };

    wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr);
    
    device_queue = wgpuDeviceGetQueue(device);

    if (create_screen_swapchain) {
        // Create the swapchain for mirror mode
        glfwGetWindowSize(window, &screen_width, &screen_height);

        surface = glfwGetWGPUSurface(get_instance(), window);
        //auto surfaceChainedDesc = WGPUglfw::SetupWindowAndGetSurfaceDescriptor(window);
        //WGPUSurfaceDescriptor surfaceDesc;
        //surfaceDesc.nextInChain = surfaceChainedDesc.get();
        //surface = get_surface(window);

        WGPUSwapChainDescriptor swapChainDesc = {};
        swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
        swapChainDesc.format = swapchain_format;
        swapChainDesc.width = screen_width;
        swapChainDesc.height = screen_height;
        swapChainDesc.presentMode = WGPUPresentMode_Mailbox;
        screen_swapchain = wgpuDeviceCreateSwapChain(device, surface, &swapChainDesc);
    }

    printErrors();

    is_initialized = true;

    return 0;
}

void WebGPUContext::create_instance()
{
#ifdef XR_SUPPORT
    instance = new dawn::native::Instance();
#else
    WGPUInstanceDescriptor instance_dscr = {};
    instance = wgpuCreateInstance(&instance_dscr);
#endif
}

WGPUShaderModule WebGPUContext::create_shader_module(char const* code)
{
    // Load the shader module https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/hello-triangle.html
    WGPUShaderModuleWGSLDescriptor shader_code_desc = {};
    shader_code_desc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    shader_code_desc.code = code;

    WGPUShaderModuleDescriptor shader_descr = {};
    shader_descr.nextInChain = &shader_code_desc.chain;

    return wgpuDeviceCreateShaderModule(device, &shader_descr);
}

WGPUBuffer WebGPUContext::create_buffer(uint64_t size, int usage, const void* data)
{
    WGPUBufferDescriptor bufferDesc = {};

    bufferDesc.size = size;
    bufferDesc.usage = usage;
    bufferDesc.mappedAtCreation = false;

    WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);

    if (data != nullptr) {
        wgpuQueueWriteBuffer(device_queue, buffer, 0, data, size);
    }

    return buffer;
}

WGPUTexture WebGPUContext::create_texture(WGPUTextureDimension dimension, WGPUTextureFormat format, WGPUExtent3D size, int usage, uint32_t mipmaps)
{
    WGPUTextureDescriptor textureDesc = {};
    textureDesc.dimension = dimension;
    textureDesc.format = format;
    textureDesc.size = size;
    textureDesc.sampleCount = 1;
    textureDesc.viewFormatCount = 0;
    textureDesc.viewFormats = nullptr;
    textureDesc.usage = usage;
    textureDesc.mipLevelCount = mipmaps;

    return wgpuDeviceCreateTexture(device, &textureDesc);
}

WGPUTextureView WebGPUContext::create_texture_view(WGPUTexture texture, WGPUTextureViewDimension dimension, WGPUTextureFormat format)
{
    WGPUTextureViewDescriptor textureViewDesc = {};
    textureViewDesc.aspect = WGPUTextureAspect_All;
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.arrayLayerCount = 1;
    textureViewDesc.dimension = dimension;
    textureViewDesc.format = format;
    textureViewDesc.mipLevelCount = 1;
    textureViewDesc.baseMipLevel = 0;
    textureViewDesc.label = "Input View";

    return wgpuTextureCreateView(texture, &textureViewDesc);
}

WGPUBindGroupLayout WebGPUContext::create_bind_group_layout(const std::vector<Uniform*>& uniforms)
{
    std::vector<WGPUBindGroupLayoutEntry> entries(uniforms.size());

    for (int i = 0; i < uniforms.size(); ++i) {
        entries[i] = uniforms[i]->get_bind_group_layout_entry();
    }

    // Create a bind group layout
    WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
    bindGroupLayoutDesc.entryCount = entries.size();
    bindGroupLayoutDesc.entries = entries.data();

    return wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);
}

WGPUBindGroup WebGPUContext::create_bind_group(const std::vector<Uniform*>& uniforms, WGPUBindGroupLayout bind_group_layout)
{
    std::vector<WGPUBindGroupEntry> entries(uniforms.size());

    for (int i = 0; i < uniforms.size(); ++i) {
        entries[i] = uniforms[i]->get_bind_group_entry();
    }

    // A bind group contains one or multiple bindings
    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = bind_group_layout;
    // There must be as many bindings as declared in the layout!
    bindGroupDesc.entryCount = entries.size();
    bindGroupDesc.entries = entries.data();

    return wgpuDeviceCreateBindGroup(device, &bindGroupDesc);
}

void WebGPUContext::printErrors()
{
    wgpuInstanceProcessEvents(get_instance());
}

WGPUPipelineLayout WebGPUContext::create_pipeline_layout(const std::vector<WGPUBindGroupLayout>& bind_group_layouts)
{
    WGPUPipelineLayoutDescriptor layout_descr = {};
    layout_descr.nextInChain = NULL;
    layout_descr.bindGroupLayoutCount = static_cast<uint32_t>(bind_group_layouts.size());
    layout_descr.bindGroupLayouts = (WGPUBindGroupLayout*)bind_group_layouts.data();

    return wgpuDeviceCreatePipelineLayout(device, &layout_descr);
}

WGPURenderPipeline WebGPUContext::create_render_pipeline(const std::vector<WGPUVertexBufferLayout>& vertex_attributes, WGPUColorTargetState color_target, WGPUShaderModule render_shader_module, WGPUPipelineLayout pipeline_layout)
{    
    WGPUVertexState vertex_state = {};
    vertex_state.module = render_shader_module;
    vertex_state.entryPoint = "vs_main";
    vertex_state.constantCount = 0;
    vertex_state.constants = NULL;
    vertex_state.bufferCount = static_cast<uint32_t>(vertex_attributes.size());
    vertex_state.buffers = vertex_attributes.data();

    WGPUFragmentState fragment_state = {};
    fragment_state.module = render_shader_module;
    fragment_state.entryPoint = "fs_main";
    fragment_state.constantCount = 0;
    fragment_state.constants = NULL;
    fragment_state.targetCount = 1;
    fragment_state.targets = &color_target;

    WGPURenderPipelineDescriptor pipeline_descr = {};
    pipeline_descr.nextInChain = NULL;
    pipeline_descr.layout = pipeline_layout;
    pipeline_descr.vertex = vertex_state;

    pipeline_descr.primitive = {
        .topology = WGPUPrimitiveTopology_TriangleList,
        .stripIndexFormat = WGPUIndexFormat_Undefined, // order of the connected vertices
        .frontFace = WGPUFrontFace_CCW,
        .cullMode = WGPUCullMode_None
    },

    pipeline_descr.depthStencil = NULL;
    pipeline_descr.multisample = {
            .count = 1,
            .mask = ~0u,
            .alphaToCoverageEnabled = false
    };

    pipeline_descr.fragment = &fragment_state;
 
    return wgpuDeviceCreateRenderPipeline(device, &pipeline_descr);
}

WGPUComputePipeline WebGPUContext::create_compute_pipeline(WGPUShaderModule compute_shader_module, WGPUPipelineLayout pipeline_layout)
{
    WGPUComputePipelineDescriptor computePipelineDesc = {};
    computePipelineDesc.compute.constantCount = 0;
    computePipelineDesc.compute.constants = nullptr;
    computePipelineDesc.compute.entryPoint = "compute";
    computePipelineDesc.compute.module = compute_shader_module;
    computePipelineDesc.layout = pipeline_layout;

    return wgpuDeviceCreateComputePipeline(device, &computePipelineDesc);
}

WGPUVertexBufferLayout WebGPUContext::create_vertex_buffer_layout(const std::vector<WGPUVertexAttribute>& vertex_attributes, uint64_t stride, WGPUVertexStepMode step_mode)
{
    WGPUVertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.attributeCount = vertex_attributes.size();
    vertexBufferLayout.attributes = vertex_attributes.data();
    vertexBufferLayout.arrayStride = stride;
    vertexBufferLayout.stepMode = step_mode;

    return vertexBufferLayout;
}

WGPUInstance WebGPUContext::get_instance()
{
#ifdef XR_SUPPORT
    return instance->Get();
#else
    return instance;
#endif
}

WGPUSurface WebGPUContext::get_surface(GLFWwindow* window)
{
#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(window);
    HINSTANCE hinstance = GetModuleHandle(NULL);

    WGPUSurfaceDescriptorFromWindowsHWND chained = {};
    chained.hinstance = hinstance;
    chained.hwnd = hwnd;

    WGPUSurfaceDescriptor descriptor = {};
    descriptor.nextInChain = &chained.chain;
    descriptor.label = nullptr;

    WGPUSurface surface = wgpuInstanceCreateSurface(get_instance(), &descriptor);
#endif

    assert_msg(surface, "Error creating surface");
    return surface;
}

Uniform::Uniform()
{
    texture_binding_layout.sampleType = WGPUTextureSampleType_Float;
    texture_binding_layout.viewDimension = WGPUTextureViewDimension_2D;
    texture_binding_layout.multisampled = false;

    storage_texture_binding_layout.access = WGPUStorageTextureAccess_WriteOnly;
    storage_texture_binding_layout.format = WGPUTextureFormat_RGBA8Unorm;
    storage_texture_binding_layout.viewDimension = WGPUTextureViewDimension_2D;
}

WGPUBindGroupLayoutEntry Uniform::get_bind_group_layout_entry() const
{
    WGPUBindGroupLayoutEntry bindingLayout = {};

    // The binding index as used in the @binding attribute in the shader
    bindingLayout.binding = binding;
    // The stage that needs to access this resource
    bindingLayout.visibility = visibility;

    if (std::holds_alternative<WGPUBuffer>(data)) {
        bindingLayout.buffer.type = buffer_binding_type;
    } else 
    if (std::holds_alternative<WGPUTextureView>(data)) {
        if (is_storage_texture) {
            bindingLayout.storageTexture = storage_texture_binding_layout;
        }
        else {
            bindingLayout.texture = texture_binding_layout;
        }
    }

    return bindingLayout;
}

WGPUBindGroupEntry Uniform::get_bind_group_entry() const
{
    // Create a binding
    WGPUBindGroupEntry bindingGroup = {};

    // The index of the binding (the entries in bindGroupDesc can be in any order)
    bindingGroup.binding = binding;

    if (std::holds_alternative<WGPUBuffer>(data)) {
        // The buffer it is actually bound to
        bindingGroup.buffer = std::get<WGPUBuffer>(data);
        // We can specify an offset within the buffer, so that a single buffer can hold
        // multiple uniform blocks.
        bindingGroup.offset = 0;
        // And we specify again the size of the buffer.
        bindingGroup.size = buffer_size;
    } else
    if (std::holds_alternative<WGPUTextureView>(data)) {
        bindingGroup.textureView = std::get<WGPUTextureView>(data);
    }

    return bindingGroup;
}
