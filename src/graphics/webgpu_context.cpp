#include "webgpu_context.h"

#include "shader.h"
#include "pipeline.h"
#include "texture.h"
#include "renderer_storage.h"

#include "shaders/mipmaps.wgsl.gen.h"
#include "shaders/cubemap_downsampler.wgsl.gen.h"
#include "shaders/panorama_to_cubemap.wgsl.gen.h"
#include "shaders/prefilter_env.wgsl.gen.h"
#include "shaders/brdf_lut_gen.wgsl.gen.h"

#include "renderer.h"

#include "spdlog/spdlog.h"

#ifdef XR_SUPPORT
#include <dawnxr/dawnxr.h>
#endif

#ifdef __EMSCRIPTEN__
#include <GLFW/glfw3.h>
#else
#include "glfw3webgpu.h"
#endif

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
    spdlog::error("Device lost: {}", message);
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

    //assert(userData.requestEnded);

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

    this->required_limits = required_limits;

    WGPUFeatureName required_features[1] = { WGPUFeatureName_Float32Filterable };

    device_desc.requiredFeatureCount = 1;
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
    enabled_toggles.push_back("use_user_defined_labels_in_backend");
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

    process_events();

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
        cubemap_mipmap_pipeline.mipmap_shader = RendererStorage::get_shader_from_source(shaders::cubemap_downsampler::source, shaders::cubemap_downsampler::path);
        cubemap_mipmap_pipeline.mipmap_pipeline = new Pipeline();
        cubemap_mipmap_pipeline.mipmap_pipeline->create_compute(cubemap_mipmap_pipeline.mipmap_shader);
    }

    {
        panorama_to_cubemap_shader = RendererStorage::get_shader_from_source(shaders::panorama_to_cubemap::source, shaders::panorama_to_cubemap::path);

        panorama_to_cubemap_pipeline = new Pipeline();
        panorama_to_cubemap_pipeline->create_compute(panorama_to_cubemap_shader);
    }

    {
        prefiltered_env_shader = RendererStorage::get_shader_from_source(shaders::prefilter_env::source, shaders::prefilter_env::path);

        prefiltered_env_pipeline = new Pipeline();
        prefiltered_env_pipeline->create_compute(prefiltered_env_shader);
    }

    {
        brdf_lut_shader = RendererStorage::get_shader_from_source(shaders::brdf_lut_gen::source, shaders::brdf_lut_gen::path);

        brdf_lut_pipeline = new Pipeline();
        brdf_lut_pipeline->create_compute(brdf_lut_shader);

        brdf_lut_texture = new Texture();
        brdf_lut_texture->create(WGPUTextureDimension_2D, WGPUTextureFormat_RG32Float, { 512, 512, 1 },
            static_cast<WGPUTextureUsage>(WGPUTextureUsage_StorageBinding | WGPUTextureUsage_TextureBinding), 1, 1, nullptr);

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
    wgpuInstanceRelease(instance);
#else
    wgpuInstanceRelease(instance);
#endif

    wgpuSurfaceRelease(surface);
    wgpuDeviceDestroy(device);
    wgpuQueueRelease(device_queue);
    wgpuSwapChainRelease(screen_swapchain);

    for (auto &mipmap_struct : mipmap_pipelines) {
        delete mipmap_struct.second.mipmap_pipeline;
    }

    delete brdf_lut_pipeline;
    delete brdf_lut_texture;

    delete prefiltered_env_pipeline;
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
    instance = wgpuCreateInstance(&instance_dscr);

#else
    instance = wgpuCreateInstance(nullptr);
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

WGPUTexture WebGPUContext::create_texture(WGPUTextureDimension dimension, WGPUTextureFormat format, WGPUExtent3D size, WGPUTextureUsage usage, uint32_t mipmaps, uint8_t sample_count)
{
    WGPUTextureDescriptor textureDesc = {};
    textureDesc.dimension = dimension;
    textureDesc.format = format;
    textureDesc.size = size;
    textureDesc.sampleCount = sample_count;
    textureDesc.viewFormatCount = 0;
    textureDesc.viewFormats = nullptr;
    textureDesc.usage = usage;
    textureDesc.mipLevelCount = mipmaps;

    return wgpuDeviceCreateTexture(device, &textureDesc);
}

WGPUTextureView WebGPUContext::create_texture_view(WGPUTexture texture, WGPUTextureViewDimension dimension, WGPUTextureFormat format, WGPUTextureAspect aspect, uint32_t base_mip_level, uint32_t mip_level_count, uint32_t base_array_layer, uint32_t array_layer_count, const char* label)
{
    WGPUTextureViewDescriptor textureViewDesc = {};
    textureViewDesc.aspect = aspect;
    textureViewDesc.baseArrayLayer = base_array_layer;
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

void WebGPUContext::create_texture_mipmaps(WGPUTexture texture, WGPUExtent3D texture_size, uint32_t mip_level_count, WGPUTextureViewDimension view_dimension, WGPUTextureFormat format, WGPUOrigin3D origin, WGPUCommandEncoder custom_command_encoder)
{
    WGPUQueue mipmap_queue;

    if (!custom_command_encoder) {
        mipmap_queue = wgpuDeviceGetQueue(device);
    }

    sMipmapPipeline mipmap_pipeline = get_mipmap_pipeline(format);
    if (!mipmap_pipeline.mipmap_pipeline) {
        return;
    }

    // Initialize a command encoder
    WGPUCommandEncoderDescriptor encoder_desc = {};
    WGPUCommandEncoder command_encoder = custom_command_encoder ? custom_command_encoder : wgpuDeviceCreateCommandEncoder(device, &encoder_desc);

    WGPUComputePassDescriptor compute_pass_desc = {};
    compute_pass_desc.timestampWrites = nullptr;
    WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

    Uniform sampler;
    sampler.data = create_sampler(WGPUAddressMode_ClampToEdge, WGPUAddressMode_ClampToEdge, WGPUAddressMode_ClampToEdge, WGPUFilterMode_Linear, WGPUFilterMode_Linear);
    sampler.binding = 2;

    // For all layers and levels
    for (uint32_t layer = 0; layer < texture_size.depthOrArrayLayers; ++layer)
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
                create_texture_view(texture, view_dimension, format, WGPUTextureAspect_All, level, 1, layer, 1, label.c_str())
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

        mipmap_pipeline.mipmap_pipeline->set(compute_pass);

        for (uint32_t nextLevel = 1; nextLevel < texture_mip_sizes.size(); ++nextLevel) {

            Uniform source_view;
            source_view.data = texture_mip_views[nextLevel - 1];
            source_view.binding = 0;

            Uniform output_view;
            output_view.data = texture_mip_views[nextLevel];
            output_view.binding = 1;

            std::vector<Uniform*> uniforms = { &source_view, &output_view, &sampler };
            WGPUBindGroup mipmaps_bind_group = create_bind_group(uniforms, mipmap_pipeline.mipmap_shader, 0);

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

        for (WGPUTextureView texture_view : texture_mip_views) {
            wgpuTextureViewRelease(texture_view);
        }
    }

    // Finalize compute_raymarching pass
    wgpuComputePassEncoderEnd(compute_pass);

    WGPUCommandBufferDescriptor cmd_buff_descriptor = {};
    cmd_buff_descriptor.nextInChain = NULL;
    cmd_buff_descriptor.label = "Create Mipmaps Command Buffer";

    if (!custom_command_encoder) {
        // Encode and submit the GPU commands
        WGPUCommandBuffer commands = wgpuCommandEncoderFinish(command_encoder, &cmd_buff_descriptor);
        wgpuQueueSubmit(mipmap_queue, 1, &commands);

        wgpuCommandBufferRelease(commands);
        wgpuCommandEncoderRelease(command_encoder);
    }

    wgpuComputePassEncoderRelease(compute_pass);

    if (!custom_command_encoder) {
        wgpuQueueRelease(mipmap_queue);
    }
}

void WebGPUContext::create_cubemap_mipmaps(WGPUTexture texture, WGPUExtent3D texture_size, uint32_t mip_level_count, WGPUTextureViewDimension view_dimension, WGPUTextureFormat format, WGPUOrigin3D origin, WGPUCommandEncoder custom_command_encoder)
{
    WGPUQueue mipmap_queue;

    if (!custom_command_encoder) {
        mipmap_queue = wgpuDeviceGetQueue(device);
    }

    // Initialize a command encoder
    WGPUCommandEncoderDescriptor encoder_desc = {};
    WGPUCommandEncoder command_encoder = custom_command_encoder ? custom_command_encoder : wgpuDeviceCreateCommandEncoder(device, &encoder_desc);

    WGPUComputePassDescriptor compute_pass_desc = {};
    compute_pass_desc.timestampWrites = nullptr;
    WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

    Uniform sampler;
    sampler.data = create_sampler(WGPUAddressMode_ClampToEdge, WGPUAddressMode_ClampToEdge, WGPUAddressMode_ClampToEdge, WGPUFilterMode_Linear, WGPUFilterMode_Linear);
    sampler.binding = 2;

    // For all levels
    std::vector<WGPUExtent3D> texture_mip_sizes;
    texture_mip_sizes.resize(mip_level_count);
    texture_mip_sizes[0] = texture_size;

    std::vector<WGPUTextureView> texture_mip_views_read;
    texture_mip_views_read.reserve(texture_mip_sizes.size());

    std::vector<WGPUTextureView> texture_mip_views_write;
    texture_mip_views_write.reserve(texture_mip_sizes.size());

    for (uint32_t level = 0; level < texture_mip_sizes.size(); ++level)
    {
        std::string label = "MIP level #" + std::to_string(level);
        texture_mip_views_read.push_back(
            create_texture_view(texture, view_dimension, format, WGPUTextureAspect_All, level, 1, 0, 6, label.c_str())
        );

        texture_mip_views_write.push_back(
            create_texture_view(texture, WGPUTextureViewDimension_2DArray, format, WGPUTextureAspect_All, level, 1, 0, 6, label.c_str())
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

    cubemap_mipmap_pipeline.mipmap_pipeline->set(compute_pass);

    for (uint32_t nextLevel = 1; nextLevel < texture_mip_sizes.size(); ++nextLevel) {

        Uniform source_view;
        source_view.data = texture_mip_views_read[nextLevel - 1];
        source_view.binding = 0;

        Uniform output_view;
        output_view.data = texture_mip_views_write[nextLevel];
        output_view.binding = 1;

        std::vector<Uniform*> uniforms = { &source_view, &output_view, &sampler };
        WGPUBindGroup mipmaps_bind_group = create_bind_group(uniforms, cubemap_mipmap_pipeline.mipmap_shader, 0);

        wgpuComputePassEncoderSetBindGroup(compute_pass, 0, mipmaps_bind_group, 0, nullptr);

        uint32_t invocationCountX = texture_mip_sizes[nextLevel].width;
        uint32_t invocationCountY = texture_mip_sizes[nextLevel].height;
        uint32_t workgroupSizePerDim = 8;
        // This ceils invocationCountX / workgroupSizePerDim
        uint32_t workgroupCountX = (invocationCountX + workgroupSizePerDim - 1) / workgroupSizePerDim;
        uint32_t workgroupCountY = (invocationCountY + workgroupSizePerDim - 1) / workgroupSizePerDim;
        wgpuComputePassEncoderDispatchWorkgroups(compute_pass, workgroupCountX, workgroupCountY, 6);

        wgpuBindGroupRelease(mipmaps_bind_group);
    }

    for (WGPUTextureView texture_view : texture_mip_views_read) {
        wgpuTextureViewRelease(texture_view);
    }

    for (WGPUTextureView texture_view : texture_mip_views_write) {
        wgpuTextureViewRelease(texture_view);
    }

    // Finalize compute_raymarching pass
    wgpuComputePassEncoderEnd(compute_pass);

    WGPUCommandBufferDescriptor cmd_buff_descriptor = {};
    cmd_buff_descriptor.nextInChain = NULL;
    cmd_buff_descriptor.label = "Create Mipmaps Command Buffer";

    if (!custom_command_encoder) {
        // Encode and submit the GPU commands
        WGPUCommandBuffer commands = wgpuCommandEncoderFinish(command_encoder, &cmd_buff_descriptor);
        wgpuQueueSubmit(mipmap_queue, 1, &commands);

        wgpuCommandBufferRelease(commands);
        wgpuCommandEncoderRelease(command_encoder);
    }

    wgpuComputePassEncoderRelease(compute_pass);

    if (!custom_command_encoder) {
        wgpuQueueRelease(mipmap_queue);
    }
}

void WebGPUContext::upload_texture(WGPUTexture texture, WGPUExtent3D texture_size, uint32_t mip_level, WGPUTextureFormat format, const void* data, WGPUOrigin3D origin)
{
    WGPUQueue mipmap_queue = wgpuDeviceGetQueue(device);

    WGPUImageCopyTexture destination = {};
    destination.texture = texture;
    destination.origin = origin;
    destination.aspect = WGPUTextureAspect_All;
    destination.mipLevel = 0;

    WGPUTextureDataLayout source = {};
    source.offset = 0;

    size_t byte_size = 0;
    uint32_t pixel_size = 0;

    switch (format) {
    case WGPUTextureFormat_RGBA8Unorm:
        pixel_size = 4 * sizeof(uint8_t);
        break;
    case WGPUTextureFormat_RGBA8UnormSrgb:
        pixel_size = 4 * sizeof(uint8_t);
        break;
    case WGPUTextureFormat_RGBA16Uint:
        pixel_size = 4 * sizeof(uint16_t);
        break;
    case WGPUTextureFormat_RGBA32Float:
        pixel_size = 4 * sizeof(float);
        break;
    default:
        assert(false);
    }

    source.bytesPerRow = pixel_size * texture_size.width;
    byte_size = source.bytesPerRow * texture_size.height;

    source.rowsPerImage = texture_size.height;

    // Copy data to the selected mipmap
    wgpuQueueWriteTexture(mipmap_queue, &destination, data, byte_size, &source, &texture_size);

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

    if (layouts_by_id.size() <= bind_group) {
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

WebGPUContext::sMipmapPipeline WebGPUContext::get_mipmap_pipeline(WGPUTextureFormat texture_format)
{
    if (mipmap_pipelines.contains(texture_format)) {
        return mipmap_pipelines[texture_format];
    }

    std::string custom_define;

    switch (texture_format) {
    case WGPUTextureFormat_RGBA8Unorm:
    case WGPUTextureFormat_RGBA8UnormSrgb:
        custom_define = "RGBA8_UNORM";
        break;
    case WGPUTextureFormat_RGBA32Float:
        custom_define = "RGBA32_FLOAT";
        break;
    default:
        assert(false);
    }

    Shader* shader = RendererStorage::get_shader_from_source(shaders::mipmaps::source, shaders::mipmaps::path, { custom_define });

    Pipeline* pipeline = new Pipeline();
    pipeline->create_compute(shader);

    sMipmapPipeline mipmap_pipeline = { pipeline, shader };
    mipmap_pipelines[texture_format] = mipmap_pipeline;

    return mipmap_pipeline;
}

void WebGPUContext::process_events()
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

void WebGPUContext::copy_texture_to_texture(WGPUTexture texture_src, WGPUTexture texture_dst, uint32_t src_mipmap_level, uint32_t dst_mipmap_level, const WGPUExtent3D& copy_size, WGPUCommandEncoder custom_command_encoder)
{
    WGPUImageCopyTexture src_copy = {};
    src_copy.texture = texture_src;
    src_copy.mipLevel = src_mipmap_level;
    src_copy.aspect = WGPUTextureAspect_All;

    WGPUImageCopyTexture dst_copy = {};
    dst_copy.texture = texture_dst;
    dst_copy.mipLevel = dst_mipmap_level;
    dst_copy.aspect = WGPUTextureAspect_All;

    WGPUQueue copy_queue;

    if (!custom_command_encoder) {
        copy_queue = wgpuDeviceGetQueue(device);
    }

    // Initialize a command encoder
    WGPUCommandEncoderDescriptor encoder_desc = {};
    WGPUCommandEncoder command_encoder = custom_command_encoder ? custom_command_encoder : wgpuDeviceCreateCommandEncoder(device, &encoder_desc);

    wgpuCommandEncoderCopyTextureToTexture(command_encoder, &src_copy, &dst_copy, &copy_size);

    if (!custom_command_encoder) {
        WGPUCommandBufferDescriptor cmd_buff_descriptor = {};
        cmd_buff_descriptor.nextInChain = NULL;
        cmd_buff_descriptor.label = "Copy texture Command Buffer";

        // Encode and submit the GPU commands
        WGPUCommandBuffer commands = wgpuCommandEncoderFinish(command_encoder, &cmd_buff_descriptor);
        wgpuQueueSubmit(copy_queue, 1, &commands);

        wgpuCommandBufferRelease(commands);
        wgpuCommandEncoderRelease(command_encoder);
    }
}

WGPURenderPipeline WebGPUContext::create_render_pipeline(WGPUShaderModule render_shader_module, WGPUPipelineLayout pipeline_layout, const std::vector<WGPUVertexBufferLayout>& vertex_attributes,
    WGPUColorTargetState color_target, bool depth_read, bool depth_write, WGPUCullMode cull_mode, WGPUPrimitiveTopology topology, uint8_t sample_count)
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
    depth_state.depthCompare = depth_read ? WGPUCompareFunction_Less : WGPUCompareFunction_Always;
    depth_state.depthWriteEnabled = depth_write;
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

    pipeline_descr.depthStencil = &depth_state;
    pipeline_descr.multisample = {
            .count = sample_count,
            .mask = ~0u,
            .alphaToCoverageEnabled = false
    };

    pipeline_descr.fragment = &fragment_state;
 
    return wgpuDeviceCreateRenderPipeline(device, &pipeline_descr);
}

WGPUComputePipeline WebGPUContext::create_compute_pipeline(WGPUShaderModule compute_shader_module, WGPUPipelineLayout pipeline_layout)
{
    WGPUComputePipelineDescriptor computePipelineDesc = {};
    computePipelineDesc.compute.nextInChain = nullptr;
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

    wgpuComputePassEncoderDispatchWorkgroups(compute_pass, 32, 32, 1);

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

void WebGPUContext::generate_prefiltered_env_texture(Texture* prefiltered_env_texture, Texture* hdr_texture)
{
    WGPUQueue prefilter_queue = wgpuDeviceGetQueue(device);

    // temporal texture to store panorama to cubemap result and to generate mipmaps
    Texture cubemap_texture;
    cubemap_texture.create(WGPUTextureDimension_2D, WGPUTextureFormat_RGBA32Float, { ENVIRONMENT_RESOLUTION, ENVIRONMENT_RESOLUTION, 6 },
        static_cast<WGPUTextureUsage>(WGPUTextureUsage_StorageBinding | WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc), 6, 1, nullptr);

    Uniform cubemap_mipmaps_uniforms[5];
    for (int i = 0; i < 5; ++i) {
        cubemap_mipmaps_uniforms[i].data = prefiltered_env_texture->get_view(WGPUTextureViewDimension_2DArray, i + 1, 1, 0, 6);
        cubemap_mipmaps_uniforms[i].binding = 1;
    }

    Uniform cubemap_all_faces_output_uniform;
    cubemap_all_faces_output_uniform.data = cubemap_texture.get_view(WGPUTextureViewDimension_2DArray, 0, 1, 0, 6);
    cubemap_all_faces_output_uniform.binding = 1;

    Uniform cubemap_all_input_output_uniform;
    cubemap_all_input_output_uniform.data = cubemap_texture.get_view(WGPUTextureViewDimension_Cube, 0, 6, 0, 6);
    cubemap_all_input_output_uniform.binding = 0;

    Uniform sampler;
    sampler.data = create_sampler(WGPUAddressMode_ClampToEdge, WGPUAddressMode_ClampToEdge, WGPUAddressMode_ClampToEdge, WGPUFilterMode_Linear, WGPUFilterMode_Linear, WGPUMipmapFilterMode_Linear, 6.0f);
    sampler.binding = 2;

    Uniform hdr_uniform;
    hdr_uniform.data = hdr_texture->get_view();
    hdr_uniform.binding = 0;

    struct PrefilterEnvUniformData {
        uint32_t current_mip_level;
        uint32_t mip_level_count;
        uint32_t pad0;
        uint32_t pad1;
    } prefilter_env_uniform_data;

    uint32_t buffer_stride = std::max(static_cast<uint32_t>(sizeof(PrefilterEnvUniformData)), required_limits.limits.minUniformBufferOffsetAlignment);

    Uniform current_level_uniform;
    current_level_uniform.data = create_buffer(buffer_stride * 6, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, nullptr, "previlter env uniform_data");
    current_level_uniform.buffer_size = sizeof(PrefilterEnvUniformData);
    current_level_uniform.binding = 3;

    // Initialize a command encoder
    WGPUCommandEncoderDescriptor encoder_desc = {};
    WGPUCommandEncoder command_encoder = wgpuDeviceCreateCommandEncoder(device, &encoder_desc);

    // Panorama to cubemap
    {
        std::vector<Uniform*> uniforms = { &hdr_uniform, &cubemap_all_faces_output_uniform, &sampler };
        WGPUBindGroup bind_group = create_bind_group(uniforms, panorama_to_cubemap_shader, 0);

        WGPUComputePassDescriptor compute_pass_desc = {};
        compute_pass_desc.timestampWrites = nullptr;
        WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

        panorama_to_cubemap_pipeline->set(compute_pass);

        uint32_t invocationCountX = cubemap_texture.get_width();
        uint32_t invocationCountY = cubemap_texture.get_height();
        uint32_t workgroupSizePerDim = 4;

        uint32_t workgroupCountX = (invocationCountX + workgroupSizePerDim - 1) / workgroupSizePerDim;
        uint32_t workgroupCountY = (invocationCountY + workgroupSizePerDim - 1) / workgroupSizePerDim;

        wgpuComputePassEncoderSetBindGroup(compute_pass, 0, bind_group, 0, nullptr);

        wgpuComputePassEncoderDispatchWorkgroups(compute_pass, workgroupCountX, workgroupCountY, 1);

        wgpuBindGroupRelease(bind_group);

        // Finalize compute_raymarching pass
        wgpuComputePassEncoderEnd(compute_pass);

        wgpuComputePassEncoderRelease(compute_pass);
    }

    create_cubemap_mipmaps(cubemap_texture.get_texture(), cubemap_texture.get_size(), 6, WGPUTextureViewDimension_Cube, cubemap_texture.get_format(), { 0, 0, 0 }, command_encoder);

    // copy first cubemap mipmap to final texture
    copy_texture_to_texture(cubemap_texture.get_texture(), prefiltered_env_texture->get_texture(), 0, 0, cubemap_texture.get_size(), command_encoder);

    // Prefilter cubemap
    {
        WGPUBindGroup bind_groups[5];

        for (uint32_t i = 0; i < 5; ++i) {
            std::vector<Uniform*> uniforms = { &cubemap_all_input_output_uniform, &cubemap_mipmaps_uniforms[i], &sampler, &current_level_uniform };
            bind_groups[i] = create_bind_group(uniforms, prefiltered_env_shader, 0);
        }

        prefilter_env_uniform_data.mip_level_count = 6;

        // Setup uniform buffers
        for (uint32_t i = 0; i < 5; ++i) {
            prefilter_env_uniform_data.current_mip_level = i + 1;
            wgpuQueueWriteBuffer(prefilter_queue, std::get<WGPUBuffer>(current_level_uniform.data), i * buffer_stride, &prefilter_env_uniform_data, sizeof(PrefilterEnvUniformData));
        }

        WGPUComputePassDescriptor compute_pass_desc = {};
        compute_pass_desc.timestampWrites = nullptr;
        WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

        prefiltered_env_pipeline->set(compute_pass);

        uint32_t invocationCountX = prefiltered_env_texture->get_width();
        uint32_t invocationCountY = prefiltered_env_texture->get_height();
        uint32_t workgroupSizePerDim = 4;

        uint32_t dynamicOffset = 0;

        for (uint32_t i = 0; i < 5; ++i) {

            dynamicOffset = i * buffer_stride;

            wgpuComputePassEncoderSetBindGroup(compute_pass, 0, bind_groups[i], 1, &dynamicOffset);

            invocationCountX = invocationCountX / 2;
            invocationCountY = invocationCountY / 2;
            uint32_t workgroupCountX = (invocationCountX + workgroupSizePerDim - 1) / workgroupSizePerDim;
            uint32_t workgroupCountY = (invocationCountY + workgroupSizePerDim - 1) / workgroupSizePerDim;

            wgpuComputePassEncoderDispatchWorkgroups(compute_pass, workgroupCountX, workgroupCountY, 1);
        }

        for (uint32_t i = 0; i < 5; ++i) {
            wgpuBindGroupRelease(bind_groups[i]);
        }

        // Finalize compute_raymarching pass
        wgpuComputePassEncoderEnd(compute_pass);

        wgpuComputePassEncoderRelease(compute_pass);
    }

    WGPUCommandBufferDescriptor cmd_buff_descriptor = {};
    cmd_buff_descriptor.nextInChain = NULL;
    cmd_buff_descriptor.label = "Create Prefiltered Env Command Buffer";

    // Encode and submit the GPU commands
    WGPUCommandBuffer commands = wgpuCommandEncoderFinish(command_encoder, &cmd_buff_descriptor);
    wgpuQueueSubmit(prefilter_queue, 1, &commands);

    wgpuCommandBufferRelease(commands);
    wgpuCommandEncoderRelease(command_encoder);

    wgpuQueueRelease(prefilter_queue);
}

void WebGPUContext::update_buffer(WGPUBuffer buffer, uint64_t buffer_offset, void const* data, uint64_t size)
{
    wgpuQueueWriteBuffer(device_queue, buffer, buffer_offset, data, size);
}

WGPUInstance WebGPUContext::get_instance()
{
    return instance;
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
