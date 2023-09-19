#include "webgpu_context.h"
#include "utils.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5_webgpu.h>
#else
#include "glfw3webgpu.h"
#endif

#include "shader.h"

#include <iostream>

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

#ifdef __EMSCRIPTEN__
    while (!userData.requestEnded) {
        emscripten_sleep(100);
    }
#endif

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

#ifdef __EMSCRIPTEN__
    while (!userData.requestEnded) {
        emscripten_sleep(100);
    }
#endif

    assert(userData.requestEnded);

    return userData.device;
}

int WebGPUContext::initialize(WGPURequestAdapterOptions adapter_opts, WGPURequiredLimits required_limits, GLFWwindow* window, bool create_screen_swapchain)
{
    WGPUAdapter adapter = requestAdapter(get_instance(), &adapter_opts);

    // Create device
    WGPUDeviceDescriptor device_desc = {};
    device_desc.label = "My Device";
    device_desc.requiredFeaturesCount = 0;
    device_desc.requiredLimits = &required_limits;
    device_desc.defaultQueue.label = "The default queue";
    device_desc.deviceLostCallback = DeviceLostCallback;

#if !defined(__EMSCRIPTEN__) && !defined(NDEBUG)
    const char* const enabled_toggles[] = { "disable_symbol_renaming" };

    WGPUDawnTogglesDescriptor device_toggles_desc = {};
    device_toggles_desc.enabledToggles = enabled_toggles;
    device_toggles_desc.enabledTogglesCount = 1;

    WGPUChainedStruct* chain_desc = reinterpret_cast<WGPUChainedStruct*>(&device_toggles_desc);
    chain_desc->sType = WGPUSType_DawnTogglesDescriptor;
    device_desc.nextInChain = chain_desc;
#endif
    
    device = requestDevice(adapter, &device_desc);

#ifdef __EMSCRIPTEN__
    // emscripten-specific extension not supported by webgpu.cpp
    WGPUSurfaceDescriptorFromCanvasHTMLSelector canvDesc = {};
    canvDesc.chain.sType = WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector;
    canvDesc.selector = "canvas";

    WGPUSurfaceDescriptor surfDesc = {};
    surfDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&canvDesc);

    surface = wgpuInstanceCreateSurface(get_instance(), &surfDesc);
#else

    wgpuDeviceSetUncapturedErrorCallback(device, PrintDeviceError, nullptr);

    if (create_screen_swapchain) {
        surface = glfwGetWGPUSurface(get_instance(), window);
    }

    print_errors();

#endif

    if (create_screen_swapchain) {
        // Create the swapchain for mirror mode
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        create_swapchain(width, height);
    }

    device_queue = wgpuDeviceGetQueue(device);

    is_initialized = true;

    return 0;
}

void WebGPUContext::destroy()
{
    if (!is_initialized)
        return;

#ifdef XR_SUPPORT
    wgpuInstanceRelease(instance->Get());
#else
    wgpuInstanceRelease(instance);
#endif

    wgpuSurfaceRelease(surface);
    wgpuDeviceDestroy(device);
    wgpuQueueRelease(device_queue);
    wgpuSwapChainRelease(screen_swapchain);
}

void WebGPUContext::create_instance()
{
    WGPUInstanceDescriptor instance_dscr = {};

#if !defined(__EMSCRIPTEN__)

#if !defined(NDEBUG)
    // allow_unsafe_apis is currently required to prevent forced breakpoint hit when DAWN_DEBUG_BREAK_ON_ERROR=1
    const char* const enabled_toggles[] = { "allow_unsafe_apis" };

    WGPUDawnTogglesDescriptor device_toggles_desc = {};
    device_toggles_desc.enabledToggles = enabled_toggles;
    device_toggles_desc.enabledTogglesCount = 1;

    WGPUChainedStruct* chain_desc = reinterpret_cast<WGPUChainedStruct*>(&device_toggles_desc);
    chain_desc->sType = WGPUSType_DawnTogglesDescriptor;
    instance_dscr.nextInChain = chain_desc;
#endif // !NDEBUG

#ifdef XR_SUPPORT
    instance = new dawn::native::Instance(&instance_dscr);
#endif

#endif // !__EMSCRIPTEN__

#if defined(__EMSCRIPTEN__) || !defined(XR_SUPPORT)
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

WGPUBuffer WebGPUContext::create_buffer(uint64_t size, int usage, const void* data, const char* label)
{
    WGPUBufferDescriptor bufferDesc = {};

    bufferDesc.size = size;
    bufferDesc.usage = usage;
    bufferDesc.mappedAtCreation = false;
    bufferDesc.label = label;

    WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);

    if (data != nullptr) {
        wgpuQueueWriteBuffer(device_queue, buffer, 0, data, size);
    }

    return buffer;
}

WGPUTexture WebGPUContext::create_texture(WGPUTextureDimension dimension, WGPUTextureFormat format, WGPUExtent3D size, WGPUTextureUsage usage, uint32_t mipmaps)
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

WGPUTextureView WebGPUContext::create_texture_view(WGPUTexture texture, WGPUTextureViewDimension dimension, WGPUTextureFormat format, WGPUTextureAspect aspect)
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

WGPUSampler WebGPUContext::create_sampler(WGPUAddressMode wrap, WGPUFilterMode mag_filter, WGPUFilterMode min_filter, WGPUMipmapFilterMode mipmap_filter)
{
    WGPUSamplerDescriptor samplerDesc = {};
    samplerDesc.addressModeU = wrap;
    samplerDesc.addressModeV = wrap;
    samplerDesc.addressModeW = wrap;
    samplerDesc.magFilter = mag_filter;
    samplerDesc.minFilter = min_filter;
    samplerDesc.mipmapFilter = mipmap_filter;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = 1.0f;
    samplerDesc.compare = WGPUCompareFunction_Undefined;
    samplerDesc.maxAnisotropy = 1;

    return wgpuDeviceCreateSampler(device, &samplerDesc);
}

void WebGPUContext::create_texture_mipmaps(WGPUTexture texture, WGPUExtent3D texture_size, uint32_t mip_level_count, const void* data)
{
    WGPUQueue mipmap_queue = wgpuDeviceGetQueue(device);

    WGPUImageCopyTexture destination = {};
    destination.texture = texture;
    destination.mipLevel = 0;
    destination.origin = { 0, 0, 0 };
    destination.aspect = WGPUTextureAspect_All;

    WGPUTextureDataLayout source = {};
    source.offset = 0;

    // Create image data
    WGPUExtent3D mip_level_size = texture_size;
    std::vector<unsigned char> previous_level_pixels;
    WGPUExtent3D previous_mip_level_size;
    for (uint32_t level = 0; level < mip_level_count; ++level) {
        // Pixel data for the current level
        std::vector<unsigned char> pixels(4 * mip_level_size.width * mip_level_size.height);
        if (level == 0) {
            // We cannot really avoid this copy since we need this
            // in previous_level_pixels at the next iteration
            memcpy(pixels.data(), data, pixels.size());
        }
        else {
            // Create mip level data
            for (uint32_t i = 0; i < mip_level_size.width; ++i) {
                for (uint32_t j = 0; j < mip_level_size.height; ++j) {
                    unsigned char* p = &pixels[4 * (j * mip_level_size.width + i)];
                    // Get the corresponding 4 pixels from the previous level
                    unsigned char* p00 = &previous_level_pixels[4 * ((2 * j + 0) * previous_mip_level_size.width + (2 * i + 0))];
                    unsigned char* p01 = &previous_level_pixels[4 * ((2 * j + 0) * previous_mip_level_size.width + (2 * i + 1))];
                    unsigned char* p10 = &previous_level_pixels[4 * ((2 * j + 1) * previous_mip_level_size.width + (2 * i + 0))];
                    unsigned char* p11 = &previous_level_pixels[4 * ((2 * j + 1) * previous_mip_level_size.width + (2 * i + 1))];
                    // Average
                    p[0] = (p00[0] + p01[0] + p10[0] + p11[0]) / 4;
                    p[1] = (p00[1] + p01[1] + p10[1] + p11[1]) / 4;
                    p[2] = (p00[2] + p01[2] + p10[2] + p11[2]) / 4;
                    p[3] = (p00[3] + p01[3] + p10[3] + p11[3]) / 4;
                }
            }
        }

        // Upload data to the GPU texture
        destination.mipLevel = level;
        source.bytesPerRow = 4 * mip_level_size.width;
        source.rowsPerImage = mip_level_size.height;

        wgpuQueueWriteTexture(mipmap_queue, &destination, pixels.data(), pixels.size(), &source, &mip_level_size);

        previous_level_pixels = std::move(pixels);
        previous_mip_level_size = mip_level_size;
        mip_level_size.width /= 2;
        mip_level_size.height /= 2;
    }

    wgpuQueueRelease(mipmap_queue);
}

WGPUBindGroupLayout WebGPUContext::create_bind_group_layout(const std::vector<Uniform*>& uniforms)
{
    std::vector<WGPUBindGroupLayoutEntry> entries(uniforms.size());

    for (int i = 0; i < uniforms.size(); ++i) {
        entries[i] = uniforms[i]->get_bind_group_layout_entry();
    }

    // Create a bind group layout
    WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
    bindGroupLayoutDesc.entryCount = static_cast<uint32_t>(entries.size());
    bindGroupLayoutDesc.entries = entries.data();

    return wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);
}

WGPUBindGroupLayout WebGPUContext::create_bind_group_layout(const std::vector<WGPUBindGroupLayoutEntry> &entries)
{
    // Create a bind group layout
    WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
    bindGroupLayoutDesc.entryCount = static_cast<uint32_t>(entries.size());
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
    bindGroupDesc.entryCount = static_cast<uint32_t>(entries.size());
    bindGroupDesc.entries = entries.data();

    return wgpuDeviceCreateBindGroup(device, &bindGroupDesc);
}

WGPUBindGroup WebGPUContext::create_bind_group(const std::vector<Uniform*>& uniforms, Shader* shader, uint16_t bind_group)
{
    std::vector<WGPUBindGroupEntry> entries(uniforms.size());

    for (int i = 0; i < uniforms.size(); ++i) {
        entries[i] = uniforms[i]->get_bind_group_entry();
    }

    std::map<int, WGPUBindGroupLayout>& layouts_by_id = shader->get_bind_group_layouts();

    if (!layouts_by_id.contains(bind_group)) {
        std::cout << "Can't find bind group " << bind_group << " in shader: " << shader->get_path() << std::endl;
        assert(0);
    }

    // A bind group contains one or multiple bindings
    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = layouts_by_id[bind_group];
    // There must be as many bindings as declared in the layout!
    bindGroupDesc.entryCount = static_cast<uint32_t>(entries.size());
    bindGroupDesc.entries = entries.data();

    return wgpuDeviceCreateBindGroup(device, &bindGroupDesc);
}

void WebGPUContext::print_errors()
{
#ifndef __EMSCRIPTEN__
    wgpuInstanceProcessEvents(get_instance());
#endif
}

WGPUPipelineLayout WebGPUContext::create_pipeline_layout(const std::vector<WGPUBindGroupLayout>& bind_group_layouts)
{
    WGPUPipelineLayoutDescriptor layout_descr = {};
    layout_descr.nextInChain = NULL;
    layout_descr.bindGroupLayoutCount = static_cast<uint32_t>(bind_group_layouts.size());
    layout_descr.bindGroupLayouts = (WGPUBindGroupLayout*)bind_group_layouts.data();

    return wgpuDeviceCreatePipelineLayout(device, &layout_descr);
}

WGPURenderPipeline WebGPUContext::create_render_pipeline(WGPUShaderModule render_shader_module, WGPUPipelineLayout pipeline_layout, const std::vector<WGPUVertexBufferLayout>& vertex_attributes, WGPUColorTargetState color_target, bool uses_depth_buffer)
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

    WGPUDepthStencilState depth_state = {};
    if (uses_depth_buffer) {
        depth_state.depthCompare = WGPUCompareFunction_Less;
        depth_state.depthWriteEnabled = true;
        depth_state.format = WGPUTextureFormat_Depth32Float;
        depth_state.stencilReadMask = 0;
        depth_state.stencilWriteMask = 0;
        // Configure the stencils even if unused
        depth_state.stencilFront.compare = WGPUCompareFunction_Always;
        depth_state.stencilFront.failOp = WGPUStencilOperation_Keep;
        depth_state.stencilFront.depthFailOp = WGPUStencilOperation_Keep;
        depth_state.stencilFront.passOp = WGPUStencilOperation_Keep;
        depth_state.stencilBack.compare = WGPUCompareFunction_Always;
        depth_state.stencilBack.failOp = WGPUStencilOperation_Keep;
        depth_state.stencilBack.depthFailOp = WGPUStencilOperation_Keep;
        depth_state.stencilBack.passOp = WGPUStencilOperation_Keep;
    }
    

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

    pipeline_descr.depthStencil = (uses_depth_buffer) ? &depth_state : nullptr;
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
    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertex_attributes.size());
    vertexBufferLayout.attributes = vertex_attributes.data();
    vertexBufferLayout.arrayStride = stride;
    vertexBufferLayout.stepMode = step_mode;

    return vertexBufferLayout;
}

WGPUInstance WebGPUContext::get_instance()
{
#if defined(XR_SUPPORT) && !defined(__EMSCRIPTEN__)
    return instance->Get();
#else
    return instance;
#endif
}

void WebGPUContext::create_swapchain(int width, int height)
{
    if (screen_swapchain != nullptr) {
        wgpuSwapChainRelease(screen_swapchain);
    }

    screen_width = width;
    screen_height = height;

    WGPUSwapChainDescriptor swap_chain_desc = {};
#ifdef __EMSCRIPTEN__
    swap_chain_desc.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding;
    swap_chain_desc.presentMode = WGPUPresentMode_Fifo;
#else
    swap_chain_desc.usage = WGPUTextureUsage_RenderAttachment;
    swap_chain_desc.presentMode = WGPUPresentMode_Mailbox;
#endif
    swap_chain_desc.format = swapchain_format;
    swap_chain_desc.width = screen_width;
    swap_chain_desc.height = screen_height;

    screen_swapchain = wgpuDeviceCreateSwapChain(device, surface, &swap_chain_desc);
}
