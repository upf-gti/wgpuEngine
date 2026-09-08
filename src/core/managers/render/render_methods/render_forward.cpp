#include "render_forward.h"

#include "core/managers/engine/engine_manager.h"
#include "core/managers/render/render_api.h"
#include "core/managers/render/render_manager.h"
#include "core/managers/render/render_storage.h"
#include "core/managers/xr/xr_manager.h"

#include "scene/3d/gs_node.h"

#include "graphics/mesh.h"
#include "graphics/pipeline.h"
#include "graphics/shader.h"
#include "graphics/texture.h"
#include "graphics/uniform.h"

#include "framework/camera/camera_2d.h"
#include "framework/camera/camera_3d.h"

#include "shaders/brdf_lut_gen.wgsl.gen.h"
#include "shaders/gaussian_splatting/gs_render.wgsl.gen.h"
#include "shaders/mesh_forward.wgsl.gen.h"
#include "shaders/mesh_shadow.wgsl.gen.h"
#include "shaders/mesh_texture_cube.wgsl.gen.h"
#include "shaders/panorama_to_cubemap.wgsl.gen.h"
#include "shaders/prefilter_env.wgsl.gen.h"
#include "shaders/quad_mirror.wgsl.gen.h"

#include "webgpu/webgpu.h"

#include "backends/imgui_impl_wgpu.h"
#include "imgui.h"
#include "spdlog/spdlog.h"

#include <algorithm>

Error RenderForward::initialize()
{
    if (!irradiance_texture) {
        irradiance_texture = RenderStorage::get_singleton()->get_texture("data/textures/environments/sky.hdr");
    }

    init_depth_buffers();
    init_lighting_bind_group();
    init_camera_bind_group();
    init_multisample_textures();

    {
        panorama_to_cubemap_shader = RenderStorage::get_singleton()->get_shader_from_source(shaders::panorama_to_cubemap::source, shaders::panorama_to_cubemap::path, shaders::panorama_to_cubemap::libraries);
        panorama_to_cubemap_pipeline.create_compute(panorama_to_cubemap_shader);
    }

    {
        std::vector<std::string> defines;
#if defined(BACKEND_METAL) || defined(BACKEND_EMSCRIPTEN)
        defines.push_back("METAL_CUBEMAP_HACK");
#endif
        prefiltered_env_shader = RenderStorage::get_singleton()->get_shader_from_source(shaders::prefilter_env::source, shaders::prefilter_env::path, shaders::prefilter_env::libraries, defines);
        prefiltered_env_pipeline.create_compute(prefiltered_env_shader);
    }

    {
        brdf_lut_shader = RenderStorage::get_singleton()->get_shader_from_source(shaders::brdf_lut_gen::source, shaders::brdf_lut_gen::path, shaders::brdf_lut_gen::libraries);
        brdf_lut_pipeline.create_compute(brdf_lut_shader);
        brdf_lut_texture.create(WGPUTextureDimension_2D, WGPUTextureFormat_RG32Float, { 512, 512, 1 },
                static_cast<WGPUTextureUsage>(WGPUTextureUsage_StorageBinding | WGPUTextureUsage_TextureBinding), 1, 1, nullptr);

        generate_brdf_lut_texture();
    }

    bool xr_available = XRManager::get_singleton()->is_xr_available();

    if (xr_available) {
        init_mirror_pipeline();
    }

    WGPUTextureFormat swapchain_format = xr_available ? XRManager::get_singleton()->get_swapchain_format() : RenderManager::get_singleton()->get_surface_format();

    WGPUColorTargetState color_target = {};
    color_target.format = swapchain_format;
    color_target.blend = nullptr;
    color_target.writeMask = WGPUColorWriteMask_All;

    RenderPipelineDescription desc = { .topology = WGPUPrimitiveTopology_TriangleStrip };

    WGPUBlendState* blend_state = new WGPUBlendState;
    blend_state->color = {
        .operation = WGPUBlendOperation_Add,
        .srcFactor = WGPUBlendFactor_SrcAlpha,
        .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
    };
    blend_state->alpha = {
        .operation = WGPUBlendOperation_Add,
        .srcFactor = WGPUBlendFactor_Zero,
        .dstFactor = WGPUBlendFactor_One,
    };

    color_target.blend = blend_state;

    msaa_count = EngineManager::get_singleton()->get_configuration().render_config.msaa_count;

    desc.depth_write = WGPUOptionalBool_False;
    desc.blending_enabled = true;
    desc.sample_count = msaa_count;

    gs_render_shader = RenderStorage::get_singleton()->get_shader_from_source(shaders::gs_render::source, shaders::gs_render::path, shaders::gs_render::libraries);
    gs_render_pipeline.create_render_async(gs_render_shader, color_target, desc);

    shadow_material = new Material();
    shadow_material->set_type(MATERIAL_UNLIT);
    shadow_material->set_fragment_write(false);
    shadow_material->set_shader(RenderStorage::get_singleton()->get_shader_from_source(shaders::mesh_shadow::source, shaders::mesh_shadow::path, shaders::mesh_shadow::libraries, shadow_material));

    // Orthographic camera for ui rendering

    uint32_t render_width = static_cast<uint32_t>(RenderManager::get_singleton()->get_render_width());
    uint32_t render_height = static_cast<uint32_t>(RenderManager::get_singleton()->get_render_height());

    camera_2d = new Camera2D();
    camera_2d->set_orthographic(0.0f, static_cast<float>(render_width), static_cast<float>(render_height), 0.0f, -1.0f, 1.0f);

    skybox_material = new Material();
    skybox_material->set_diffuse_texture(irradiance_texture);
    skybox_material->set_cull_type(CULL_BACK);
    skybox_material->set_type(MATERIAL_UNLIT);
    skybox_material->set_depth_write(false);
    skybox_material->set_priority(20);
    skybox_material->set_shader(RenderStorage::get_singleton()->get_shader_from_source(shaders::mesh_texture_cube::source, shaders::mesh_texture_cube::path, shaders::mesh_texture_cube::libraries, skybox_material));

    skybox_surface = new Surface();
    skybox_surface->create_skybox();
    skybox_surface->set_material(skybox_material);

    skybox_mesh = new Mesh();
    skybox_mesh->add_surface(skybox_surface);
    skybox_mesh->set_frustum_culling_enabled(false);

    return Error::OK;
}

Error RenderForward::finalize()
{
    return Error::OK;
}

void RenderForward::render_opaque(WGPURenderPassEncoder render_pass, const std::vector<std::vector<sRenderData>>& render_lists, const sInstanceData& instance_data, WGPUBindGroup camera_bind_group, uint32_t camera_buffer_stride)
{
#ifndef NDEBUG
    RenderAPI::get_singleton()->push_debug_group(render_pass, "Opaque");
#endif

    render_render_list(render_pass, render_lists[RENDER_LIST_OPAQUE], RENDER_LIST_OPAQUE, instance_data, camera_bind_group, camera_buffer_stride);

#ifndef NDEBUG
    RenderAPI::get_singleton()->pop_debug_group(render_pass);
#endif
}

void RenderForward::render_transparent(WGPURenderPassEncoder render_pass, const std::vector<std::vector<sRenderData>>& render_lists, const sInstanceData& instance_data, WGPUBindGroup camera_bind_group, uint32_t camera_buffer_stride)
{
#ifndef NDEBUG
    RenderAPI::get_singleton()->push_debug_group(render_pass, "Transparent");
#endif

    render_render_list(render_pass, render_lists[RENDER_LIST_TRANSPARENT], RENDER_LIST_TRANSPARENT, instance_data, camera_bind_group, camera_buffer_stride);

#ifndef NDEBUG
    RenderAPI::get_singleton()->pop_debug_group(render_pass);
#endif
}

void RenderForward::render_splats(WGPURenderPassEncoder render_pass, const std::vector<std::vector<sRenderData>>& render_lists, const sInstanceData& instance_data, WGPUBindGroup camera_bind_group, uint32_t camera_buffer_stride)
{
#ifndef NDEBUG
    RenderAPI::get_singleton()->push_debug_group(render_pass, "Gaussian Splats");
#endif

    for (GSNode* gs_node : gs_scenes_list) {
        if (!gs_render_pipeline.set(render_pass)) {
            continue;
        }

        RenderAPI::get_singleton()->render_pass_set_bind_group(render_pass, 0, gs_node->get_render_bindgroup(), 0, nullptr);
        RenderAPI::get_singleton()->render_pass_set_bind_group(render_pass, 1, camera_bind_group, 1, &camera_buffer_stride);

        RenderAPI::get_singleton()->render_pass_encoder_set_vertex_buffer(render_pass, 0, gs_node->get_render_buffer(), 0, gs_node->get_splats_render_bytes_size());
        RenderAPI::get_singleton()->render_pass_encoder_set_vertex_buffer(render_pass, 1, gs_node->get_ids_buffer(), 0, gs_node->get_ids_render_bytes_size());

        RenderAPI::get_singleton()->render_pass_encoder_draw(render_pass, 4, gs_node->get_splat_count(), 0, 0);
    }

#ifndef NDEBUG
    RenderAPI::get_singleton()->pop_debug_group(render_pass);
#endif
}

void RenderForward::render_2D(WGPURenderPassEncoder render_pass, const std::vector<std::vector<sRenderData>>& render_lists, const sInstanceData& instance_data, WGPUBindGroup camera_bind_group)
{
#ifndef NDEBUG
    RenderAPI::get_singleton()->push_debug_group(render_pass, "2D");
#endif

    render_render_list(render_pass, render_lists[RENDER_LIST_2D], RENDER_LIST_2D, instance_data, camera_bind_group);

    render_render_list(render_pass, render_lists[RENDER_LIST_2D_TRANSPARENT], RENDER_LIST_2D_TRANSPARENT, instance_data, camera_bind_group);

#ifndef NDEBUG
    RenderAPI::get_singleton()->pop_debug_group(render_pass);
#endif
}

void RenderForward::render_render_list(WGPURenderPassEncoder render_pass, const std::vector<sRenderData>& render_list, int list_index, const sInstanceData& instance_data, WGPUBindGroup camera_bind_group, uint32_t camera_buffer_stride)
{
    const Pipeline* prev_pipeline = nullptr;

    RenderAPI::get_singleton()->render_pass_set_bind_group(render_pass, 0, instance_data.instances_bind_groups[list_index], 0, nullptr);
    RenderAPI::get_singleton()->render_pass_set_bind_group(render_pass, 1, camera_bind_group, 1, &camera_buffer_stride);

    const Material* prev_material = nullptr;

    for (int i = 0; i < render_list.size();) {
        const sRenderData& render_data = render_list[i];

        const Material* material = render_data.material;

        if (!material) {
            assert(0);
            continue;
        }

        const Pipeline* pipeline = material->get_shader()->get_pipeline();

        assert(pipeline);

        if (pipeline != prev_pipeline) {
            if (!pipeline->set(render_pass)) {
                i += render_data.repeat;
                continue;
            }
        }

        // Not initialized
        if (render_data.surface->get_vertex_count() == 0) {
            spdlog::error("Skipping not initialized mesh");
            continue;
        }

        // Set bind groups

        if (material != prev_material && (material->get_fragment_write() || (!material->get_fragment_write() && material->get_use_skinning()))) {
            RenderAPI::get_singleton()->render_pass_set_bind_group(render_pass, 2, RenderStorage::get_singleton()->get_material_bind_group(material), 0, nullptr);

            if ((!prev_material && material->get_type() == MATERIAL_PBR) ||
                    (prev_material && prev_material->get_type() != MATERIAL_PBR && material->get_type() == MATERIAL_PBR)) {
                RenderAPI::get_singleton()->render_pass_set_bind_group(render_pass, 3, lighting_bind_group, 0, nullptr);
            }

            prev_material = material;
        }

        //#ifndef NDEBUG
        //        webgpu_context->push_debug_group(render_pass, render_data.surface->get_name().c_str());
        //#endif

        if (material->get_type() == MATERIAL_UI) {
            WGPUBindGroup ui_bind_group = RenderStorage::get_singleton()->get_ui_widget_bind_group(render_data.mesh_ref);
            if (ui_bind_group) {
                RenderAPI::get_singleton()->render_pass_set_bind_group(render_pass, 3, ui_bind_group, 0, nullptr);
            }
        }

        // Set vertex buffer while encoding the render pass
        RenderAPI::get_singleton()->render_pass_encoder_set_vertex_buffer(render_pass, 0, render_data.surface->get_vertex_buffer(), 0, render_data.surface->get_vertices_byte_size());

        if (material->get_fragment_write()) {
            RenderAPI::get_singleton()->render_pass_encoder_set_vertex_buffer(render_pass, 1, render_data.surface->get_vertex_data_buffer(), 0, render_data.surface->get_interleaved_data_byte_size());
        }

        WGPUBuffer index_buffer = render_data.surface->get_index_buffer();

        if (index_buffer) {
            RenderAPI::get_singleton()->render_pass_encoder_set_index_buffer(render_pass, index_buffer, WGPUIndexFormat_Uint32, 0, render_data.surface->get_indices_byte_size());

            RenderAPI::get_singleton()->render_pass_encoder_draw_indexed(render_pass, render_data.surface->get_index_count(), render_data.repeat, 0, 0, i);
        } else {
            RenderAPI::get_singleton()->render_pass_encoder_draw(render_pass, render_data.surface->get_vertex_count(), render_data.repeat, 0, i);
        }

        //#ifndef NDEBUG
        //        webgpu_context->pop_debug_group(render_pass);
        //#endif

        prev_pipeline = pipeline;

        i += render_data.repeat;
    }
}

void RenderForward::render_scene(const std::vector<std::vector<sRenderData>>& render_lists)
{
    camera_data.right_controller_position = camera_data.eye;
    camera_data.eye = camera_3d->get_eye();
    camera_data.view_projection = camera_3d->get_view_projection();
    camera_data.view = camera_3d->get_view();
    camera_data.projection = camera_3d->get_projection();

    wgpuQueueWriteBuffer(RenderAPI::get_singleton()->get_main_queue(), std::get<WGPUBuffer>(camera_uniform.data), 0, &camera_data, sizeof(sCameraData));

    // Prepare the color attachment
    WGPURenderPassColorAttachment render_pass_color_attachment = {};

    if (framebuffer_view) {
        if (msaa_count > 1) {
            render_pass_color_attachment.view = multisample_textures_views[eye_idx];
            render_pass_color_attachment.resolveTarget = framebuffer_view;
        } else {
            render_pass_color_attachment.view = framebuffer_view;
        }

        render_pass_color_attachment.loadOp = WGPULoadOp_Clear;
        render_pass_color_attachment.storeOp = WGPUStoreOp_Store;
        render_pass_color_attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        render_pass_color_attachment.clearValue = WGPUColor{ clear_color.r, clear_color.g, clear_color.b, clear_color.a };
    }

    // Prepate the depth attachment
    WGPURenderPassDepthStencilAttachment render_pass_depth_attachment = {};

    if (depth_view) {
        render_pass_depth_attachment.view = depth_view;
        render_pass_depth_attachment.depthClearValue = 0.0f;
        render_pass_depth_attachment.depthLoadOp = WGPULoadOp_Clear;
        render_pass_depth_attachment.depthStoreOp = WGPUStoreOp_Store;
        render_pass_depth_attachment.depthReadOnly = false;
        render_pass_depth_attachment.stencilClearValue = 0; // Stencil config necesary, even if unused
        render_pass_depth_attachment.stencilLoadOp = WGPULoadOp_Undefined;
        render_pass_depth_attachment.stencilStoreOp = WGPUStoreOp_Undefined;
        render_pass_depth_attachment.stencilReadOnly = true;
    }

    WGPURenderPassDescriptor render_pass_descr = {};
    render_pass_descr.colorAttachmentCount = framebuffer_view ? 1 : 0;
    render_pass_descr.colorAttachments = framebuffer_view ? &render_pass_color_attachment : nullptr;
    render_pass_descr.depthStencilAttachment = depth_view ? &render_pass_depth_attachment : nullptr;
    render_pass_descr.label = { pass_name.c_str(), pass_name.length() };

    std::vector<WGPUPassTimestampWrites> timestampWrites(1);
    timestampWrites[0].beginningOfPassWriteIndex = RenderManager::get_singleton()->timestamp(RenderAPI::get_singleton()->get_main_command_encoder(), (pass_name + "_pre_render").c_str());
    timestampWrites[0].querySet = RenderManager::get_singleton()->get_query_set();
    timestampWrites[0].endOfPassWriteIndex = RenderManager::get_singleton()->timestamp(RenderAPI::get_singleton()->get_main_command_encoder(), (pass_name + "_render").c_str());

    render_pass_descr.timestampWrites = timestampWrites.data();

    // Create & fill the render pass (encoder)
    WGPURenderPassEncoder render_pass = RenderAPI::get_singleton()->begin_render_pass(RenderAPI::get_singleton()->get_main_command_encoder(), &render_pass_descr);

#ifndef NDEBUG
    RenderAPI::get_singleton()->push_debug_group(render_pass, pass_name.c_str());
#endif

    if (custom_pre_opaque_pass) {
        custom_pre_opaque_pass(render_pass, camera_bind_group, custom_pass_user_data, camera_offset * camera_buffer_stride);
    }

    render_opaque(render_pass, render_lists, instance_data, camera_bind_group, camera_offset * camera_buffer_stride);

    if (custom_post_opaque_pass) {
        custom_post_opaque_pass(render_pass, camera_bind_group, custom_pass_user_data, camera_offset * camera_buffer_stride);
    }

    if (render_transparents) {
        if (custom_pre_transparent_pass) {
            custom_pre_transparent_pass(render_pass, camera_bind_group, custom_pass_user_data, camera_offset * camera_buffer_stride);
        }

        render_transparent(render_pass, render_lists, instance_data, camera_bind_group, camera_offset * camera_buffer_stride);

        if (custom_post_transparent_pass) {
            custom_post_transparent_pass(render_pass, camera_bind_group, custom_pass_user_data, camera_offset * camera_buffer_stride);
        }

        render_splats(render_pass, render_lists, instance_data, camera_bind_group, camera_offset * camera_buffer_stride);
    }

#ifndef NDEBUG
    RenderAPI::get_singleton()->pop_debug_group(render_pass);
#endif

    RenderAPI::get_singleton()->render_pass_encoder_end(render_pass);

    RenderAPI::get_singleton()->render_pass_encoder_release(render_pass);
}

void RenderForward::resize_swapchain()
{
    init_depth_buffers();
    init_multisample_textures();
}

void RenderForward::set_msaa_count(uint8_t msaa_count)
{
    bool recreate = msaa_count != this->msaa_count && multisample_textures[0].get_texture() != nullptr;

    this->msaa_count = msaa_count;

    if (recreate) {
        init_depth_buffers();
        init_multisample_textures();
    }

    RenderStorage::get_singleton()->reload_all_render_pipelines();
}

void RenderForward::set_custom_pass_user_data(void* user_data)
{
    custom_pass_user_data = user_data;
}

void RenderForward::init_depth_buffers()
{
    uint32_t render_width = static_cast<uint32_t>(RenderManager::get_singleton()->get_render_width());
    uint32_t render_height = static_cast<uint32_t>(RenderManager::get_singleton()->get_render_height());

    if (render_width == 0 || render_height == 0) {
        spdlog::error("Can not create depth buffer with size ({}, {})", render_width, render_height);
        return;
    }

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

void RenderForward::init_camera_bind_group()
{
    camera_buffer_stride = std::max(static_cast<uint32_t>(sizeof(sCameraData)), RenderAPI::get_singleton()->get_supported_limits().minUniformBufferOffsetAlignment);

    camera_uniform.data = RenderAPI::get_singleton()->buffer_create(camera_buffer_stride * EYE_COUNT, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, nullptr, "camera_buffer");
    camera_uniform.binding = 0;
    camera_uniform.buffer_size = sizeof(sCameraData);

    camera_2d_uniform.data = RenderAPI::get_singleton()->buffer_create(sizeof(sCameraData), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, nullptr, "camera_2d_buffer");
    camera_2d_uniform.binding = 0;
    camera_2d_uniform.buffer_size = sizeof(sCameraData);

    shadow_camera_uniform.data = RenderAPI::get_singleton()->buffer_create(camera_buffer_stride * shadow_uniform_buffer_size, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, nullptr, "shadow_camera_buffer");
    shadow_camera_uniform.binding = 0;
    shadow_camera_uniform.buffer_size = sizeof(sCameraData);

    std::vector<Uniform*> uniforms = { &camera_uniform };
    render_camera_bind_group = RenderAPI::get_singleton()->bind_group_create(uniforms, RenderStorage::get_singleton()->get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, shaders::mesh_forward::libraries), 1);

    uniforms = { &shadow_camera_uniform };
    shadow_camera_bind_group = RenderAPI::get_singleton()->bind_group_create(uniforms, RenderStorage::get_singleton()->get_shader_from_source(shaders::mesh_shadow::source, shaders::mesh_shadow::path, shaders::mesh_shadow::libraries), 1);

    WGPUBindGroupLayoutEntry entry = {};
    entry.binding = 0;
    entry.buffer.type = WGPUBufferBindingType_Uniform;
    entry.buffer.hasDynamicOffset = true;
    entry.visibility = WGPUShaderStage_Compute;

    uniforms = { &camera_uniform };

    WGPUBindGroupLayout compute_camera_bind_group_layout = RenderAPI::get_singleton()->create_bind_group_layout({ entry });
    compute_camera_bind_group = RenderAPI::get_singleton()->bind_group_create(uniforms, compute_camera_bind_group_layout);

    uniforms = { &camera_2d_uniform };
    render_camera_bind_group_2d = RenderAPI::get_singleton()->bind_group_create(uniforms, RenderStorage::get_singleton()->get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, shaders::mesh_forward::libraries), 1);
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
    lighting_bind_group = RenderAPI::get_singleton()->bind_group_create(uniforms, RenderStorage::get_singleton()->get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, shaders::mesh_forward::libraries), 3);
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

        WGPUComputePassDescriptor compute_pass_desc = { .label = get_string_view("panorama_to_cubemap_pass") };
        compute_pass_desc.timestampWrites = nullptr;
        WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

        RenderAPI::get_singleton()->push_debug_group(compute_pass, "Panorama to Cubemap");

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

        WGPUComputePassDescriptor compute_pass_desc = { .label = get_string_view("prefilter_env_pass") };
        compute_pass_desc.timestampWrites = nullptr;
        WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

        RenderAPI::get_singleton()->push_debug_group(compute_pass, "Prefilter Cubemap");

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
    cmd_buff_descriptor.label = get_string_view("Create Prefiltered Env Command Buffer");

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

void RenderForward::init_mirror_pipeline()
{
    mirror_shader = RenderStorage::get_singleton()->get_shader_from_source(shaders::quad_mirror::source, shaders::quad_mirror::path, shaders::quad_mirror::libraries);

    quad_surface.create_quad(2.0f, 2.0f);

    WGPUTextureFormat swapchain_format = RenderManager::get_singleton()->get_surface_format();

    WGPUColorTargetState color_target = {};
    color_target.format = swapchain_format;
    color_target.blend = nullptr;
    color_target.writeMask = WGPUColorWriteMask_All;

    // Generate uniforms from the swapchain
    for (uint8_t i = 0; i < XRManager::get_singleton()->get_num_images_per_swapchain(); i++) {
        Uniform swapchain_uni;

        swapchain_uni.data = XRManager::get_singleton()->get_swapchain_view(EYE_LEFT, i);
        swapchain_uni.binding = 0;

        swapchain_uniforms.push_back(swapchain_uni);
    }

    linear_sampler_uniform.data = RenderAPI::get_singleton()->sampler_create(WGPUAddressMode_ClampToEdge, WGPUAddressMode_ClampToEdge, WGPUAddressMode_ClampToEdge, WGPUFilterMode_Linear, WGPUFilterMode_Linear);
    linear_sampler_uniform.binding = 1;

    // Generate bindgroups from the swapchain
    for (uint8_t i = 0; i < swapchain_uniforms.size(); i++) {
        Uniform swapchain_uni;

        std::vector<Uniform*> uniforms = { &swapchain_uniforms[i], &linear_sampler_uniform };

        swapchain_bind_groups.push_back(RenderAPI::get_singleton()->bind_group_create(uniforms, mirror_shader, 0));
    }

    mirror_pipeline.create_render(mirror_shader, color_target, { .use_depth = false, .allow_msaa = false });
}

void RenderForward::render_mirror(WGPUTextureView screen_surface_texture_view, WGPUBindGroup displayed_fbo_bind_group)
{
    ImGui::Render();

    // Create & fill the render pass (encoder)
    {
        // Prepare the color attachment
        WGPURenderPassColorAttachment render_pass_color_attachment = {};
        render_pass_color_attachment.view = screen_surface_texture_view;
        render_pass_color_attachment.loadOp = WGPULoadOp_Clear;
        render_pass_color_attachment.storeOp = WGPUStoreOp_Store;
        render_pass_color_attachment.clearValue = WGPUColor(clear_color.x, clear_color.y, clear_color.z, 1.0f);
        render_pass_color_attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

        WGPURenderPassDescriptor render_pass_descr = {};
        render_pass_descr.colorAttachmentCount = 1;
        render_pass_descr.colorAttachments = &render_pass_color_attachment;
        render_pass_descr.depthStencilAttachment = nullptr;

        {
            WGPURenderPassEncoder render_pass = RenderAPI::get_singleton()->begin_render_pass(RenderAPI::get_singleton()->get_main_command_encoder(), &render_pass_descr);

            RenderAPI::get_singleton()->push_debug_group(render_pass, "Mirror");

            // Bind Pipeline
            if (!mirror_pipeline.set(render_pass)) {
                RenderAPI::get_singleton()->render_pass_encoder_end(render_pass);
                RenderAPI::get_singleton()->render_pass_encoder_release(render_pass);
                return;
            }

            // Set binding group
            RenderAPI::get_singleton()->render_pass_set_bind_group(render_pass, 0, displayed_fbo_bind_group, 0, nullptr);

            // Set vertex buffer while encoding the render pass
            RenderAPI::get_singleton()->render_pass_encoder_set_vertex_buffer(render_pass, 0, quad_surface.get_vertex_buffer(), 0, quad_surface.get_vertices_byte_size());
            RenderAPI::get_singleton()->render_pass_encoder_set_vertex_buffer(render_pass, 1, quad_surface.get_vertex_data_buffer(), 0, quad_surface.get_interleaved_data_byte_size());

            // Submit drawcall
            RenderAPI::get_singleton()->render_pass_encoder_draw(render_pass, 6, 1, 0, 0);

            RenderAPI::get_singleton()->pop_debug_group(render_pass);

            RenderAPI::get_singleton()->render_pass_encoder_end(render_pass);
            RenderAPI::get_singleton()->render_pass_encoder_release(render_pass);
        }
    }

    // render imgui
    {
        WGPURenderPassColorAttachment color_attachments = {};
        color_attachments.view = screen_surface_texture_view;
        color_attachments.loadOp = WGPULoadOp_Load;
        color_attachments.storeOp = WGPUStoreOp_Store;
        color_attachments.clearValue = { 0.0, 0.0, 0.0, 0.0 };
        color_attachments.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

        WGPURenderPassDescriptor render_pass_desc = {};
        render_pass_desc.colorAttachmentCount = 1;
        render_pass_desc.colorAttachments = &color_attachments;
        render_pass_desc.depthStencilAttachment = nullptr;

        WGPURenderPassEncoder pass = RenderAPI::get_singleton()->begin_render_pass(RenderAPI::get_singleton()->get_main_command_encoder(), &render_pass_desc);

        RenderAPI::get_singleton()->push_debug_group(pass, "ImGui");

        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);

        RenderAPI::get_singleton()->pop_debug_group(pass);

        RenderAPI::get_singleton()->render_pass_encoder_end(pass);
        RenderAPI::get_singleton()->render_pass_encoder_release(pass);
    }
}

void RenderForward::update_lights()
{
    uint64_t buffer_size = sizeof(sLightUniformData) * num_lights;

    RenderAPI::get_singleton()->buffer_update(std::get<WGPUBuffer>(lights_buffer.data), 0, &lights_uniform_data[0], buffer_size);
    RenderAPI::get_singleton()->buffer_update(std::get<WGPUBuffer>(num_lights_buffer.data), 0, &num_lights, sizeof(int));
}
