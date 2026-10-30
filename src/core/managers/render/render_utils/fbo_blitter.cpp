#include "fbo_blitter.h"

#include "core/managers/debug/debug_manager.h"
#include "core/managers/render/render_manager.h"

#include "shaders/blitter.wgsl.gen.h"

Error FBOBlitter::initialize(WGPUTextureView source_texture_view, WGPUTextureFormat target_fbo_format, WGPUFilterMode filter_mode, bool apply_gamma)
{
    if (!source_texture_view) {
        LOG_ERROR("Could not initialize blitter with invalid source_texture_view");
        return Error::FAILED;
    }

    this->source_texture_view = source_texture_view;
    this->filter_mode = filter_mode;

    WGPUColorTargetState color_target = {};
    color_target.format = target_fbo_format;
    color_target.blend = nullptr;
    color_target.writeMask = WGPUColorWriteMask_All;

    std::vector<std::string> defines;
    if (apply_gamma) {
        defines.push_back("APPLY_GAMMA");
    }

    blitter_shader = RenderStorage::get_singleton()->get_shader_from_source(shaders::blitter::source, shaders::blitter::path, shaders::blitter::libraries, defines);
    blitter_pipeline.create_render(blitter_shader, color_target, { .use_depth = false, .allow_msaa = false });

    quad_surface.create_quad(2.0f, 2.0f);

    {
        Uniform* u = new Uniform();
        u->data = source_texture_view;
        u->binding = 0;
        blitter_uniforms.push_back(u);
    }

    {
        Uniform* sampler_uniform = new Uniform();
        sampler_uniform->data = RENDER_API->sampler_create(
                WGPUAddressMode_ClampToEdge,
                WGPUAddressMode_ClampToEdge,
                WGPUAddressMode_ClampToEdge,
                filter_mode,
                filter_mode,
                WGPUMipmapFilterMode_Linear,
                1);

        sampler_uniform->binding = 1;
        blitter_uniforms.push_back(sampler_uniform);
    }

    blit_bind_group = RENDER_API->bind_group_create(blitter_uniforms, blitter_shader, 0);

    return Error::OK;
}

Error FBOBlitter::finalize()
{
    RENDER_API->bind_group_release(blit_bind_group);

    for (Uniform* uniform : blitter_uniforms) {
        uniform->destroy();
        delete uniform;
    }

    blitter_uniforms.clear();

    return Error::OK;
}

void FBOBlitter::blit(WGPUTextureView target_fbo_view)
{
    if (!source_texture_view) {
        LOG_ERROR("No source texture set to blitter");
        return;
    }

    if (!blitter_shader || !blitter_shader->is_loaded()) {
        return;
    }

    WGPURenderPassColorAttachment color_attachments = {};
    color_attachments.view = target_fbo_view;
    color_attachments.loadOp = WGPULoadOp_Load;
    color_attachments.storeOp = WGPUStoreOp_Store;
    color_attachments.clearValue = { 0.0, 0.0, 0.0, 0.0 };
    color_attachments.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

    WGPURenderPassDescriptor render_pass_desc = {};
    render_pass_desc.colorAttachmentCount = 1;
    render_pass_desc.colorAttachments = &color_attachments;
    render_pass_desc.depthStencilAttachment = nullptr;

    WGPURenderPassEncoder render_pass = RENDER_API->command_encoder_begin_render_pass(RENDER_MANAGER->get_main_command_encoder(), &render_pass_desc);

    if (!blitter_pipeline.set(render_pass)) {
        LOG_ERROR("Could not set blitter pipeline");
        RENDER_API->render_pass_encoder_end(render_pass);
        RENDER_API->render_pass_encoder_release(render_pass);
        return;
    }

    RENDER_API->push_debug_group(render_pass, "Blitter");

    RENDER_API->render_pass_set_bind_group(render_pass, 0, blit_bind_group, 0, nullptr);

    RENDER_API->render_pass_encoder_set_vertex_buffer(render_pass, 0, quad_surface.get_vertex_buffer(), 0, quad_surface.get_vertices_byte_size());
    RENDER_API->render_pass_encoder_set_vertex_buffer(render_pass, 1, quad_surface.get_vertex_data_buffer(), 0, quad_surface.get_interleaved_data_byte_size());

    WGPUBuffer index_buffer = quad_surface.get_index_buffer();

    if (index_buffer) {
        RENDER_API->render_pass_encoder_set_index_buffer(render_pass, index_buffer, WGPUIndexFormat_Uint32, 0, quad_surface.get_indices_byte_size());

        RENDER_API->render_pass_encoder_draw_indexed(render_pass, quad_surface.get_index_count(), 1, 0, 0, 0);
    } else {
        RENDER_API->render_pass_encoder_draw(render_pass, quad_surface.get_vertex_count(), 1, 0, 0);
    }

    RENDER_API->pop_debug_group(render_pass);

    RENDER_API->render_pass_encoder_end(render_pass);
    RENDER_API->render_pass_encoder_release(render_pass);
}

void FBOBlitter::set_source_texture(WGPUTextureView source_texture_view)
{
    if (blitter_uniforms.empty()) {
        LOG_ERROR("Could not set source_texture_view to blitter (not initialized?)");
        return;
    }

    blitter_uniforms[0]->data = source_texture_view;

    RENDER_API->bind_group_release(blit_bind_group);
    blit_bind_group = RENDER_API->bind_group_create(blitter_uniforms, blitter_shader, 0);
}

void FBOBlitter::set_filter_mode(WGPUFilterMode filter_mode)
{
    this->filter_mode = filter_mode;

    if (blitter_uniforms.empty()) {
        LOG_ERROR("Could not set filter mode to blitter (not initialized?)");
        return;
    }

    RENDER_API->sampler_release(std::get<WGPUSampler>(blitter_uniforms[1]->data));

    blitter_uniforms[1]->data = RENDER_API->sampler_create(
            WGPUAddressMode_ClampToEdge,
            WGPUAddressMode_ClampToEdge,
            WGPUAddressMode_ClampToEdge,
            filter_mode,
            filter_mode,
            WGPUMipmapFilterMode_Linear,
            1);

    RENDER_API->bind_group_release(blit_bind_group);
    blit_bind_group = RENDER_API->bind_group_create(blitter_uniforms, blitter_shader, 0);
}
