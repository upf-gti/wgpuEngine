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

#include "framework/utils/utils.h"

#include "renderer.h"

#include "spdlog/spdlog.h"

#ifdef OPENXR_SUPPORT
#include "xr/dawnxr/dawnxr.h"
#include "xr/dawnxr/dawnxr_internal.h"

#include "xr/openxr/openxr_context.h"

#include "graphics/backend_include.h"

#endif

#ifdef __EMSCRIPTEN__
#include <GLFW/glfw3.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#else
#include "glfw3webgpu.hpp"
#endif

WGPUTextureFormat WebGPUContext::swapchain_format = WGPUTextureFormat_BGRA8Unorm;

// TODO: this should be queried
#if defined(OPENXR_SUPPORT)
WGPUTextureFormat WebGPUContext::xr_swapchain_format = WGPUTextureFormat_BGRA8UnormSrgb;
#elif defined(WEBXR_SUPPORT)
WGPUTextureFormat WebGPUContext::xr_swapchain_format = WGPUTextureFormat_BGRA8Unorm;
#else
WGPUTextureFormat WebGPUContext::xr_swapchain_format = WGPUTextureFormat_Undefined;
#endif

WGPUStringView get_string_view(const char* str)
{
    return { str, strlen(str) };
}

void PrintDeviceError(WGPUDevice const* device, WGPUErrorType type, struct WGPUStringView message, void* userdata1, void* userdata2) {
    const char* errorTypeName = "";
    switch (type) {
    case WGPUErrorType_Validation:
        errorTypeName = "Validation";
        break;
    case WGPUErrorType_OutOfMemory:
        errorTypeName = "Out of memory";
        break;
    case WGPUErrorType_Unknown:
        errorTypeName = "Unknown";
        break;
    case WGPUErrorType_Internal:
        errorTypeName = "Internal";
        break;
    default:
        return;
    }

    spdlog::error("{} error: {}", errorTypeName, message.data);
    // assert(0);
}

void DeviceLostCallback(WGPUDevice const* device, WGPUDeviceLostReason reason, struct WGPUStringView message, void* userdata1, void* userdata2) {
    if (reason != WGPUDeviceLostReason_Destroyed) {
        spdlog::error("Device lost: {}", message.data);
    }
}

void PrintGLFWError(int code, const char* message) {
    spdlog::error("GLFW error: {} - {}", code, message);
}

WGPUFuture WebGPUContext::request_adapter(XRContext* xr_context, bool is_openxr_available)
{
    WGPURequestAdapterOptions adapter_opts = {};

    // To choose dedicated GPU on laptops
    adapter_opts.powerPreference = WGPUPowerPreference_HighPerformance;
    adapter_opts.featureLevel = WGPUFeatureLevel_Core;

#ifdef WEBXR_SUPPORT
    WGPURequestAdapterWebXROptions webxr_options = {};
    webxr_options.xrCompatible = true;
    webxr_options.chain.next = nullptr;
    webxr_options.chain.sType = WGPUSType_RequestAdapterWebXROptions;
    adapter_opts.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&webxr_options);
#endif

#ifdef OPENXR_SUPPORT

    OpenXRContext* openxr_context = static_cast<OpenXRContext*>(xr_context);

#if defined(BACKEND_DX12)
    adapter_opts.backendType = WGPUBackendType_D3D12;
#elif defined(BACKEND_VULKAN)
    dawn::native::vulkan::RequestAdapterOptionsOpenXRConfig adapter_opts_xr_config = {};
    adapter_opts.backendType = WGPUBackendType_Vulkan;
#endif

    // Create internal vulkan instance
    if (is_openxr_available) {

#if defined(BACKEND_VULKAN)
        dawnxr::internal::createVulkanOpenXRConfig(openxr_context->instance, openxr_context->system_id, (void**)&adapter_opts_xr_config.openXRConfig);
        adapter_opts.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&adapter_opts_xr_config);
#endif
    }
    else {
        spdlog::warn("XR not available, fallback to desktop mode");
    }
#endif // OPENXR_SUPPORT

    // request adapter
    WGPURequestAdapterCallbackInfo adapter_callback_info = {};
    adapter_callback_info.mode = WGPUCallbackMode_AllowProcessEvents;
    adapter_callback_info.userdata1 = &adapter;
    adapter_callback_info.callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* userdata1, void* userdata2) {
        WGPUAdapter* out_adapter = reinterpret_cast<WGPUAdapter*>(userdata1);
        if (status == WGPURequestAdapterStatus_Success) {
            *out_adapter = adapter;
        }
        else {
            std::string error_str = "";
            switch (status) {
            case WGPURequestAdapterStatus_Error:
                error_str += "Error";
                break;
            case WGPURequestAdapterStatus_CallbackCancelled:
                error_str += "Callback Cancelled";
                break;
            case WGPURequestAdapterStatus_Unavailable:
                error_str += "Unavailable";
                break;
            default:
                error_str += "Unknown";
                break;
            }

            spdlog::error("Failed to get WebGPU adapter with \"{}\" status: {}", error_str, message.data);
        }
    };

    // Call to the WebGPU request adapter procedure
    return wgpuInstanceRequestAdapter(
        instance,
        &adapter_opts,
        adapter_callback_info
    );
}

WGPUFuture WebGPUContext::request_device(const std::vector<WGPUFeatureName> required_features)
{
    // Create device
    WGPUDeviceDescriptor device_desc = {};
    device_desc.label = get_string_view("My Device");

    device_desc.requiredFeatureCount = required_features.size();
    device_desc.requiredFeatures = required_features.data();

    device_desc.requiredLimits = &required_limits;
    device_desc.defaultQueue.label = get_string_view("The default queue");

    device_desc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    device_desc.deviceLostCallbackInfo.callback = DeviceLostCallback;
    device_desc.uncapturedErrorCallbackInfo.callback = PrintDeviceError;

#if !defined(__EMSCRIPTEN__)

    std::vector<const char*> enabled_toggles;
    std::vector<const char*> disabled_toggles;

    disabled_toggles.push_back("lazy_clear_resource_on_first_use");

    enabled_toggles.push_back("use_dxc");

#ifdef _DEBUG
    enabled_toggles.push_back("disable_symbol_renaming");
    enabled_toggles.push_back("emit_hlsl_debug_symbols");
    enabled_toggles.push_back("use_user_defined_labels_in_backend");
#else
    enabled_toggles.push_back("skip_validation");
    enabled_toggles.push_back("disable_robustness");
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

    // request device
    {
        WGPURequestDeviceCallbackInfo device_callback_info = {};
        device_callback_info.mode = WGPUCallbackMode_AllowProcessEvents;
        device_callback_info.userdata1 = &device;
        device_callback_info.callback = [](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* userdata1, void* userdata2) {
            WGPUDevice* out_device = reinterpret_cast<WGPUDevice*>(userdata1);
            if (status == WGPURequestDeviceStatus_Success) {
                *out_device = device;
            }
            else {
                spdlog::error("Could not get WebGPU device: {}", message.data);
            }
        };

        return wgpuAdapterRequestDevice(
            adapter,
            &device_desc,
            device_callback_info
        );
    }
}

void WebGPUContext::print_device_info()
{
    WGPUAdapterInfo adapter_info = {};
    wgpuAdapterGetInfo(adapter, &adapter_info);

    //spdlog::info("VendorID: {0:x}", adapter_info.vendorID);
    //spdlog::info("Vendor: {}", adapter_info.vendor);
    spdlog::info("Architecture: {:.{}}", adapter_info.architecture.data, adapter_info.architecture.length);
    //spdlog::info("DeviceID: {0:x}", adapter_info.deviceID);
    spdlog::info("Name: {:.{}}", adapter_info.device.data, adapter_info.device.length);
    spdlog::info("Driver description: {:.{}}\n", adapter_info.description.data, adapter_info.description.length);
}

int WebGPUContext::initialize(bool create_screen_swapchain)
{
    wgpuDeviceGetLimits(device, &supported_limits);

#ifdef __EMSCRIPTEN__

    // emscripten-specific extension not supported by webgpu.cpp
    WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvDesc = {};
    canvDesc.chain.sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector;
    canvDesc.selector = { "canvas", WGPU_STRLEN };

    WGPUSurfaceDescriptor surfDesc = {};
    surfDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&canvDesc);

    surface = wgpuInstanceCreateSurface(get_instance(), &surfDesc);
#else

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

    device_queue = wgpuDeviceGetQueue(device);

    {
        std::vector<std::string> defines;
#if defined(BACKEND_METAL) || defined(BACKEND_EMSCRIPTEN)
        defines.push_back("METAL_CUBEMAP_HACK");
#endif
        cubemap_mipmap_pipeline.mipmap_shader = RendererStorage::get_shader_from_source(shaders::cubemap_downsampler::source, shaders::cubemap_downsampler::path, shaders::cubemap_downsampler::libraries, defines);
        cubemap_mipmap_pipeline.mipmap_pipeline = new Pipeline();
        cubemap_mipmap_pipeline.mipmap_pipeline->create_compute(cubemap_mipmap_pipeline.mipmap_shader);
    }

    {
        panorama_to_cubemap_shader = RendererStorage::get_shader_from_source(shaders::panorama_to_cubemap::source, shaders::panorama_to_cubemap::path, shaders::panorama_to_cubemap::libraries);

        panorama_to_cubemap_pipeline = new Pipeline();
        panorama_to_cubemap_pipeline->create_compute(panorama_to_cubemap_shader);
    }

    {
        std::vector<std::string> defines;
#if defined(BACKEND_METAL) || defined(BACKEND_EMSCRIPTEN)
        defines.push_back("METAL_CUBEMAP_HACK");
#endif
        prefiltered_env_shader = RendererStorage::get_shader_from_source(shaders::prefilter_env::source, shaders::prefilter_env::path, shaders::prefilter_env::libraries, defines);

        prefiltered_env_pipeline = new Pipeline();
        prefiltered_env_pipeline->create_compute(prefiltered_env_shader);
    }

    {
        brdf_lut_shader = RendererStorage::get_shader_from_source(shaders::brdf_lut_gen::source, shaders::brdf_lut_gen::path, shaders::brdf_lut_gen::libraries);

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

    wgpuSurfaceRelease(surface);
    wgpuDeviceDestroy(device);
    wgpuQueueRelease(device_queue);
    wgpuInstanceRelease(instance);

    for (auto &mipmap_struct : mipmap_pipelines) {
        delete mipmap_struct.second.mipmap_pipeline;
    }

    delete brdf_lut_pipeline;
    delete brdf_lut_texture;

    delete prefiltered_env_pipeline;

    if (window) {
        close_window();
    }
}

void WebGPUContext::close_window()
{
#if !defined(XR_SUPPORT) || (defined(XR_SUPPORT) && defined(USE_MIRROR_WINDOW))
    glfwDestroyWindow(window);
#endif
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
    WGPUShaderSourceWGSL shader_code_desc = {};
    shader_code_desc.chain.sType = WGPUSType_ShaderSourceWGSL;
    shader_code_desc.code = get_string_view(code);

    WGPUShaderModuleDescriptor shader_descr = {};
    shader_descr.nextInChain = &shader_code_desc.chain;

    return wgpuDeviceCreateShaderModule(device, &shader_descr);
}

WGPUBuffer WebGPUContext::create_buffer(size_t size, int usage, const void* data, const char* label)
{
    WGPUBufferDescriptor bufferDesc = {};

    bufferDesc.size = size;
    bufferDesc.usage = usage;
    bufferDesc.mappedAtCreation = false;
    bufferDesc.label = { label, WGPU_STRLEN };

    WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);

    if (data != nullptr) {
        update_buffer(buffer, 0, data, size);
    }

    return buffer;
}

WGPUTexture WebGPUContext::create_texture(WGPUTextureDimension dimension, WGPUTextureFormat format, WGPUExtent3D size, WGPUTextureUsage usage, uint32_t mipmaps, uint8_t sample_count, const char* label)
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
    textureDesc.label = { label, WGPU_STRLEN };

    return wgpuDeviceCreateTexture(device, &textureDesc);
}

WGPUTextureView WebGPUContext::create_texture_view(WGPUTexture texture, WGPUTextureViewDimension dimension, WGPUTextureFormat format, WGPUTextureAspect aspect, uint32_t base_mip_level, uint32_t mip_level_count, uint32_t base_array_layer, uint32_t array_layer_count, const char* label) const
{
    WGPUTextureViewDescriptor textureViewDesc = {};
    textureViewDesc.aspect = aspect;
    textureViewDesc.baseArrayLayer = base_array_layer;
    textureViewDesc.arrayLayerCount = array_layer_count;
    textureViewDesc.dimension = dimension;
    textureViewDesc.format = format;
    textureViewDesc.mipLevelCount = mip_level_count;
    textureViewDesc.baseMipLevel = base_mip_level;
    textureViewDesc.label = { label, WGPU_STRLEN };

    return wgpuTextureCreateView(texture, &textureViewDesc);
}

WGPUSampler WebGPUContext::create_sampler(WGPUAddressMode wrap_u, WGPUAddressMode wrap_v, WGPUAddressMode wrap_w, WGPUFilterMode mag_filter, WGPUFilterMode min_filter, WGPUMipmapFilterMode mipmap_filter, float lod_max_clamp, uint16_t max_anisotropy, WGPUCompareFunction compare_function)
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
    samplerDesc.compare = compare_function;
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

    WGPUComputePassDescriptor compute_pass_desc = { .label = { "texture_mipmaps_pass", WGPU_STRLEN} };
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
                    std::max(previous_size.width / 2, 1u),
                    std::max(previous_size.height / 2, 1u),
                    std::max(previous_size.depthOrArrayLayers / 2, 1u)
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
    cmd_buff_descriptor.label = { "Create Mipmaps Command Buffer", WGPU_STRLEN };

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

    WGPUComputePassDescriptor compute_pass_desc = { .label = { "cubemap_mipmaps_pass", WGPU_STRLEN} };
    compute_pass_desc.timestampWrites = nullptr;
    WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

    push_debug_group(compute_pass, { "Create Cubemap Mipmaps", WGPU_STRLEN } );

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

    Uniform mipmaps_face_size_uniforms[5];

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

            uint32_t idx = level - 1;
            mipmaps_face_size_uniforms[idx].data = create_buffer(sizeof(uint32_t), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &texture_mip_sizes[level].width, "mipmap_face_size");
            mipmaps_face_size_uniforms[idx].buffer_size = sizeof(uint32_t);
            mipmaps_face_size_uniforms[idx].binding = 3;
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

        std::vector<Uniform*> uniforms = { &source_view, &output_view, &sampler, &mipmaps_face_size_uniforms[nextLevel - 1]};
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

    pop_debug_group(compute_pass);

    // Finalize compute_raymarching pass
    wgpuComputePassEncoderEnd(compute_pass);

    WGPUCommandBufferDescriptor cmd_buff_descriptor = {};
    cmd_buff_descriptor.nextInChain = NULL;
    cmd_buff_descriptor.label = { "Create Mipmaps Command Buffer", WGPU_STRLEN };

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

    //for (int i = 0; i < mip_level_count - 1; ++i) {
    //    mipmaps_face_size_uniforms->destroy();
    //}

}

void WebGPUContext::upload_texture(WGPUTexture texture, WGPUTextureDimension dimension, WGPUExtent3D texture_size, uint32_t mip_level, WGPUTextureFormat format, const void* data, WGPUOrigin3D origin)
{
    WGPUQueue mipmap_queue = wgpuDeviceGetQueue(device);

    WGPUTexelCopyTextureInfo destination = {};
    destination.texture = texture;
    destination.origin = origin;
    destination.aspect = WGPUTextureAspect_All;
    destination.mipLevel = mip_level;

    WGPUTexelCopyBufferLayout source = {};
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
    case WGPUTextureFormat_R32Float:
        pixel_size = sizeof(float);
        break;
    case WGPUTextureFormat_RGBA32Float:
        pixel_size = 4 * sizeof(float);
        break;
    case WGPUTextureFormat_RGBA32Sint:
        pixel_size = 4 * sizeof(int32_t);
        break;
    default:
        assert(false);
    }

    source.bytesPerRow = pixel_size * texture_size.width;
    byte_size = source.bytesPerRow * texture_size.height;
    if (dimension == WGPUTextureDimension_3D) byte_size = byte_size * texture_size.depthOrArrayLayers;

    source.rowsPerImage = texture_size.height;

    // Copy data to the selected mipmap
    wgpuQueueWriteTexture(mipmap_queue, &destination, data, byte_size, &source, &texture_size);

    wgpuQueueRelease(mipmap_queue);
}

WGPUBindGroupLayout WebGPUContext::create_bind_group_layout(const std::vector<WGPUBindGroupLayoutEntry> &entries, char const* label)
{
    // Create a bind group layout
    WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
    bindGroupLayoutDesc.entryCount = static_cast<uint32_t>(entries.size());
    bindGroupLayoutDesc.entries = entries.data();
    bindGroupLayoutDesc.label = { label, WGPU_STRLEN };

    return wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);
}

WGPUBindGroup WebGPUContext::create_bind_group(const std::vector<Uniform*>& uniforms, WGPUBindGroupLayout bind_group_layout, char const* label)
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
    bindGroupDesc.label = { label, WGPU_STRLEN };

    return wgpuDeviceCreateBindGroup(device, &bindGroupDesc);
}

WGPUBindGroup WebGPUContext::create_bind_group(const std::vector<Uniform*>& uniforms, const Shader* shader, uint16_t bind_group, char const* label) const
{
    assert(!uniforms.empty());

    spdlog::trace("Creating bind group {} for shader {}", bind_group, shader->get_path());

    std::vector<WGPUBindGroupEntry> entries(uniforms.size());

    for (int i = 0; i < uniforms.size(); ++i) {
        entries[i] = uniforms[i]->get_bind_group_entry();
    }

    const std::vector<WGPUBindGroupLayout>& layouts_by_id = shader->get_bind_group_layouts();

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
    bindGroupDesc.label = { label, WGPU_STRLEN };

    return wgpuDeviceCreateBindGroup(device, &bindGroupDesc);
}

void WebGPUContext::read_buffer(WGPUBuffer buffer, size_t size, void* output_data)
{
    WGPUBuffer output_buffer = create_buffer(size, WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, nullptr, "read_buffer");

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, {});

    wgpuCommandEncoderCopyBufferToBuffer(encoder, buffer, 0, output_buffer, 0, size);

    WGPUCommandBufferDescriptor cmd_buff_descriptor = {};
    cmd_buff_descriptor.nextInChain = NULL;
    cmd_buff_descriptor.label = { "read buffer command", WGPU_STRLEN };

    WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, &cmd_buff_descriptor);

    wgpuQueueSubmit(device_queue, 1, &commands);

    wgpuCommandBufferRelease(commands);
    wgpuCommandEncoderRelease(encoder);

    struct BufferData {
        bool finished = false;
        WGPUBuffer output_buffer = nullptr;
        size_t buffer_size = 0;
        void* data = nullptr;
    } userdata;

    userdata.output_buffer = output_buffer;
    userdata.buffer_size = size;
    userdata.data = output_data;

    WGPUBufferMapCallbackInfo callback_info;
    callback_info.mode = WGPUCallbackMode_AllowProcessEvents;
    callback_info.userdata1 = &userdata;

    callback_info.callback = [](WGPUMapAsyncStatus status, struct WGPUStringView message, void* userdata1, void* userdata2) {

        BufferData* buffer_data = reinterpret_cast<BufferData*>(userdata1);

        if (status == WGPUMapAsyncStatus_Success) {
            memcpy(buffer_data->data, wgpuBufferGetConstMappedRange(buffer_data->output_buffer, 0, buffer_data->buffer_size), buffer_data->buffer_size);
            wgpuBufferUnmap(buffer_data->output_buffer);
        }
        else {
            //spdlog::error("Error reading buffer: {}", message);
        }

        buffer_data->finished = true;

    };

    bool finished = false;
    wgpuBufferMapAsync(output_buffer, WGPUMapMode_Read, 0, size, callback_info);

    while (!userdata.finished) {
        process_events();
    }

    wgpuBufferRelease(output_buffer);
}

void WebGPUContext::read_buffer_async(WGPUBuffer buffer, size_t size, const std::function<void(const void* output_buffer, void* userdata)>& read_callback, void* read_userdata)
{
    WGPUBuffer output_buffer = create_buffer(size, WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, nullptr, "read_buffer_async");

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, {});

    wgpuCommandEncoderCopyBufferToBuffer(encoder, buffer, 0, output_buffer, 0, size);

    WGPUCommandBufferDescriptor cmd_buff_descriptor = {};
    cmd_buff_descriptor.nextInChain = NULL;
    cmd_buff_descriptor.label = { "read buffer command", WGPU_STRLEN };

    WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, &cmd_buff_descriptor);

    wgpuQueueSubmit(device_queue, 1, &commands);

    wgpuCommandBufferRelease(commands);
    wgpuCommandEncoderRelease(encoder);

    struct BufferData {
        WGPUBuffer output_buffer = nullptr;
        size_t buffer_size = 0;
        std::function<void(const void*, void*)> read_callback;
        void* read_userdata;
    };

    BufferData* userdata = new BufferData();
    userdata->output_buffer = output_buffer;
    userdata->buffer_size = size;
    userdata->read_callback = read_callback;
    userdata->read_userdata = read_userdata;

    WGPUBufferMapCallbackInfo callback_info;
    callback_info.mode = WGPUCallbackMode_AllowProcessEvents;
    callback_info.userdata1 = userdata;

    callback_info.callback = [](WGPUMapAsyncStatus status, struct WGPUStringView message, void* userdata1, void* userdata2) {

        BufferData* buffer_data = reinterpret_cast<BufferData*>(userdata1);

        if (status == WGPUMapAsyncStatus_Success) {
            const void* read_data = wgpuBufferGetConstMappedRange(buffer_data->output_buffer, 0, buffer_data->buffer_size);
            buffer_data->read_callback(read_data, buffer_data->read_userdata);
            wgpuBufferUnmap(buffer_data->output_buffer);
        }
        else {
            //spdlog::error("Error reading buffer: {}", message);
        }

        wgpuBufferRelease(buffer_data->output_buffer);
        delete buffer_data;
    };

    wgpuBufferMapAsync(output_buffer, WGPUMapMode_Read, 0, size, callback_info);
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

    Shader* shader = RendererStorage::get_shader_from_source(shaders::mipmaps::source, shaders::mipmaps::path, shaders::mipmaps::libraries, { custom_define });

    Pipeline* pipeline = new Pipeline();
    pipeline->create_compute(shader);

    sMipmapPipeline mipmap_pipeline = { pipeline, shader };
    mipmap_pipelines[texture_format] = mipmap_pipeline;

    return mipmap_pipeline;
}

void WebGPUContext::push_debug_group(WGPURenderPassEncoder render_pass, WGPUStringView label)
{
#ifndef __EMSCRIPTEN__
    wgpuRenderPassEncoderPushDebugGroup(render_pass, label);
#endif
}

void WebGPUContext::push_debug_group(WGPUComputePassEncoder compute_pass, WGPUStringView label)
{
#ifndef __EMSCRIPTEN__
    wgpuComputePassEncoderPushDebugGroup(compute_pass, label);
#endif
}

void WebGPUContext::pop_debug_group(WGPURenderPassEncoder render_pass)
{
#ifndef __EMSCRIPTEN__
    wgpuRenderPassEncoderPopDebugGroup(render_pass);
#endif
}

void WebGPUContext::pop_debug_group(WGPUComputePassEncoder compute_pass)
{
#ifndef __EMSCRIPTEN__
    wgpuComputePassEncoderPopDebugGroup(compute_pass);
#endif
}

void WebGPUContext::process_events()
{
    wgpuInstanceProcessEvents(instance);

//#ifndef __EMSCRIPTEN__
//    wgpuInstanceProcessEvents(instance);
//#else
//    emscripten_sleep(50);
//#endif
}

//WGPURenderPipelineDescriptor WebGPUContext::create_render_pipeline_common(WGPUShaderModule render_shader_module, WGPUPipelineLayout pipeline_layout, const std::vector<WGPUVertexBufferLayout>& vertex_attributes, WGPUColorTargetState color_target, bool use_depth, bool depth_read, bool depth_write, WGPUCullMode cull_mode, WGPUPrimitiveTopology topology, uint8_t sample_count, const char* vs_entry_point, const char* fs_entry_point)
//{
//    WGPUVertexState vertex_state = {};
//    vertex_state.module = render_shader_module;
//    vertex_state.entryPoint = vs_entry_point;
//    vertex_state.constantCount = 0;
//    vertex_state.constants = NULL;
//    vertex_state.bufferCount = static_cast<uint32_t>(vertex_attributes.size());
//    vertex_state.buffers = vertex_attributes.data();
//
//    WGPUFragmentState fragment_state = {};
//    fragment_state.module = render_shader_module;
//    fragment_state.entryPoint = fs_entry_point;
//    fragment_state.constantCount = 0;
//    fragment_state.constants = NULL;
//    fragment_state.targetCount = 1;
//    fragment_state.targets = &color_target;
//
//    WGPUDepthStencilState depth_state = {};
//    depth_state.depthCompare = depth_read ? WGPUCompareFunction_Less : WGPUCompareFunction_Always;
//    depth_state.depthWriteEnabled = depth_write;
//    depth_state.format = WGPUTextureFormat_Depth32Float;
//    depth_state.stencilReadMask = 0;
//    depth_state.stencilWriteMask = 0;
//    // Configure the stencils even if unused
//    depth_state.stencilFront.compare = WGPUCompareFunction_Always;
//    depth_state.stencilFront.failOp = WGPUStencilOperation_Keep;
//    depth_state.stencilFront.depthFailOp = WGPUStencilOperation_Keep;
//    depth_state.stencilFront.passOp = WGPUStencilOperation_Keep;
//    depth_state.stencilBack.compare = WGPUCompareFunction_Always;
//    depth_state.stencilBack.failOp = WGPUStencilOperation_Keep;
//    depth_state.stencilBack.depthFailOp = WGPUStencilOperation_Keep;
//    depth_state.stencilBack.passOp = WGPUStencilOperation_Keep;
//
//    WGPURenderPipelineDescriptor pipeline_descr = {};
//    pipeline_descr.nextInChain = NULL;
//    pipeline_descr.layout = pipeline_layout;
//    pipeline_descr.vertex = vertex_state;
//
//    pipeline_descr.primitive = {
//        .topology = topology,
//        .stripIndexFormat = WGPUIndexFormat_Undefined, // order of the connected vertices
//        .frontFace = WGPUFrontFace_CCW,
//        .cullMode = cull_mode
//    },
//
//        pipeline_descr.depthStencil = use_depth ? &depth_state : nullptr;
//    pipeline_descr.multisample = {
//            .count = sample_count,
//            .mask = ~0u,
//            .alphaToCoverageEnabled = false
//    };
//
//    pipeline_descr.fragment = &fragment_state;
//
//    return pipeline_descr;
//}

WGPUPipelineLayout WebGPUContext::create_pipeline_layout(const std::vector<WGPUBindGroupLayout>& bind_group_layouts, const std::string& label)
{
    WGPUPipelineLayoutDescriptor layout_descr = {};
    layout_descr.nextInChain = NULL;
    layout_descr.bindGroupLayoutCount = static_cast<uint32_t>(bind_group_layouts.size());
    layout_descr.bindGroupLayouts = (WGPUBindGroupLayout*)bind_group_layouts.data();
    layout_descr.label = { label.c_str(), label.length() };

    return wgpuDeviceCreatePipelineLayout(device, &layout_descr);
}

void WebGPUContext::copy_texture_to_texture(WGPUTexture texture_src, WGPUTexture texture_dst, uint32_t src_mipmap_level, uint32_t dst_mipmap_level, const WGPUExtent3D& copy_size,
    const WGPUOrigin3D& src_origin, const WGPUOrigin3D& dst_origin, WGPUCommandEncoder custom_command_encoder)
{
    WGPUTexelCopyTextureInfo src_copy = {};
    src_copy.texture = texture_src;
    src_copy.mipLevel = src_mipmap_level;
    src_copy.origin = src_origin;
    src_copy.aspect = WGPUTextureAspect_All;

    WGPUTexelCopyTextureInfo dst_copy = {};
    dst_copy.texture = texture_dst;
    dst_copy.mipLevel = dst_mipmap_level;
    dst_copy.origin = dst_origin;
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
        cmd_buff_descriptor.label = { "Copy texture Command Buffer", WGPU_STRLEN };

        // Encode and submit the GPU commands
        WGPUCommandBuffer commands = wgpuCommandEncoderFinish(command_encoder, &cmd_buff_descriptor);
        wgpuQueueSubmit(copy_queue, 1, &commands);

        wgpuCommandBufferRelease(commands);
        wgpuCommandEncoderRelease(command_encoder);
    }
}

WGPURenderPipeline WebGPUContext::create_render_pipeline(WGPUShaderModule render_shader_module, WGPUPipelineLayout pipeline_layout, const std::vector<WGPUVertexBufferLayout>& vertex_attributes,
    WGPUColorTargetState color_target, const RenderPipelineDescription& description, std::vector< WGPUConstantEntry> constants)
{    
    WGPUVertexState vertex_state = {};
    vertex_state.module = render_shader_module;
    vertex_state.entryPoint = { description.vs_entry_point.c_str(), description.vs_entry_point.size() };
    vertex_state.constantCount = constants.size();
    vertex_state.constants = constants.data();
    vertex_state.bufferCount = static_cast<uint32_t>(vertex_attributes.size());
    vertex_state.buffers = vertex_attributes.data();

    WGPUFragmentState fragment_state = {};
    fragment_state.module = render_shader_module;
    fragment_state.entryPoint = { description.fs_entry_point.c_str(), description.fs_entry_point.size() };
    fragment_state.constantCount = constants.size();
    fragment_state.constants = constants.data();
    fragment_state.targetCount = 1;
    fragment_state.targets = &color_target;

    WGPUDepthStencilState depth_state = {};
    depth_state.depthCompare = description.depth_read ? description.depth_compare : WGPUCompareFunction_Always;
    depth_state.depthWriteEnabled = description.depth_write;
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

    bool strip_topology = (description.topology == WGPUPrimitiveTopology_TriangleStrip) || (description.topology == WGPUPrimitiveTopology_LineStrip);

    pipeline_descr.primitive = {
        .topology = description.topology,
        .stripIndexFormat = strip_topology ? WGPUIndexFormat_Uint32 : WGPUIndexFormat_Undefined, // order of the connected vertices
        .frontFace = WGPUFrontFace_CCW,
        .cullMode = description.cull_mode
    },

    pipeline_descr.depthStencil = description.use_depth ? &depth_state : nullptr;
    pipeline_descr.multisample = {
            .count = description.sample_count,
            .mask = ~0u,
            .alphaToCoverageEnabled = false
    };

    pipeline_descr.fragment = &fragment_state;
 
    return wgpuDeviceCreateRenderPipeline(device, &pipeline_descr);
}

void WebGPUContext::create_render_pipeline_async(WGPUShaderModule render_shader_module, WGPUPipelineLayout pipeline_layout, const std::vector<WGPUVertexBufferLayout>& vertex_attributes,
    WGPUColorTargetState color_target, WGPUCreateRenderPipelineAsyncCallbackInfo callback_info, const RenderPipelineDescription& description, std::vector< WGPUConstantEntry> constants)
{
    WGPUVertexState vertex_state = {};
    vertex_state.module = render_shader_module;
    vertex_state.entryPoint = { description.vs_entry_point.c_str(), description.vs_entry_point.size() };
    vertex_state.constantCount = constants.size();
    vertex_state.constants = constants.data();
    vertex_state.bufferCount = static_cast<uint32_t>(vertex_attributes.size());
    vertex_state.buffers = vertex_attributes.data();

    WGPUFragmentState fragment_state = {};
    fragment_state.module = render_shader_module;
    fragment_state.entryPoint = { description.fs_entry_point.c_str(), description.fs_entry_point.size() };
    fragment_state.constantCount = constants.size();
    fragment_state.constants = constants.data();
    fragment_state.targetCount = 1;
    fragment_state.targets = &color_target;

    WGPUDepthStencilState depth_state = {};
    depth_state.depthCompare = description.depth_read ? description.depth_compare : WGPUCompareFunction_Always;
    depth_state.depthWriteEnabled = description.depth_write;
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

    bool strip_topology = (description.topology == WGPUPrimitiveTopology_TriangleStrip) || (description.topology == WGPUPrimitiveTopology_LineStrip);

    pipeline_descr.primitive = {
        .topology = description.topology,
        .stripIndexFormat = strip_topology ? WGPUIndexFormat_Uint32 : WGPUIndexFormat_Undefined, // order of the connected vertices
        .frontFace = WGPUFrontFace_CCW,
        .cullMode = description.cull_mode
    },

    pipeline_descr.depthStencil = description.use_depth ? &depth_state : nullptr;
    pipeline_descr.multisample = {
            .count = description.sample_count,
            .mask = ~0u,
            .alphaToCoverageEnabled = false
    };

    if (description.has_fragment_state) {
        pipeline_descr.fragment = &fragment_state;
    }

    wgpuDeviceCreateRenderPipelineAsync(device, &pipeline_descr, callback_info);
}

WGPUComputePipeline WebGPUContext::create_compute_pipeline(WGPUShaderModule compute_shader_module, WGPUPipelineLayout pipeline_layout,
    const char* entry_point, std::vector< WGPUConstantEntry> constants)
{
    WGPUComputePipelineDescriptor computePipelineDesc = {};
    computePipelineDesc.compute.nextInChain = nullptr;
    computePipelineDesc.compute.constantCount = constants.size();
    computePipelineDesc.compute.constants = constants.data();
    computePipelineDesc.compute.entryPoint = { entry_point, WGPU_STRLEN };
    computePipelineDesc.compute.module = compute_shader_module;
    computePipelineDesc.layout = pipeline_layout;

    return wgpuDeviceCreateComputePipeline(device, &computePipelineDesc);
}

void WebGPUContext::create_compute_pipeline_async(WGPUShaderModule compute_shader_module, WGPUPipelineLayout pipeline_layout,
    WGPUCreateComputePipelineAsyncCallbackInfo callback_info, const char* entry_point, std::vector< WGPUConstantEntry> constants)
{
    WGPUComputePipelineDescriptor computePipelineDesc = {};
    computePipelineDesc.compute.nextInChain = nullptr;
    computePipelineDesc.compute.constantCount = constants.size();
    computePipelineDesc.compute.constants = constants.data();
    computePipelineDesc.compute.entryPoint = { entry_point, WGPU_STRLEN };
    computePipelineDesc.compute.module = compute_shader_module;
    computePipelineDesc.layout = pipeline_layout;

   wgpuDeviceCreateComputePipelineAsync(device, &computePipelineDesc, callback_info);
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

WGPUQuerySet WebGPUContext::create_query_set(uint8_t maximum_query_sets)
{
    WGPUQuerySetDescriptor query_set_descriptor = {};
    query_set_descriptor.count = maximum_query_sets;
    query_set_descriptor.type = WGPUQueryType_Timestamp;
    query_set_descriptor.label = { "timestamp_query", WGPU_STRLEN };

    return wgpuDeviceCreateQuerySet(device, &query_set_descriptor);
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

    WGPUComputePassDescriptor compute_pass_desc = { .label = { "brdf_lut_pass", WGPU_STRLEN} };
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
    cmd_buff_descriptor.label = { "Create BRDF Command Buffer", WGPU_STRLEN };

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

    Uniform mipmaps_face_size_uniforms[5];
    Uniform cubemap_mipmaps_uniforms[5];
    for (int i = 0; i < 5; ++i) {
        cubemap_mipmaps_uniforms[i].data = prefiltered_env_texture->get_view(WGPUTextureViewDimension_2DArray, i + 1, 1, 0, 6);
        cubemap_mipmaps_uniforms[i].binding = 1;

        uint32_t face_size = prefiltered_env_texture->get_width() / (2 << i);
        mipmaps_face_size_uniforms[i].data = create_buffer(sizeof(uint32_t), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &face_size, "face_size");
        mipmaps_face_size_uniforms[i].buffer_size = sizeof(uint32_t);
        mipmaps_face_size_uniforms[i].binding = 4;
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

    uint32_t buffer_stride = std::max(static_cast<uint32_t>(sizeof(PrefilterEnvUniformData)), required_limits.minUniformBufferOffsetAlignment);

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

        WGPUComputePassDescriptor compute_pass_desc = { .label = { "panorama_to_cubemap_pass", WGPU_STRLEN} };
        compute_pass_desc.timestampWrites = nullptr;
        WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

        push_debug_group(compute_pass, { "Panorama to Cubemap", WGPU_STRLEN });

        panorama_to_cubemap_pipeline->set(compute_pass);

        uint32_t invocationCountX = cubemap_texture.get_width();
        uint32_t invocationCountY = cubemap_texture.get_height();
        uint32_t workgroupSizePerDim = 4;

        uint32_t workgroupCountX = (invocationCountX + workgroupSizePerDim - 1) / workgroupSizePerDim;
        uint32_t workgroupCountY = (invocationCountY + workgroupSizePerDim - 1) / workgroupSizePerDim;

        wgpuComputePassEncoderSetBindGroup(compute_pass, 0, bind_group, 0, nullptr);

        wgpuComputePassEncoderDispatchWorkgroups(compute_pass, workgroupCountX, workgroupCountY, 1);

        wgpuBindGroupRelease(bind_group);

        pop_debug_group(compute_pass);

        // Finalize compute_raymarching pass
        wgpuComputePassEncoderEnd(compute_pass);

        wgpuComputePassEncoderRelease(compute_pass);
    }

    create_cubemap_mipmaps(cubemap_texture.get_texture(), cubemap_texture.get_size(), 6, WGPUTextureViewDimension_Cube, cubemap_texture.get_format(), { 0, 0, 0 }, command_encoder);

    // copy first cubemap mipmap to final texture
    copy_texture_to_texture(cubemap_texture.get_texture(), prefiltered_env_texture->get_texture(), 0, 0, cubemap_texture.get_size(), { 0, 0, 0 }, { 0, 0, 0 }, command_encoder);

    // Prefilter cubemap
    {
        WGPUBindGroup bind_groups[5];

        for (uint32_t i = 0; i < 5; ++i) {
            std::vector<Uniform*> uniforms = { &cubemap_all_input_output_uniform, &cubemap_mipmaps_uniforms[i], &sampler, &current_level_uniform, &mipmaps_face_size_uniforms[i]};
            bind_groups[i] = create_bind_group(uniforms, prefiltered_env_shader, 0);
        }

        prefilter_env_uniform_data.mip_level_count = 6;

        // Setup uniform buffers
        for (uint32_t i = 0; i < 5; ++i) {
            prefilter_env_uniform_data.current_mip_level = i + 1;
            wgpuQueueWriteBuffer(prefilter_queue, std::get<WGPUBuffer>(current_level_uniform.data), i * buffer_stride, &prefilter_env_uniform_data, sizeof(PrefilterEnvUniformData));
        }

        WGPUComputePassDescriptor compute_pass_desc = { .label = { "prefilter_env_pass", WGPU_STRLEN} };
        compute_pass_desc.timestampWrites = nullptr;
        WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

        push_debug_group(compute_pass, { "Prefilter Cubemap", WGPU_STRLEN });

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

        pop_debug_group(compute_pass);

        // Finalize compute_raymarching pass
        wgpuComputePassEncoderEnd(compute_pass);

        wgpuComputePassEncoderRelease(compute_pass);
    }

    WGPUCommandBufferDescriptor cmd_buff_descriptor = {};
    cmd_buff_descriptor.nextInChain = NULL;
    cmd_buff_descriptor.label = { "Create Prefiltered Env Command Buffer", WGPU_STRLEN };

    // Encode and submit the GPU commands
    WGPUCommandBuffer commands = wgpuCommandEncoderFinish(command_encoder, &cmd_buff_descriptor);
    wgpuQueueSubmit(prefilter_queue, 1, &commands);

    wgpuCommandBufferRelease(commands);
    wgpuCommandEncoderRelease(command_encoder);

    wgpuQueueRelease(prefilter_queue);

    for (int i = 0; i < 5; ++i) {
        cubemap_mipmaps_uniforms[i].destroy();
        mipmaps_face_size_uniforms[i].destroy();
    }

    cubemap_all_faces_output_uniform.destroy();
    cubemap_all_input_output_uniform.destroy();
    sampler.destroy();
    hdr_uniform.destroy();
    current_level_uniform.destroy();
}

void WebGPUContext::update_buffer(WGPUBuffer buffer, uint64_t buffer_offset, void const* data, size_t size)
{
    wgpuQueueWriteBuffer(device_queue, buffer, buffer_offset, data, size);
}

WGPUInstance WebGPUContext::get_instance()
{
    return instance;
}

void WebGPUContext::create_swapchain(int width, int height)
{
    WGPUSurfaceCapabilities surface_capabilities = {};
    wgpuSurfaceGetCapabilities(surface, adapter, &surface_capabilities);

    bool support_mailbox_present = false;

    for (uint32_t i = 0u; i < surface_capabilities.presentModeCount; i++) {
        if (surface_capabilities.presentModes[i] == WGPUPresentMode_Mailbox) {
            support_mailbox_present = true;
            break;
        }
    }

    WGPUSurfaceConfiguration surface_config = {};
#ifdef __EMSCRIPTEN__
    surface_config.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding;
    surface_config.presentMode = WGPUPresentMode_Fifo;
#else
    surface_config.usage = WGPUTextureUsage_RenderAttachment;
    surface_config.presentMode = (support_mailbox_present) ? WGPUPresentMode_Mailbox : WGPUPresentMode_Fifo;
#endif
    surface_config.format = swapchain_format;
    surface_config.width = width;
    surface_config.height = height;

    surface_config.alphaMode = WGPUCompositeAlphaMode_Opaque;
    surface_config.device = device;
    surface_config.viewFormatCount = 0;

    wgpuSurfaceConfigure(surface, &surface_config);
}
