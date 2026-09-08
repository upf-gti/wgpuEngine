#include "renderer.h"

#if defined(OPENXR_SUPPORT)

#include "xr/openxr/openxr_context.h"

#include "xr/dawnxr/dawnxr_internal.h"

#include "graphics/backend_include.h"

#elif defined(WEBXR_SUPPORT)

#include "xr/webxr/webxr_context.h"

#endif

#include "framework/camera/camera.h"
#include "graphics/material.h"
#include "graphics/mesh.h"
#include "graphics/pipeline.h"
#include "core/managers/render/render_storage.h"
#include "graphics/shader.h"
#include "graphics/texture.h"

#include "scene/3d/gs_node.h"
#include "scene/3d/light_3d.h"
#include "scene/3d/mesh_instance_3d.h"

#include "shaders/AABB_shader.wgsl.gen.h"
#include "shaders/mesh_forward.wgsl.gen.h"
#include "shaders/mesh_shadow.wgsl.gen.h"

#include "core/managers/input/input_manager.h"
#include "core/managers/xr/xr_manager.h"

#include "framework/camera/camera_2d.h"
#include "framework/camera/editor_camera.h"
#include "framework/camera/flyover_camera.h"
#include "framework/camera/orbit_camera.h"
#include "framework/parsers/parse_scene.h"
#include "framework/ui/io.h"

#include <algorithm>

#include "shaders/gaussian_splatting/gs_render.wgsl.gen.h"
#include "shaders/mesh_texture_cube.wgsl.gen.h"
#include "shaders/quad_mirror.wgsl.gen.h"

#include "glm/gtx/quaternion.hpp"

#include "spdlog/spdlog.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

void Renderer::clean()
{
#if defined(XR_SUPPORT)

#if defined(USE_MIRROR_WINDOW)
    if (is_xr_available) {
        for (uint8_t i = 0; i < swapchain_uniforms.size(); i++) {
            swapchain_uniforms[i].destroy();
            wgpuBindGroupRelease(swapchain_bind_groups[i]);
        }
    }

#endif // XR_SUPPORT
#endif // USE_MIRROR_WINDOW

    uint8_t num_textures = is_xr_available ? 2 : 1;
    for (int i = 0; i < num_textures; ++i) {
        wgpuTextureViewRelease(eye_depth_texture_view[i]);
    }

    RenderStorage::clean_registered_pipelines();

    wgpuBindGroupRelease(render_camera_bind_group);
    wgpuBindGroupRelease(render_camera_bind_group_2d);
    wgpuBindGroupRelease(shadow_camera_bind_group);

    camera_uniform.destroy();
    camera_2d_uniform.destroy();
    shadow_camera_uniform.destroy();

    for (int i = 0; i < RENDER_LIST_COUNT; ++i) {
        render_instances_data.instances_data_uniforms[i].destroy();

        if (render_instances_data.instances_bind_groups[i]) {
            wgpuBindGroupRelease(render_instances_data.instances_bind_groups[i]);
        }

        shadow_instances_data.instances_data_uniforms[i].destroy();

        if (shadow_instances_data.instances_bind_groups[i]) {
            wgpuBindGroupRelease(shadow_instances_data.instances_bind_groups[i]);
        }
    }

    webgpu_context->destroy();

    delete render_storage;
    delete[] eye_depth_textures;
    delete[] multisample_textures;

    delete shadow_material;

    //delete selected_mesh_aabb;

    delete camera_3d;
    delete camera_2d;

    delete skybox_mesh;
}

void Renderer::update(float delta_time)
{
#if defined(XR_SUPPORT)
    if (is_xr_available) {
        xr_context->update();
    }
#endif

    if (!is_xr_available) {
        const auto& io = ImGui::GetIO();
        if (!io.WantCaptureMouse && !io.WantCaptureKeyboard && !IO::any_focus()) {
            camera_3d->update(delta_time);
        }
    } else {
        camera_3d->update(delta_time);
    }
}

void Renderer::render()
{
    WGPUTextureView screen_surface_texture_view;
    WGPUSurfaceTexture screen_surface_texture;

    if (!eye_depth_texture_view[0]) {
        spdlog::error("Can not render if depth buffer is not initialized");
        clear_renderables();

#ifdef XR_SUPPORT
        // TODO: use callback for webxr first frame instead
        if (is_xr_available) {
            glm::ivec4 viewport = xr_context->viewport;

            if (viewport.z != webgpu_context->render_width || viewport.w != webgpu_context->render_height) {
                webgpu_context->render_width = viewport.z;
                webgpu_context->render_height = viewport.w;

                resize_window(viewport.z, viewport.w);

#if defined(USE_MIRROR_WINDOW)
                init_mirror_pipeline();
#endif
            }
        }
#endif

        ImGui::Render();

        return;
    }

    if (!is_xr_available || use_mirror_screen) {
        wgpuSurfaceGetCurrentTexture(webgpu_context->surface, &screen_surface_texture);
        if (screen_surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal) {
            // Probably minimized window
            ImGui::EndFrame();
            return;
        }

        screen_surface_texture_view = webgpu_context->create_texture_view(screen_surface_texture.texture, WGPUTextureViewDimension_2D, webgpu_context->swapchain_format);
    }

    update_lights();

    //render_shadow_maps();

    camera_data.exposure = exposure;
    camera_data.ibl_intensity = ibl_intensity;
    camera_data.screen_size = { webgpu_context->screen_width, webgpu_context->screen_height };

    std::vector<std::vector<sRenderData>> render_lists(RENDER_LIST_COUNT);

    if (!is_xr_available) {
        camera_data.right_controller_position = camera_data.eye;

        prepare_cull_instancing(*camera_3d, render_lists, render_instances_data);

        camera_data.eye = camera_3d->get_eye();
        camera_data.view_projection = camera_3d->get_view_projection();
        camera_data.view = camera_3d->get_view();
        camera_data.projection = camera_3d->get_projection();

        wgpuQueueWriteBuffer(webgpu_context->device_queue, std::get<WGPUBuffer>(camera_uniform.data), 0, &camera_data, sizeof(sCameraData));

        //glm::vec3 eye = camera_3d->get_eye();
        //glm::vec3 center = camera_3d->get_center();

        render_camera(render_lists, screen_surface_texture_view, eye_depth_texture_view[EYE_LEFT], render_instances_data, render_camera_bind_group, true, "forward_render");
    }
#ifdef XR_SUPPORT
    else {

        xr_context->init_frame();

        camera_data.right_controller_position = XRManager::get_singleton()->get_controller_position(HAND_RIGHT);

        // prepare eye cameras
        Camera cameras[EYE_COUNT];
        for (uint32_t eye_idx = 0; eye_idx < EYE_COUNT; eye_idx++) {
            cameras[eye_idx].set_eye(xr_context->per_view_data[eye_idx].position);
            cameras[eye_idx].set_view(xr_context->per_view_data[eye_idx].view_matrix, false);
            cameras[eye_idx].set_projection(xr_context->per_view_data[eye_idx].projection_matrix, false);
            cameras[eye_idx].set_view_projection(xr_context->per_view_data[eye_idx].view_projection_matrix);
        }

        Camera vr_camera;
        vr_camera.set_eye((cameras[EYE_LEFT].get_eye() + cameras[EYE_RIGHT].get_eye()) * 0.5f);

        // Interpolate view
        {
            glm::mat4 left_view = xr_context->per_view_data[EYE_LEFT].view_matrix;
            glm::mat4 right_view = xr_context->per_view_data[EYE_RIGHT].view_matrix;
            glm::mat4 combined_view = left_view * 0.5f + right_view * 0.5f;
            vr_camera.set_view(combined_view, false);
        }

        // Set new FOV in projection
        {
            glm::mat4 left_proj = xr_context->per_view_data[EYE_LEFT].projection_matrix;
            float aspect = left_proj[1][1] / left_proj[0][0];
            if (!std::isnan(aspect)) {
                float eye_fov = 2.0f * atan(1.0f / left_proj[1][1]);
                float combined_eye_tan = tan(eye_fov / 2.0f) * 2.0f; // assuming same fov for both eyes
                float combined_fov = 2.0f * atan(combined_eye_tan);
                vr_camera.set_projection(glm::perspective(combined_fov, aspect, z_near, z_far));
            }
        }

        prepare_cull_instancing(vr_camera, render_lists, render_instances_data);

        for (uint32_t eye_idx = 0; eye_idx < EYE_COUNT; eye_idx++) {
            xr_context->acquire_swapchain(eye_idx);

            camera_data.eye = cameras[eye_idx].get_eye();
            camera_data.view_projection = cameras[eye_idx].get_view_projection();
            camera_data.view = cameras[eye_idx].get_view();
            camera_data.projection = cameras[eye_idx].get_projection();

            wgpuQueueWriteBuffer(webgpu_context->device_queue, std::get<WGPUBuffer>(camera_uniform.data), eye_idx * camera_buffer_stride, &camera_data, sizeof(sCameraData));

            render_camera(render_lists, xr_context->get_swapchain_view(eye_idx), eye_depth_texture_view[eye_idx], render_instances_data, render_camera_bind_group, true, "forward_render_xr", eye_idx, eye_idx);

            xr_context->release_swapchain(eye_idx);
        }

#if defined(USE_MIRROR_WINDOW)
        if (use_mirror_screen) {
            render_mirror(screen_surface_texture_view, custom_mirror_fbo_bind_group ? custom_mirror_fbo_bind_group : swapchain_bind_groups[xr_context->get_swapchain_image_index(0)]);
        }
#endif
    }
#endif

    // Render 2D
    if (!is_xr_available || use_mirror_screen) {
        camera_2d_data.eye = camera_2d->get_eye();
        camera_2d_data.view_projection = camera_2d->get_view_projection();

        camera_2d_data.exposure = exposure;
        camera_2d_data.ibl_intensity = ibl_intensity;

        wgpuQueueWriteBuffer(webgpu_context->device_queue, std::get<WGPUBuffer>(camera_2d_uniform.data), 0, &camera_2d_data, sizeof(sCameraData));

        // Prepare the color attachment
        WGPURenderPassColorAttachment render_pass_color_attachment = {};
        if (msaa_count > 1) {
            render_pass_color_attachment.view = multisample_textures_views[EYE_LEFT];
            render_pass_color_attachment.resolveTarget = screen_surface_texture_view;
        } else {
            render_pass_color_attachment.view = screen_surface_texture_view;
        }

        render_pass_color_attachment.loadOp = WGPULoadOp_Load;
        render_pass_color_attachment.storeOp = WGPUStoreOp_Store;
        render_pass_color_attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        render_pass_color_attachment.clearValue = WGPUColor{ clear_color.r, clear_color.g, clear_color.b, clear_color.a };

        WGPURenderPassDescriptor render_pass_descr = {};
        render_pass_descr.colorAttachmentCount = 1;
        render_pass_descr.colorAttachments = &render_pass_color_attachment;
        render_pass_descr.depthStencilAttachment = nullptr;

        // Create & fill the render pass (encoder)
        WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(global_command_encoder, &render_pass_descr);

        if (custom_pre_2d_pass) {
            custom_pre_2d_pass(render_pass, render_camera_bind_group_2d, custom_pass_user_data, 0);
        }

        render_2D(render_pass, render_lists, render_instances_data, render_camera_bind_group_2d);

        if (custom_post_2d_pass) {
            custom_post_2d_pass(render_pass, render_camera_bind_group_2d, custom_pass_user_data, 0);
        }

        wgpuRenderPassEncoderEnd(render_pass);
        wgpuRenderPassEncoderRelease(render_pass);

        if (!is_xr_available) {
            ImGui::Render();
        }

// TODO: remove the ifdef, IMGui brings a viewport issue that can not be fixed by setting the viewport via webgpu
#ifndef BACKEND_METAL
        // render imgui
        if (!is_xr_available) {
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

            WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(global_command_encoder, &render_pass_desc);

            webgpu_context->push_debug_group(pass, { "ImGui", WGPU_STRLEN });

            ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);

            webgpu_context->pop_debug_group(pass);

            RenderAPI::get_singleton()->render_pass_encoder_end(pass);
            wgpuRenderPassEncoderRelease(pass);
        }
#endif
    }

    submit_global_command_encoder();

    if (RenderdocCapture::is_capture_started() && debug_this_frame) {
        RenderdocCapture::end_capture_frame();
        debug_this_frame = false;
    }

    if (!is_xr_available) {
        wgpuTextureViewRelease(screen_surface_texture_view);
        wgpuTextureRelease(screen_surface_texture.texture);
    }
#ifdef XR_SUPPORT
    else {
        xr_context->end_frame();

#ifdef WEBXR_SUPPORT
        for (uint32_t eye_idx = 0; eye_idx < EYE_COUNT; eye_idx++) {
            wgpuTextureViewRelease(xr_context->get_swapchain_view(eye_idx));
        }
#endif // WEBXR_SUPPORT
    }
#endif // XR_SUPPORT

#ifndef __EMSCRIPTEN__
    if (!is_xr_available || use_mirror_screen) {
        wgpuSurfacePresent(webgpu_context->surface);
    }
#endif

    if (timestamps_requested) {
        get_timestamps();
        timestamps_requested = false;
    }

    clear_renderables();
}

void Renderer::submit_global_command_encoder()
{
    WGPUCommandBufferDescriptor cmd_buff_descriptor = {};
    cmd_buff_descriptor.nextInChain = NULL;
    cmd_buff_descriptor.label = { "Command buffer", WGPU_STRLEN };

    wgpuCommandEncoderResolveQuerySet(global_command_encoder, timestamp_query_set, 0, query_index, timestamp_query_buffer, 0);

    WGPUCommandBuffer commands = wgpuCommandEncoderFinish(global_command_encoder, &cmd_buff_descriptor);

    wgpuQueueSubmit(webgpu_context->device_queue, 1, &commands);

    wgpuCommandBufferRelease(commands);
    wgpuCommandEncoderRelease(global_command_encoder);
}

void Renderer::set_camera_params(eCameraType camera_type, const glm::vec3& camera_eye, const glm::vec3& camera_center)
{
    this->camera_type = camera_type;

    Camera* old_camera = camera_3d;
    if (camera_type == CAMERA_FLYOVER) {
        camera_3d = new FlyoverCamera();
    } else if (camera_type == CAMERA_ORBIT) {
        camera_3d = new OrbitCamera();
    } else if (camera_type == CAMERA_EDITOR) {
        camera_3d = new EditorCamera();
    }

    camera_3d->set_perspective(glm::radians(45.0f), webgpu_context->screen_width / static_cast<float>(webgpu_context->screen_height), z_near, z_far);

    if (old_camera) {
        camera_3d->look_at(old_camera->get_eye(), old_camera->get_center(), old_camera->get_up());
    } else {
        camera_3d->look_at(camera_eye, camera_center, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    camera_3d->set_mouse_sensitivity(0.003f);
    camera_3d->set_speed(0.5f);

    delete old_camera;
}

void Renderer::render_shadow_maps()
{
    camera_data.exposure = exposure;
    camera_data.ibl_intensity = ibl_intensity;
    camera_data.screen_size = { webgpu_context->screen_width, webgpu_context->screen_height };

    std::vector<std::vector<sRenderData>> render_lists(RENDER_LIST_COUNT);

    for (uint32_t light_idx = 0; light_idx < lights_with_shadow.size(); ++light_idx) {
        Light3D* light = lights_with_shadow[light_idx];

        const Camera& light_camera = light->get_light_camera();

        prepare_cull_instancing(light->get_light_camera(), render_lists, shadow_instances_data, true);

        // Update main 3d camera

        camera_data.eye = light_camera.get_eye();
        camera_data.view = light_camera.get_view();
        camera_data.projection = light_camera.get_projection();
        camera_data.view_projection = light_camera.get_view_projection();

        wgpuQueueWriteBuffer(webgpu_context->device_queue, std::get<WGPUBuffer>(shadow_camera_uniform.data), light_idx * camera_buffer_stride, &camera_data, sizeof(sCameraData));

        if (!light->get_shadow_depth_texture()) {
            light->create_shadow_data();
        }

        render_camera(render_lists, nullptr, light->get_shadow_depth_texture_view(), shadow_instances_data, shadow_camera_bind_group, false, "shadow_map", 0, light_idx);
    }

    // copy shadow maps (temp solution)
    {
        for (uint32_t light_idx = 0; light_idx < lights_with_shadow.size(); ++light_idx) {
            Light3D* light = lights_with_shadow[light_idx];
            const WGPUExtent3D& size = { SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 1 };
            webgpu_context->copy_texture_to_texture(light->get_shadow_depth_texture(), shadow_array_texture, 0, 0, size, { 0, 0, 0 }, { 0, 0, light_idx }, get_global_command_encoder());
        }
    }
}

void Renderer::add_renderable(Mesh* mesh, const glm::mat4x4& global_matrix)
{
    if ((render_entity_list.size() + 1) >= current_render_list_size) {
        current_render_list_size <<= 1;
        render_entity_list.reserve(current_render_list_size);
    }

    render_entity_list.push_back({ mesh, global_matrix });
}

void Renderer::add_splat_scene(GSNode* gs_scene)
{
    gs_scenes_list.push_back(gs_scene);
}

void Renderer::clear_renderables()
{
    render_entity_list.clear();
    gs_scenes_list.clear();

    lights_with_shadow.clear();

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        lights_uniform_data[i] = {};
    }

    num_lights = 0;
    query_index = 0;
}

void Renderer::resize_window(int width, int height)
{
    if (width == 0 || height == 0) {
        spdlog::error("Can not create swapchain with size ({}, {})", width, height);
        return;
    }

    if (!is_xr_available) {
        spdlog::info("pixel ratio: {}", webgpu_context->dpi_scale);

        webgpu_context->screen_width = width;
        webgpu_context->screen_height = height;

#ifdef __EMSCRIPTEN__
        webgpu_context->render_width = static_cast<uint32_t>(width * webgpu_context->dpi_scale);
        webgpu_context->render_height = static_cast<uint32_t>(height * webgpu_context->dpi_scale);
#else
        webgpu_context->render_width = width;
        webgpu_context->render_height = height;
#endif

        if (camera_3d) {
            camera_3d->set_perspective(glm::radians(45.0f), webgpu_context->render_width / static_cast<float>(webgpu_context->render_height), z_near, z_far);
        }

        if (camera_2d) {
            camera_2d->set_orthographic(0.0f, static_cast<float>(webgpu_context->render_width), static_cast<float>(webgpu_context->render_height), 0.0f, -1.0f, 1.0f);
        }
    }

    webgpu_context->create_swapchain(webgpu_context->render_width, webgpu_context->render_height);

    init_depth_buffers();
    init_multisample_textures();
}

void Renderer::set_irradiance_texture(Texture* texture)
{
    irradiance_texture = texture;
    skybox_material->set_diffuse_texture(irradiance_texture);

    init_lighting_bind_group();
}
