#include "webgpu_context.h"
#include "utils.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5_webgpu.h>
#else
#include "glfw3webgpu.h"
#endif

#include "shader.h"
#include "pipeline.h"
#include "texture.h"

#include "renderer.h"

#include "spdlog/spdlog.h"

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

    spdlog::error("{} error: {}", errorTypeName, message);
    // assert(0);
}

void DeviceLostCallback(WGPUDeviceLostReason reason, const char* message, void*) {
    spdlog::error("Device lost: {}");
}

void PrintGLFWError(int code, const char* message) {
    spdlog::error("GLFW error: {} - {}", code, message);
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
            spdlog::error("Could not get WebGPU adapter: {}", message);
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
            spdlog::error("Could not get WebGPU device: {}", message);
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

    WGPUFeatureName required_features[1] = { WGPUFeatureName_Float32Filterable };

#ifdef __EMSCRIPTEN__
    device_desc.requiredFeaturesCount = 1;
#else
    device_desc.requiredFeatureCount = 1;
#endif

    device_desc.requiredFeatures = required_features;

    device_desc.requiredLimits = &required_limits;
    device_desc.defaultQueue.label = "The default queue";
    device_desc.deviceLostCallback = DeviceLostCallback;

#if !defined(__EMSCRIPTEN__)
    std::vector<const char*> enabled_toggles;
    std::vector<const char*> disabled_toggles;

    disabled_toggles.push_back("lazy_clear_resource_on_first_use");

    enabled_toggles.push_back("use_dxc");

#ifdef _DEBUG
    enabled_toggles.push_back("disable_symbol_renaming");
    enabled_toggles.push_back("emit_hlsl_debug_symbols");
#endif

    WGPUDawnTogglesDescriptor device_toggles_desc = {};
    device_toggles_desc.enabledToggles = enabled_toggles.data();
    device_toggles_desc.enabledToggleCount = enabled_toggles.size();

    device_toggles_desc.disabledToggles = disabled_toggles.data();
    device_toggles_desc.disabledToggleCount = disabled_toggles.size();

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

    this->window = window;

    device_queue = wgpuDeviceGetQueue(device);

    {
        mipmaps_shader = RendererStorage::get_shader("data/shaders/mipmaps.wgsl");

        mipmaps_pipeline = new Pipeline();
        mipmaps_pipeline->create_compute(mipmaps_shader);
    }

    {
        brdf_lut_shader = RendererStorage::get_shader("data/shaders/brdf_lut_gen.wgsl");

        brdf_lut_pipeline = new Pipeline();
        brdf_lut_pipeline->create_compute(brdf_lut_shader);

        brdf_lut_texture = new Texture();
        brdf_lut_texture->create(WGPUTextureDimension_2D, WGPUTextureFormat_RG32Float, { 512, 512, 1 },
            static_cast<WGPUTextureUsage>(WGPUTextureUsage_StorageBinding | WGPUTextureUsage_TextureBinding), 1, nullptr);

        generate_brdf_lut_texture();
    }


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

    delete mipmaps_pipeline;
    delete brdf_lut_pipeline;
    delete brdf_lut_texture;
}

void WebGPUContext::create_instance()
{
    WGPUInstanceDescriptor instance_dscr = {};

#if !defined(__EMSCRIPTEN__)

    // allow_unsafe_apis is currently required to prevent forced breakpoint hit when DAWN_DEBUG_BREAK_ON_ERROR=1
    const char* const enabled_toggles[] = { "allow_unsafe_apis" };

    WGPUDawnTogglesDescriptor device_toggles_desc = {};
    device_toggles_desc.enabledToggles = enabled_toggles;
    device_toggles_desc.enabledToggleCount = 1;

    WGPUChainedStruct* chain_desc = reinterpret_cast<WGPUChainedStruct*>(&device_toggles_desc);
    chain_desc->sType = WGPUSType_DawnTogglesDescriptor;
    instance_dscr.nextInChain = chain_desc;

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

WGPUTextureView WebGPUContext::create_texture_view(WGPUTexture texture, WGPUTextureViewDimension dimension, WGPUTextureFormat format, WGPUTextureAspect aspect, uint32_t mip_level_count, uint32_t base_mip_level, uint32_t array_layer_count, const char* label)
{
    WGPUTextureViewDescriptor textureViewDesc = {};
    textureViewDesc.aspect = aspect;
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.arrayLayerCount = array_layer_count;
    textureViewDesc.dimension = dimension;
    textureViewDesc.format = format;
    textureViewDesc.mipLevelCount = mip_level_count;
    textureViewDesc.baseMipLevel = base_mip_level;
    textureViewDesc.label = label;

    return wgpuTextureCreateView(texture, &textureViewDesc);
}

WGPUSampler WebGPUContext::create_sampler(WGPUAddressMode wrap_u, WGPUAddressMode wrap_v, WGPUAddressMode wrap_w, WGPUFilterMode mag_filter, WGPUFilterMode min_filter, WGPUMipmapFilterMode mipmap_filter, float lod_max_clamp, uint16_t max_anisotropy)
{
    WGPUSamplerDescriptor samplerDesc = {};
    samplerDesc.addressModeU = wrap_u;
    samplerDesc.addressModeV = wrap_v;
    samplerDesc.addressModeW = wrap_w;
    samplerDesc.magFilter = mag_filter;
    samplerDesc.minFilter = min_filter;
    samplerDesc.mipmapFilter = mipmap_filter;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = lod_max_clamp;
    samplerDesc.compare = WGPUCompareFunction_Undefined;
    samplerDesc.maxAnisotropy = max_anisotropy;

    return wgpuDeviceCreateSampler(device, &samplerDesc);
}

void WebGPUContext::create_texture_mipmaps(WGPUTexture texture, WGPUExtent3D texture_size, uint32_t mip_level_count, const void* data, WGPUTextureViewDimension view_dimension, WGPUTextureFormat format, WGPUOrigin3D origin)
{
    WGPUQueue mipmap_queue = wgpuDeviceGetQueue(device);

    WGPUImageCopyTexture destination = {};
    destination.texture = texture;
    destination.origin = origin;
    destination.aspect = WGPUTextureAspect_All;
    destination.mipLevel = 0;

    WGPUTextureDataLayout source = {};
    source.offset = 0;
    source.bytesPerRow = 4 * texture_size.width;
    source.rowsPerImage = texture_size.height;

    // For level 0
    wgpuQueueWriteTexture(mipmap_queue, &destination, data, (4 * texture_size.width * texture_size.height), &source, &texture_size);

    // For ALL levels
    {
        std::vector<WGPUExtent3D> texture_mip_sizes;
        texture_mip_sizes.resize(mip_level_count);
        texture_mip_sizes[0] = texture_size;

        std::vector<WGPUTextureView> texture_mip_views;
        texture_mip_views.reserve(texture_mip_sizes.size());

        for (uint32_t level = 0; level < texture_mip_sizes.size(); ++level)
        {
            std::string label = "MIP level #" + std::to_string(level);
            texture_mip_views.push_back(
                create_texture_view(texture, view_dimension, format, WGPUTextureAspect_All, 1, level, texture_size.depthOrArrayLayers, label.c_str())
            );

            if (level > 0) {
                WGPUExtent3D previous_size = texture_mip_sizes[level - 1];
                texture_mip_sizes[level] = {
                    previous_size.width / 2,
                    previous_size.height / 2,
                    previous_size.depthOrArrayLayers / 2
                };
            }
        }

        // Initialize a command encoder
        WGPUCommandEncoderDescriptor encoder_desc = {};
        WGPUCommandEncoder command_encoder = wgpuDeviceCreateCommandEncoder(device, &encoder_desc);

        WGPUComputePassDescriptor compute_pass_desc = {};
        compute_pass_desc.timestampWrites = nullptr;
        WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

        mipmaps_pipeline->set(compute_pass);

        for (uint32_t nextLevel = 1; nextLevel < texture_mip_sizes.size(); ++nextLevel) {

            Uniform source_view;
            source_view.data = texture_mip_views[nextLevel - 1];
            source_view.binding = 0;

            Uniform output_view;
            output_view.data = texture_mip_views[nextLevel];
            output_view.binding = 1;

            std::vector<Uniform*> uniforms = { &source_view, &output_view };
            WGPUBindGroup mipmaps_bind_group = create_bind_group(uniforms, mipmaps_shader, 0);

            wgpuComputePassEncoderSetBindGroup(compute_pass, 0, mipmaps_bind_group, 0, nullptr);

            uint32_t invocationCountX = texture_mip_sizes[nextLevel].width;
            uint32_t invocationCountY = texture_mip_sizes[nextLevel].height;
            uint32_t workgroupSizePerDim = 8;
            // This ceils invocationCountX / workgroupSizePerDim
            uint32_t workgroupCountX = (invocationCountX + workgroupSizePerDim - 1) / workgroupSizePerDim;
            uint32_t workgroupCountY = (invocationCountY + workgroupSizePerDim - 1) / workgroupSizePerDim;
            wgpuComputePassEncoderDispatchWorkgroups(compute_pass, workgroupCountX, workgroupCountY, 1);

            wgpuBindGroupRelease(mipmaps_bind_group);
        }

        // Finalize compute_raymarching pass
        wgpuComputePassEncoderEnd(compute_pass);

        WGPUCommandBufferDescriptor cmd_buff_descriptor = {};
        cmd_buff_descriptor.nextInChain = NULL;
        cmd_buff_descriptor.label = "Create Mipmaps Command Buffer";

        // Encode and submit the GPU commands
        WGPUCommandBuffer commands = wgpuCommandEncoderFinish(command_encoder, &cmd_buff_descriptor);
        wgpuQueueSubmit(mipmap_queue, 1, &commands);

        wgpuCommandBufferRelease(commands);
        wgpuComputePassEncoderRelease(compute_pass);
        wgpuCommandEncoderRelease(command_encoder);
    }

    wgpuQueueRelease(mipmap_queue);
}

void WebGPUContext::upload_texture_mipmaps(WGPUTexture texture, WGPUExtent3D texture_size, uint32_t mip_level, const void* data, WGPUOrigin3D origin)
{
    WGPUQueue mipmap_queue = wgpuDeviceGetQueue(device);

    WGPUImageCopyTexture destination = {};
    destination.texture = texture;
    destination.origin = origin;
    destination.mipLevel = mip_level;
    destination.aspect = WGPUTextureAspect_All;

    WGPUExtent3D layer_size = { texture_size.width, texture_size.height, 1 };
    uint32_t channel_size = sizeof(float); // hardcoded by now...

    WGPUTextureDataLayout source = {};
    source.offset = 0;
    source.bytesPerRow = channel_size * 4 * texture_size.width;
    source.rowsPerImage = texture_size.height;

    // Copy data to the selected mipmap
    wgpuQueueWriteTexture(mipmap_queue, &destination, data, (channel_size * 4 * texture_size.width * texture_size.height), &source, &layer_size);

    wgpuQueueRelease(mipmap_queue);
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
    spdlog::trace("Creating bind group {} for shader {}", bind_group, shader->get_path());

    std::vector<WGPUBindGroupEntry> entries(uniforms.size());

    for (int i = 0; i < uniforms.size(); ++i) {
        entries[i] = uniforms[i]->get_bind_group_entry();
    }

    std::vector<WGPUBindGroupLayout>& layouts_by_id = shader->get_bind_group_layouts();

    if (layouts_by_id.size() < bind_group) {
        spdlog::error("Can't find bind group {} in shader: {}", bind_group, shader->get_path());
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

WGPURenderPipeline WebGPUContext::create_render_pipeline(WGPUShaderModule render_shader_module, WGPUPipelineLayout pipeline_layout, const std::vector<WGPUVertexBufferLayout>& vertex_attributes, WGPUColorTargetState color_target, bool uses_depth_buffer, bool uses_depth_write, WGPUCullMode cull_mode, WGPUPrimitiveTopology topology)
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
        depth_state.depthWriteEnabled = uses_depth_write;
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
        .topology = topology,
        .stripIndexFormat = WGPUIndexFormat_Undefined, // order of the connected vertices
        .frontFace = WGPUFrontFace_CCW,
        .cullMode = cull_mode
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

void WebGPUContext::generate_brdf_lut_texture()
{
    Uniform brdf_lut_uniform;
    brdf_lut_uniform.data = brdf_lut_texture->get_view();
    brdf_lut_uniform.binding = 0;

    std::vector<Uniform*> uniforms = { &brdf_lut_uniform };
    WGPUBindGroup bind_group = create_bind_group(uniforms, brdf_lut_shader, 0);

    WGPUQueue brdf_queue = wgpuDeviceGetQueue(device);

    // Initialize a command encoder
    WGPUCommandEncoderDescriptor encoder_desc = {};
    WGPUCommandEncoder command_encoder = wgpuDeviceCreateCommandEncoder(device, &encoder_desc);

    WGPUComputePassDescriptor compute_pass_desc = {};
    compute_pass_desc.timestampWrites = nullptr;
    WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

    brdf_lut_pipeline->set(compute_pass);

    wgpuComputePassEncoderSetBindGroup(compute_pass, 0, bind_group, 0, nullptr);

    wgpuComputePassEncoderDispatchWorkgroups(compute_pass, 16, 16, 1);

    wgpuBindGroupRelease(bind_group);

    // Finalize compute_raymarching pass
    wgpuComputePassEncoderEnd(compute_pass);

    WGPUCommandBufferDescriptor cmd_buff_descriptor = {};
    cmd_buff_descriptor.nextInChain = NULL;
    cmd_buff_descriptor.label = "Create BRDF Command Buffer";

    // Encode and submit the GPU commands
    WGPUCommandBuffer commands = wgpuCommandEncoderFinish(command_encoder, &cmd_buff_descriptor);
    wgpuQueueSubmit(brdf_queue, 1, &commands);

    wgpuCommandBufferRelease(commands);
    wgpuComputePassEncoderRelease(compute_pass);
    wgpuCommandEncoderRelease(command_encoder);
    

    wgpuQueueRelease(brdf_queue);
}

void WebGPUContext::update_buffer(WGPUBuffer buffer, uint64_t buffer_offset, void const* data, uint64_t size)
{
    wgpuQueueWriteBuffer(device_queue, buffer, buffer_offset, data, size);
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
