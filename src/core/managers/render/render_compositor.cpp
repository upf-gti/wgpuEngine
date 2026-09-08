#include "render_compositor.h"

#include "core/managers/debug/debug_manager.h"
#include "core/managers/render/render_api.h"
#include "core/managers/render/render_manager.h"
#include "core/managers/xr/xr_manager.h"

#include "backends/imgui_impl_wgpu.h"
#include "imgui.h"

Error RenderCompositor::initialize()
{
    return Error::OK;
}

Error RenderCompositor::finalize()
{
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
    bool xr_available = XR_MANAGER->is_xr_available();

    WGPUTextureView screen_surface_texture_view;
    WGPUSurfaceTexture screen_surface_texture;

    wgpuSurfaceGetCurrentTexture(RenderAPI::get_singleton()->get_main_surface(), &screen_surface_texture);

    if (screen_surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal) {
        LOG_WARN("Suboptimal Surface Texture");
        RenderAPI::get_singleton()->surface_configure_swapchain(RENDER_MANAGER->get_render_width(), RENDER_MANAGER->get_render_height());
        ImGui::EndFrame();
        return;
    }

    screen_surface_texture_view = RenderAPI::get_singleton()->texture_view_create(screen_surface_texture.texture, WGPUTextureViewDimension_2D, RENDER_MANAGER->get_surface_format());

    // Render 3D elements
    render_cull.render(renderables_list, screen_surface_texture_view, environment_data);

    // TODO: Render 2D elements

    ImGui::Render();

    if (!xr_available) {
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

        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(RENDER_MANAGER->get_main_command_encoder(), &render_pass_desc);

        RenderAPI::get_singleton()->push_debug_group(pass, "ImGui");

        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);

        RenderAPI::get_singleton()->pop_debug_group(pass);

        wgpuRenderPassEncoderEnd(pass);
        wgpuRenderPassEncoderRelease(pass);
    }

    ImGui::EndFrame();

    RENDER_MANAGER->resolve_query_set(RENDER_MANAGER->get_main_command_encoder(), 0);

    WGPUCommandBufferDescriptor cmd_buff_descriptor = {};
    cmd_buff_descriptor.nextInChain = NULL;
    cmd_buff_descriptor.label = { "Command buffer", WGPU_STRLEN };

    WGPUCommandBuffer commands = wgpuCommandEncoderFinish(RENDER_MANAGER->get_main_command_encoder(), &cmd_buff_descriptor);

    wgpuQueueSubmit(RenderAPI::get_singleton()->get_main_queue(), 1, &commands);

    wgpuCommandBufferRelease(commands);
    wgpuCommandEncoderRelease(RENDER_MANAGER->get_main_command_encoder());

#ifdef WEBXR_SUPPORT
    for (uint32_t eye_idx = 0; eye_idx < EYE_COUNT; eye_idx++) {
        wgpuTextureViewRelease(xr_context->get_swapchain_view(eye_idx));
    }
#endif // WEBXR_SUPPORT

    wgpuTextureViewRelease(screen_surface_texture_view);
    wgpuTextureRelease(screen_surface_texture.texture);

#ifndef __EMSCRIPTEN__
    wgpuSurfacePresent(RenderAPI::get_singleton()->get_main_surface());
#endif
}

void RenderCompositor::resize_swapchain()
{
    render_cull.resize_swapchain();
}
