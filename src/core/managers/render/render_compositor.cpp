#include "render_compositor.h"

#include "core/managers/debug/debug_manager.h"
#include "core/managers/render/render_api.h"
#include "core/managers/render/render_manager.h"
#include "core/managers/xr/xr_manager.h"

#include "backends/imgui_impl_wgpu.h"
#include "imgui.h"

Error RenderCompositor::initialize()
{
    XRContext* xr_context = XR_MANAGER->xr_context;
    blitter.initialize(XR_MANAGER->is_xr_available() ? xr_context->get_swapchain_view(0) : render_fbo_view, RENDER_MANAGER->get_surface_format(), WGPUFilterMode_Linear, XR_MANAGER->is_xr_available());

    return Error::OK;
}

Error RenderCompositor::finalize()
{
    RENDER_API->texture_release(render_fbo);
    RENDER_API->texture_view_release(render_fbo_view);

    blitter.finalize();

    return Error::OK;
}

void RenderCompositor::set_render_method(RenderMethod* method)
{
    render_cull.set_render_method(method);
}

RenderMethod* RenderCompositor::get_render_method()
{
    return render_cull.get_render_method();
}

void RenderCompositor::render(const std::vector<sRenderableData>& renderables_list, const sEnvironmentData& environment_data)
{
    WGPUTextureView screen_surface_texture_view;
    WGPUSurfaceTexture screen_surface_texture;

    wgpuSurfaceGetCurrentTexture(RenderAPI::get_singleton()->get_main_surface(), &screen_surface_texture);

    if (screen_surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal) {
        LOG_WARN("Suboptimal Surface Texture");
        ImGui::EndFrame();
        return;
    }

    screen_surface_texture_view = RenderAPI::get_singleton()->texture_view_create(screen_surface_texture.texture, WGPUTextureViewDimension_2D, RENDER_MANAGER->get_surface_format());

    XR_MANAGER->init_frame();

    // Render 3D elements
    render_cull.render(renderables_list, render_fbo_view, environment_data);

    if (XR_MANAGER->is_xr_available()) {
        // TODO: this is internally recreating the bind group per frame, it's not terrible but we should think on a better way
        // there is also something wrong in the order we fetch the swapchain, visually the image it's not as smooth as it should be
        blitter.set_source_texture(XR_MANAGER->get_swapchain_view(EYE_LEFT));
    }

    blitter.blit(screen_surface_texture_view);

    // TODO: Render 2D elements

    render_imgui(screen_surface_texture_view);

    RENDER_MANAGER->resolve_query_set(RENDER_MANAGER->get_main_command_encoder(), 0);

    WGPUCommandBufferDescriptor cmd_buff_descriptor = {};
    cmd_buff_descriptor.nextInChain = NULL;
    cmd_buff_descriptor.label = { "Command buffer", WGPU_STRLEN };

    WGPUCommandBuffer commands = RENDER_API->command_encoder_finish(RENDER_MANAGER->get_main_command_encoder(), &cmd_buff_descriptor);

    RENDER_API->queue_submit(RenderAPI::get_singleton()->get_main_queue(), 1, &commands);

    RENDER_API->command_buffer_release(commands);

    XR_MANAGER->end_frame();

#ifdef WEBXR_SUPPORT
    for (uint32_t eye_idx = 0; eye_idx < EYE_COUNT; eye_idx++) {
        wgpuTextureViewRelease(xr_context->get_swapchain_view(eye_idx));
    }
#endif // WEBXR_SUPPORT

    RENDER_API->texture_view_release(screen_surface_texture_view);
    RENDER_API->texture_release(screen_surface_texture.texture);

#ifndef __EMSCRIPTEN__
    RENDER_API->surface_present(RenderAPI::get_singleton()->get_main_surface());
#endif
}

void RenderCompositor::set_render_size(int32_t width, int32_t height)
{
    init_render_fbo(width, height);
    render_cull.set_render_size(width, height);

    if (!XR_MANAGER->is_xr_available()) {
        blitter.set_source_texture(render_fbo_view);
    }
}

void RenderCompositor::render_imgui(WGPUTextureView screen_surface_texture_view)
{
    ImGui::Render();

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

    WGPURenderPassEncoder pass = RENDER_API->command_encoder_begin_render_pass(RENDER_MANAGER->get_main_command_encoder(), &render_pass_desc);

    RENDER_API->push_debug_group(pass, "ImGui");

    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);

    RENDER_API->pop_debug_group(pass);

    RENDER_API->render_pass_encoder_end(pass);
    RENDER_API->render_pass_encoder_release(pass);

    ImGui::EndFrame();
}

void RenderCompositor::init_render_fbo(int32_t width, int32_t height)
{
    if (render_fbo) {
        RENDER_API->texture_release(render_fbo);
        RENDER_API->texture_view_release(render_fbo_view);
    }

    uint32_t render_width = static_cast<uint32_t>(width);
    uint32_t render_height = static_cast<uint32_t>(height);

    WGPUTextureFormat texture_format = XR_MANAGER->is_xr_available() ? XR_MANAGER->get_swapchain_format() : RENDER_MANAGER->get_surface_format();

    render_fbo = RENDER_API->texture_create(
            WGPUTextureDimension_2D,
            texture_format,
            { render_width, render_height, 1 },
            WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding,
            1,
            1,
            "render_fbo");

    render_fbo_view = RENDER_API->texture_view_create(render_fbo, WGPUTextureViewDimension_2D, texture_format, WGPUTextureAspect_All, 0, 1, 0, 1, "render_fbo_view");
}
