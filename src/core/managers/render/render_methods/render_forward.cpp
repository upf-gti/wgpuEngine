#include "render_forward.h"

#include "core/managers/render/render_api.h"
#include "core/managers/render/render_manager.h"
#include "core/managers/xr/xr_manager.h"
#include "core/managers/engine/engine_manager.h"

#include "graphics/pipeline.h"
#include "graphics/renderer_storage.h"
#include "graphics/texture.h"
#include "graphics/uniform.h"
#include "graphics/shader.h"

#include "shaders/brdf_lut_gen.wgsl.gen.h"
#include "shaders/panorama_to_cubemap.wgsl.gen.h"
#include "shaders/prefilter_env.wgsl.gen.h"
#include "shaders/mesh_forward.wgsl.gen.h"

#include "webgpu/webgpu.h"

#include "spdlog/spdlog.h"

#include <algorithm>

Error RenderForward::initialize()
{
    {
        if (!irradiance_texture) {
            irradiance_texture = RendererStorage::get_texture("data/textures/environments/sky.hdr");
        }

        init_depth_buffers();
        init_lighting_bind_group();
    }
    {
        panorama_to_cubemap_shader = RendererStorage::get_shader_from_source(shaders::panorama_to_cubemap::source, shaders::panorama_to_cubemap::path, shaders::panorama_to_cubemap::libraries);
        panorama_to_cubemap_pipeline.create_compute(panorama_to_cubemap_shader);
    }

    {
        std::vector<std::string> defines;
#if defined(BACKEND_METAL) || defined(BACKEND_EMSCRIPTEN)
        defines.push_back("METAL_CUBEMAP_HACK");
#endif
        prefiltered_env_shader = RendererStorage::get_shader_from_source(shaders::prefilter_env::source, shaders::prefilter_env::path, shaders::prefilter_env::libraries, defines);
        prefiltered_env_pipeline.create_compute(prefiltered_env_shader);
    }

    {
        brdf_lut_shader = RendererStorage::get_shader_from_source(shaders::brdf_lut_gen::source, shaders::brdf_lut_gen::path, shaders::brdf_lut_gen::libraries);
        brdf_lut_pipeline.create_compute(brdf_lut_shader);
        brdf_lut_texture.create(WGPUTextureDimension_2D, WGPUTextureFormat_RG32Float, { 512, 512, 1 },
                static_cast<WGPUTextureUsage>(WGPUTextureUsage_StorageBinding | WGPUTextureUsage_TextureBinding), 1, 1, nullptr);

        generate_brdf_lut_texture();
    }

    return Error::OK;
}

Error RenderForward::finalize()
{
    return Error::OK;
}

void RenderForward::render_scene()
{
}

void RenderForward::init_depth_buffers()
{
    uint32_t render_width = static_cast<uint32_t>(RenderManager::get_singleton()->get_render_width());
    uint32_t render_height = static_cast<uint32_t>(RenderManager::get_singleton()->get_render_height());

    if (render_width == 0 || render_height == 0) {
        spdlog::error("Can not create depth buffer with size ({}, {})", render_width, render_height);
        return;
    }

    uint8_t msaa_count = EngineManager::get_singleton()->get_configuration().render_config.msaa_count;
    uint8_t num_textures = XRManager::get_singleton()->is_xr_available() ? 2 : 1;
    for (int i = 0; i < num_textures; ++i) {
        depth_buffers[i].create(
                WGPUTextureDimension_2D,
                WGPUTextureFormat_Depth32Float,
                { render_width, render_height, 1 },
                WGPUTextureUsage_RenderAttachment,
                1, msaa_count, nullptr);

        if (depth_buffers_views[i]) {
            wgpuTextureViewRelease(depth_buffers_views[i]);
        }

        // Generate Texture views of depth buffers
        depth_buffers_views[i] = depth_buffers[i].get_view();
    }

    spdlog::info("Depth buffers initialized with size ({}, {})", render_width, render_height);
}

void RenderForward::init_multisample_textures()
{
    uint32_t render_width = static_cast<uint32_t>(RenderManager::get_singleton()->get_render_width());
    uint32_t render_height = static_cast<uint32_t>(RenderManager::get_singleton()->get_render_height());

    if (render_width == 0 || render_height == 0) {
        spdlog::error("Can not multisample textures with size ({}, {})", render_width, render_height);
        return;
    }

    bool xr_available = XRManager::get_singleton()->is_xr_available();
    WGPUTextureFormat swapchain_format = xr_available ? XRManager::get_singleton()->get_swapchain_format() : RenderManager::get_singleton()->get_surface_format();

    uint8_t msaa_count = EngineManager::get_singleton()->get_configuration().render_config.msaa_count;
    uint8_t num_textures = xr_available ? 2 : 1;
    for (int i = 0; i < num_textures; ++i) {
        multisample_textures[i].create(
                WGPUTextureDimension_2D,
                swapchain_format,
                { render_width, render_height, 1 },
                WGPUTextureUsage_RenderAttachment,
                1, msaa_count, nullptr);

        if (multisample_textures_views[i]) {
            wgpuTextureViewRelease(multisample_textures_views[i]);
        }

        multisample_textures_views[i] = multisample_textures[i].get_view();
    }

    spdlog::info("Multisample textures initialized with size ({}, {})", render_width, render_height);
}

void RenderForward::init_lighting_bind_group()
{
    // delete if already created
    if (std::holds_alternative<WGPUTextureView>(irradiance_texture_uniform.data)) {
        wgpuTextureViewRelease(std::get<WGPUTextureView>(irradiance_texture_uniform.data));
        wgpuSamplerRelease(std::get<WGPUSampler>(ibl_sampler_uniform.data));
        wgpuBindGroupRelease(lighting_bind_group);
    } else {
        // only created once
        brdf_lut_uniform.data = brdf_lut_texture.get_view();
        brdf_lut_uniform.binding = 1;
    }

    if (irradiance_texture) {
        irradiance_texture_uniform.data = irradiance_texture->get_view(WGPUTextureViewDimension_Cube, 0, 6, 0, 6);
        irradiance_texture_uniform.binding = 0;

        ibl_sampler_uniform.data = RenderAPI::get_singleton()->sampler_create(
                WGPUAddressMode_ClampToEdge,
                WGPUAddressMode_ClampToEdge,
                WGPUAddressMode_ClampToEdge,
                WGPUFilterMode_Linear,
                WGPUFilterMode_Linear,
                WGPUMipmapFilterMode_Linear,
                static_cast<float>(irradiance_texture->get_mipmap_count()));

        ibl_sampler_uniform.binding = 2;
    }

    if (std::holds_alternative<WGPUBuffer>(lights_buffer.data)) {
        wgpuBufferDestroy(std::get<WGPUBuffer>(lights_buffer.data));
        lights_buffer.data = {};
    }

    lights_buffer.data = RenderAPI::get_singleton()->buffer_create(sizeof(sLightUniformData) * MAX_LIGHTS, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &lights_uniform_data[0], "lights_buffer");
    lights_buffer.binding = 3;
    lights_buffer.buffer_size = sizeof(sLightUniformData) * MAX_LIGHTS;

    num_lights_buffer.data = RenderAPI::get_singleton()->buffer_create(sizeof(int), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &num_lights, "num_lights_buffer");
    num_lights_buffer.binding = 4;
    num_lights_buffer.buffer_size = sizeof(int);

    // Shadow maps
    {
        shadow_array_texture = RenderAPI::get_singleton()->texture_create(
                WGPUTextureDimension_2D,
                WGPUTextureFormat_Depth32Float,
                { SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, MAX_LIGHTS },
                WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
                1,
                1,
                "shadow_array_texture");

        shadow_maps_array.data = RenderAPI::get_singleton()->texture_view_create(
                shadow_array_texture,
                WGPUTextureViewDimension_2DArray,
                WGPUTextureFormat_Depth32Float,
                WGPUTextureAspect_DepthOnly,
                0,
                1,
                0,
                MAX_LIGHTS,
                "shadow_depth_texture_view");
        shadow_maps_array.binding = 5;

        // Shadowmap sampler
        shadow_sampler.data = RenderAPI::get_singleton()->sampler_create(
                WGPUAddressMode_ClampToEdge,
                WGPUAddressMode_ClampToEdge,
                WGPUAddressMode_ClampToEdge,
                WGPUFilterMode_Linear,
                WGPUFilterMode_Linear,
                WGPUMipmapFilterMode_Linear,
                1.0f,
                1u,
                WGPUCompareFunction_Greater // reverse Z
        );
        shadow_sampler.binding = 6;
    }

    std::vector<Uniform*> uniforms = { &irradiance_texture_uniform, &brdf_lut_uniform, &ibl_sampler_uniform, &lights_buffer, &num_lights_buffer /*, &shadow_maps_array, &shadow_sampler*/ };
    lighting_bind_group = RenderAPI::get_singleton()->bind_group_create(uniforms, RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, shaders::mesh_forward::libraries), 3);
}

void RenderForward::generate_brdf_lut_texture()
{
    Uniform brdf_lut_uniform;
    brdf_lut_uniform.data = brdf_lut_texture.get_view();
    brdf_lut_uniform.binding = 0;

    std::vector<Uniform*> uniforms = { &brdf_lut_uniform };
    WGPUBindGroup bind_group = RenderAPI::get_singleton()->bind_group_create(uniforms, brdf_lut_shader, 0);

    WGPUQueue brdf_queue = wgpuDeviceGetQueue(RenderAPI::get_singleton()->get_device());

    // Initialize a command encoder
    WGPUCommandEncoderDescriptor encoder_desc = {};
    WGPUCommandEncoder command_encoder = wgpuDeviceCreateCommandEncoder(RenderAPI::get_singleton()->get_device(), &encoder_desc);

    WGPUComputePassDescriptor compute_pass_desc = { .label = { "brdf_lut_pass", WGPU_STRLEN } };
    compute_pass_desc.timestampWrites = nullptr;
    WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

    brdf_lut_pipeline.set(compute_pass);

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

void RenderForward::generate_prefiltered_env_texture(Texture* prefiltered_env_texture, Texture* hdr_texture)
{
    WGPUQueue prefilter_queue = wgpuDeviceGetQueue(RenderAPI::get_singleton()->get_device());

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
        mipmaps_face_size_uniforms[i].data = RenderAPI::get_singleton()->buffer_create(sizeof(uint32_t), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &face_size, "face_size");
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
    sampler.data = RenderAPI::get_singleton()->sampler_create(WGPUAddressMode_ClampToEdge, WGPUAddressMode_ClampToEdge, WGPUAddressMode_ClampToEdge, WGPUFilterMode_Linear, WGPUFilterMode_Linear, WGPUMipmapFilterMode_Linear, 6.0f);
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

    const WGPULimits limits = RenderAPI::get_singleton()->get_supported_limits();
    uint32_t buffer_stride = std::max(static_cast<uint32_t>(sizeof(PrefilterEnvUniformData)), limits.minUniformBufferOffsetAlignment);

    Uniform current_level_uniform;
    current_level_uniform.data = RenderAPI::get_singleton()->buffer_create(buffer_stride * 6, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, nullptr, "previlter env uniform_data");
    current_level_uniform.buffer_size = sizeof(PrefilterEnvUniformData);
    current_level_uniform.binding = 3;

    // Initialize a command encoder
    WGPUCommandEncoderDescriptor encoder_desc = {};
    WGPUCommandEncoder command_encoder = wgpuDeviceCreateCommandEncoder(RenderAPI::get_singleton()->get_device(), &encoder_desc);

    // Panorama to cubemap
    {
        std::vector<Uniform*> uniforms = { &hdr_uniform, &cubemap_all_faces_output_uniform, &sampler };
        WGPUBindGroup bind_group = RenderAPI::get_singleton()->bind_group_create(uniforms, panorama_to_cubemap_shader, 0);

        WGPUComputePassDescriptor compute_pass_desc = { .label = { "panorama_to_cubemap_pass", WGPU_STRLEN } };
        compute_pass_desc.timestampWrites = nullptr;
        WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

        RenderAPI::get_singleton()->push_debug_group(compute_pass, { "Panorama to Cubemap", WGPU_STRLEN });

        panorama_to_cubemap_pipeline.set(compute_pass);

        uint32_t invocationCountX = cubemap_texture.get_width();
        uint32_t invocationCountY = cubemap_texture.get_height();
        uint32_t workgroupSizePerDim = 4;

        uint32_t workgroupCountX = (invocationCountX + workgroupSizePerDim - 1) / workgroupSizePerDim;
        uint32_t workgroupCountY = (invocationCountY + workgroupSizePerDim - 1) / workgroupSizePerDim;

        wgpuComputePassEncoderSetBindGroup(compute_pass, 0, bind_group, 0, nullptr);

        wgpuComputePassEncoderDispatchWorkgroups(compute_pass, workgroupCountX, workgroupCountY, 1);

        wgpuBindGroupRelease(bind_group);

        RenderAPI::get_singleton()->pop_debug_group(compute_pass);

        // Finalize compute_raymarching pass
        wgpuComputePassEncoderEnd(compute_pass);

        wgpuComputePassEncoderRelease(compute_pass);
    }

    RenderAPI::get_singleton()->cubemap_mipmaps_create(cubemap_texture.get_texture(), cubemap_texture.get_size(), 6, WGPUTextureViewDimension_Cube, cubemap_texture.get_format(), { 0, 0, 0 }, command_encoder);

    // copy first cubemap mipmap to final texture
    RenderAPI::get_singleton()->texture_copy(cubemap_texture.get_texture(), prefiltered_env_texture->get_texture(), 0, 0, cubemap_texture.get_size(), { 0, 0, 0 }, { 0, 0, 0 }, command_encoder);

    // Prefilter cubemap
    {
        WGPUBindGroup bind_groups[5];

        for (uint32_t i = 0; i < 5; ++i) {
            std::vector<Uniform*> uniforms = { &cubemap_all_input_output_uniform, &cubemap_mipmaps_uniforms[i], &sampler, &current_level_uniform, &mipmaps_face_size_uniforms[i] };
            bind_groups[i] = RenderAPI::get_singleton()->bind_group_create(uniforms, prefiltered_env_shader, 0);
        }

        prefilter_env_uniform_data.mip_level_count = 6;

        // Setup uniform buffers
        for (uint32_t i = 0; i < 5; ++i) {
            prefilter_env_uniform_data.current_mip_level = i + 1;
            wgpuQueueWriteBuffer(prefilter_queue, std::get<WGPUBuffer>(current_level_uniform.data), i * buffer_stride, &prefilter_env_uniform_data, sizeof(PrefilterEnvUniformData));
        }

        WGPUComputePassDescriptor compute_pass_desc = { .label = { "prefilter_env_pass", WGPU_STRLEN } };
        compute_pass_desc.timestampWrites = nullptr;
        WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

        RenderAPI::get_singleton()->push_debug_group(compute_pass, { "Prefilter Cubemap", WGPU_STRLEN });

        prefiltered_env_pipeline.set(compute_pass);

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

        RenderAPI::get_singleton()->pop_debug_group(compute_pass);

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
