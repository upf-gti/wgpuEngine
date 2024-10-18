#include "renderer.h"

#include "graphics/webgpu_context.h"

#ifdef XR_SUPPORT

#include "xr/openxr_context.h"

#include "xr/dawnxr/dawnxr_internal.h"

#if defined(BACKEND_DX12)
#include <dawn/native/D3D12Backend.h>
#elif defined(BACKEND_VULKAN)
#include "dawn/native/VulkanBackend.h"
#endif

#endif

#include "graphics/texture.h"
#include "framework/camera/camera.h"
#include "framework/nodes/light_3d.h"
#include "graphics/mesh_instance.h"
#include "graphics/pipeline.h"
#include "graphics/shader.h"
#include "graphics/debug/renderdoc_capture.h"
#include "graphics/renderer_storage.h"

#include "shaders/mesh_forward.wgsl.gen.h"
#include "shaders/AABB_shader.wgsl.gen.h"

#include "framework/parsers/parse_scene.h"
#include "framework/nodes/mesh_instance_3d.h"
#include "framework/camera/camera_2d.h"
#include "framework/camera/flyover_camera.h"
#include "framework/camera/orbit_camera.h"
#include "framework/input.h"
#include "framework/ui/io.h"

#include <algorithm>

#include "shaders/quad_mirror.wgsl.gen.h"

#include "glm/gtx/quaternion.hpp"

#include "spdlog/spdlog.h"

Renderer* Renderer::instance = nullptr;

Renderer::Renderer()
{
    instance = this;

    renderer_storage = new RendererStorage();

#ifdef _DEBUG
    RenderdocCapture::init();
#endif

    webgpu_context = new WebGPUContext();

    Pipeline::webgpu_context = webgpu_context;
    Surface::webgpu_context = webgpu_context;
    Texture::webgpu_context = webgpu_context;

#ifdef XR_SUPPORT
    xr_context = new OpenXRContext();
    is_openxr_available = xr_context->create_instance();
#endif
}

Renderer::~Renderer()
{
    delete webgpu_context;

#ifdef XR_SUPPORT
    delete xr_context;
#endif
}

int Renderer::pre_initialize(GLFWwindow* window, bool use_mirror_screen)
{
    this->use_mirror_screen = use_mirror_screen;

    webgpu_context->window = window;
    webgpu_context->create_instance();

    Shader::set_custom_define("MAX_LIGHTS", MAX_LIGHTS);

    eye_depth_textures = new Texture[EYE_COUNT];
    multisample_textures = new Texture[EYE_COUNT];

    timestamps_buffer = new uint64_t[maximum_query_sets];

#ifndef __EMSCRIPTEN__
    renderdoc_capture = new RenderdocCapture();
#endif

    return 0;
}

int Renderer::initialize()
{
    if (!webgpu_context->adapter) {
        webgpu_context->request_adapter(xr_context, is_openxr_available);
        webgpu_context->process_events();
        return 1;
    }

    if (webgpu_context->adapter && !webgpu_context->device) {
        webgpu_context->request_device();
        webgpu_context->process_events();
        return 1;
    }

    bool create_screen_swapchain = true;
#ifdef XR_SUPPORT
    if (is_openxr_available) {
        create_screen_swapchain = use_mirror_screen;
    }
#endif

    if (webgpu_context->adapter && webgpu_context->device) {
        if (webgpu_context->initialize(create_screen_swapchain)) {
            spdlog::error("Could not initialize WebGPU context");
            return 1;
        }
    }

    spdlog::info("Renderer initialized");

    initialized = true;

    return 0;
}

int Renderer::post_initialize()
{
    webgpu_context->print_device_info();

#ifdef XR_SUPPORT

    xr_context->z_near = z_near;
    xr_context->z_far = z_far;

    if (is_openxr_available && !xr_context->init(webgpu_context)) {
        spdlog::error("Could not initialize OpenXR context");
        is_openxr_available = false;
    }

    if (is_openxr_available) {
        webgpu_context->render_width = xr_context->viewconfig_views[0].recommendedImageRectWidth;
        webgpu_context->render_height = xr_context->viewconfig_views[0].recommendedImageRectHeight;
    }
#endif

    if (!is_openxr_available) {
        webgpu_context->render_width = webgpu_context->screen_width;
        webgpu_context->render_height = webgpu_context->screen_height;
    }

    // Create the command encoder
    WGPUCommandEncoderDescriptor encoder_desc = {};
    global_command_encoder = wgpuDeviceCreateCommandEncoder(webgpu_context->device, &encoder_desc);

    if (!irradiance_texture) {
        irradiance_texture = RendererStorage::get_texture("data/textures/environments/sky.hdr");
    }

    init_depth_buffers();

    init_lighting_bind_group();
    init_camera_bind_group();

    init_multisample_textures();

    init_timestamp_queries();

#ifdef XR_SUPPORT
    if (is_openxr_available && use_mirror_screen) {
        init_mirror_pipeline();
    }
#endif

    RendererStorage::register_basic_surfaces();

    // Orthographic camera for ui rendering

    camera_2d = new Camera2D();
    camera_2d->set_orthographic(0.0f, webgpu_context->render_width, webgpu_context->render_height, 0.0f, -1.0f, 1.0f);

    //selected_mesh_aabb = parse_mesh("data/meshes/cube/aabb_cube.obj", false);

    //Material* AABB_material = new Material();
    //AABB_material->set_color(glm::vec4(0.8f, 0.3f, 0.9f, 1.0f));
    //AABB_material->set_transparency_type(ALPHA_BLEND);
    //AABB_material->set_cull_type(CULL_NONE);
    //AABB_material->set_type(MATERIAL_UNLIT);
    //AABB_material->set_shader(RendererStorage::get_shader_from_source(shaders::AABB_shader::source, shaders::AABB_shader::path, AABB_material));
    //selected_mesh_aabb->set_surface_material_override(selected_mesh_aabb->get_surface(0), AABB_material);

    return 0;
}

void Renderer::clean()
{

#if defined(XR_SUPPORT)

    xr_context->clean();

#if defined(USE_MIRROR_WINDOW)
    if (is_openxr_available) {
        for (uint8_t i = 0; i < swapchain_uniforms.size(); i++) {
            swapchain_uniforms[i].destroy();
            wgpuBindGroupRelease(swapchain_bind_groups[i]);
        }
    }

#endif // XR_SUPPORT
#endif // USE_MIRROR_WINDOW

    uint8_t num_textures = is_openxr_available ? 2 : 1;
    for (int i = 0; i < num_textures; ++i)
    {
        wgpuTextureViewRelease(eye_depth_texture_view[i]);
    }

    RendererStorage::clean_registered_pipelines();

    webgpu_context->destroy();

    delete renderer_storage;
    delete[] eye_depth_textures;
    delete[] multisample_textures;
    delete[] timestamps_buffer;

    //delete selected_mesh_aabb;

#ifndef __EMSCRIPTEN__
    delete renderdoc_capture;
#endif

    if (camera) {
        delete camera;
    }

    if (camera_2d) {
        delete camera_2d;
    }
}

void Renderer::update(float delta_time)
{
#if defined(XR_SUPPORT)
    if (is_openxr_available) {
        xr_context->update();
    }
#endif

    if (debug_this_frame) {
        RenderdocCapture::start_capture_frame();
    }

    // Create the command encoder
    WGPUCommandEncoderDescriptor encoder_desc = {};
    global_command_encoder = wgpuDeviceCreateCommandEncoder(webgpu_context->device, &encoder_desc);

    if (!is_openxr_available) {
        const auto& io = ImGui::GetIO();
        if (!io.WantCaptureMouse && !io.WantCaptureKeyboard && !IO::any_focus()) {
            camera->update(delta_time);
        }
    }
    else {
        camera->update(delta_time);
    }
}

void Renderer::render()
{
    glm::vec3 camera_position;

    if (!is_openxr_available) {
        if (!frustum_camera_paused) {
            frustum_cull.set_view_projection(camera->get_view_projection());
        }

        camera_position = camera->get_eye();
    }
#ifdef XR_SUPPORT
    else {
        // TODO: use both eyes, only left eye for now
        frustum_cull.set_view_projection(xr_context->per_view_data[0].view_projection_matrix);
        camera_position = xr_context->per_view_data[0].position;
    }
#endif

    prepare_instancing(camera_position);

    WGPUTextureView screen_surface_texture_view;
    WGPUSurfaceTexture screen_surface_texture;

    if (!is_openxr_available || use_mirror_screen) {

        wgpuSurfaceGetCurrentTexture(webgpu_context->surface, &screen_surface_texture);
        if (screen_surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
            spdlog::error("Error getting swapchain texture");
            return;
        }

        screen_surface_texture_view = webgpu_context->create_texture_view(screen_surface_texture.texture, WGPUTextureViewDimension_2D, webgpu_context->swapchain_format);
    }

    if (!is_openxr_available) {
        render_screen(screen_surface_texture_view);
    }

#if defined(XR_SUPPORT)
    if (is_openxr_available) {
        render_xr();

        if (use_mirror_screen) {
            render_mirror(screen_surface_texture_view);
        }
    }
#endif

    submit_global_command_encoder();

    if (RenderdocCapture::is_capture_started() && debug_this_frame) {
        RenderdocCapture::end_capture_frame();
        debug_this_frame = false;
    }

    if (!is_openxr_available) {
        wgpuTextureViewRelease(screen_surface_texture_view);
        wgpuTextureRelease(screen_surface_texture.texture);
    }

#ifdef XR_SUPPORT
    else {
        xr_context->end_frame();
    }
#endif

#ifndef __EMSCRIPTEN__
    if (!is_openxr_available || use_mirror_screen) {
        wgpuSurfacePresent(webgpu_context->surface);
    }
#endif

    clear_renderables();
}

void Renderer::process_events()
{
    webgpu_context->process_events();
}

void Renderer::submit_global_command_encoder()
{
    WGPUCommandBufferDescriptor cmd_buff_descriptor = {};
    cmd_buff_descriptor.nextInChain = NULL;
    cmd_buff_descriptor.label = { "Command buffer", WGPU_STRLEN };

    resolve_query_set(global_command_encoder, 0);

    WGPUCommandBuffer commands = wgpuCommandEncoderFinish(global_command_encoder, &cmd_buff_descriptor);

    wgpuQueueSubmit(webgpu_context->device_queue, 1, &commands);

    wgpuCommandBufferRelease(commands);
    wgpuCommandEncoderRelease(global_command_encoder);

}

void Renderer::set_camera_type(eCameraType camera_type)
{
    this->camera_type = camera_type;

    Camera* old_camera = camera;
    if (camera_type == CAMERA_FLYOVER) {
        camera = new FlyoverCamera();
    }
    else if (camera_type == CAMERA_ORBIT) {
        camera = new OrbitCamera();
    }

    camera->set_perspective(glm::radians(45.0f), webgpu_context->render_width / static_cast<float>(webgpu_context->render_height), z_near, z_far);

    if (old_camera) {
        camera->look_at(old_camera->get_eye(), old_camera->get_center(), old_camera->get_up());
    }
    else {
        camera->look_at(glm::vec3(0.0f, 0.2f, 0.8f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    camera->set_mouse_sensitivity(0.003f);
    camera->set_speed(0.5f);

    delete old_camera;
}

eCameraType Renderer::get_camera_type()
{
    return camera_type;
}

void Renderer::set_custom_pass_user_data(void* user_data)
{
    this->custom_pass_user_data = user_data;
}

void Renderer::render_screen(WGPUTextureView screen_surface_texture_view)
{
    // Update main 3d camera

    camera_data.eye = camera->get_eye();
    camera_data.view_projection = camera->get_view_projection();
    camera_data.view = camera->get_view();
    camera_data.projection = camera->get_projection();
    camera_data.screen_size = { webgpu_context->screen_width, webgpu_context->screen_height };

    // Use camera position as controller position
    camera_data.right_controller_position = camera_data.eye;

    camera_data.exposure = exposure;
    camera_data.ibl_intensity = ibl_intensity;

    wgpuQueueWriteBuffer(webgpu_context->device_queue, std::get<WGPUBuffer>(camera_uniform.data), 0, &camera_data, sizeof(sCameraData));

    // Update 2d camera for UI

    camera_2d_data.eye = camera_2d->get_eye();
    camera_2d_data.view_projection = camera_2d->get_view_projection();

    camera_2d_data.exposure = exposure;
    camera_2d_data.ibl_intensity = ibl_intensity;

    wgpuQueueWriteBuffer(webgpu_context->device_queue, std::get<WGPUBuffer>(camera_2d_uniform.data), 0, &camera_2d_data, sizeof(sCameraData));

    ImGui::Render();

    {
        // Prepare the color attachment
        WGPURenderPassColorAttachment render_pass_color_attachment = {};
        if (msaa_count > 1) {
            render_pass_color_attachment.view = multisample_textures_views[0];
            render_pass_color_attachment.resolveTarget = screen_surface_texture_view;
        }
        else {
            render_pass_color_attachment.view = screen_surface_texture_view;
        }

        render_pass_color_attachment.loadOp = WGPULoadOp_Clear;
        render_pass_color_attachment.storeOp = WGPUStoreOp_Store;
        render_pass_color_attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        render_pass_color_attachment.clearValue = WGPUColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);

        // Prepate the depth attachment
        WGPURenderPassDepthStencilAttachment render_pass_depth_attachment = {};
        render_pass_depth_attachment.view = eye_depth_texture_view[EYE_LEFT];
        render_pass_depth_attachment.depthClearValue = 0.0f;
        render_pass_depth_attachment.depthLoadOp = WGPULoadOp_Clear;
        render_pass_depth_attachment.depthStoreOp = WGPUStoreOp_Store;
        render_pass_depth_attachment.depthReadOnly = false;
        render_pass_depth_attachment.stencilClearValue = 0; // Stencil config necesary, even if unused
        render_pass_depth_attachment.stencilLoadOp = WGPULoadOp_Undefined;
        render_pass_depth_attachment.stencilStoreOp = WGPUStoreOp_Undefined;
        render_pass_depth_attachment.stencilReadOnly = true;

        WGPURenderPassDescriptor render_pass_descr = {};
        render_pass_descr.colorAttachmentCount = 1;
        render_pass_descr.colorAttachments = &render_pass_color_attachment;
        render_pass_descr.depthStencilAttachment = &render_pass_depth_attachment;

        std::vector<WGPURenderPassTimestampWrites> timestampWrites(1);
        timestampWrites[0].beginningOfPassWriteIndex = timestamp(global_command_encoder, "pre_render");
        timestampWrites[0].querySet = timestamp_query_set;
        timestampWrites[0].endOfPassWriteIndex = timestamp(global_command_encoder, "render");

        render_pass_descr.timestampWrites = timestampWrites.data();

        // Create & fill the render pass (encoder)
        WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(global_command_encoder, &render_pass_descr);

        if (custom_pre_opaque_pass) {
            custom_pre_opaque_pass(custom_pass_user_data, render_pass, 0);
        }

        render_opaque(render_pass, render_camera_bind_group);

        if (custom_post_opaque_pass) {
            custom_post_opaque_pass(custom_pass_user_data, render_pass, 0);
        }

        if (custom_pre_transparent_pass) {
            custom_pre_transparent_pass(custom_pass_user_data, render_pass, 0);
        }

        render_transparent(render_pass, render_camera_bind_group);

        if (custom_post_transparent_pass) {
            custom_post_transparent_pass(custom_pass_user_data, render_pass, 0);
        }

        if (custom_pre_2d_pass) {
            custom_pre_2d_pass(custom_pass_user_data, render_pass, 0);
        }

        render_2D(render_pass, render_camera_bind_group_2d);

        if (custom_post_2d_pass) {
            custom_post_2d_pass(custom_pass_user_data, render_pass, 0);
        }

        wgpuRenderPassEncoderEnd(render_pass);

        wgpuRenderPassEncoderRelease(render_pass);

        //timestamp(global_command_encoder, "render");

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

            WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(global_command_encoder, &render_pass_desc);

            ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);

            wgpuRenderPassEncoderEnd(pass);
            wgpuRenderPassEncoderRelease(pass);
        }
    }
}

#if defined(XR_SUPPORT)

void Renderer::render_xr()
{
    xr_context->init_frame();

    for (uint32_t i = 0; i < xr_context->view_count; ++i) {

        xr_context->acquire_swapchain(i);

        const sSwapchainData& swapchainData = xr_context->swapchains[i];

        camera_data.eye = xr_context->per_view_data[i].position;
        camera_data.view_projection = xr_context->per_view_data[i].view_projection_matrix;

        camera_data.view = xr_context->per_view_data[i].view_matrix;
        camera_data.projection = xr_context->per_view_data[i].projection_matrix;
        camera_data.screen_size = { webgpu_context->render_width, webgpu_context->render_height };

        camera_data.right_controller_position = Input::get_controller_position(HAND_RIGHT);

        camera_data.exposure = exposure;
        camera_data.ibl_intensity = ibl_intensity;

        wgpuQueueWriteBuffer(webgpu_context->device_queue, std::get<WGPUBuffer>(camera_uniform.data), i * xr_camera_buffer_stride, &camera_data, sizeof(sCameraData));

        {
            // Prepare the color attachment
            WGPURenderPassColorAttachment render_pass_color_attachment = {};
            render_pass_color_attachment.view = swapchainData.images[swapchainData.image_index].textureView;

            if (msaa_count > 1) {
                render_pass_color_attachment.view = multisample_textures_views[i];
                render_pass_color_attachment.resolveTarget = swapchainData.images[swapchainData.image_index].textureView;
            }
            else {
                render_pass_color_attachment.view = swapchainData.images[swapchainData.image_index].textureView;
            }

            render_pass_color_attachment.loadOp = WGPULoadOp_Clear;
            render_pass_color_attachment.storeOp = WGPUStoreOp_Store;
            render_pass_color_attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
            render_pass_color_attachment.clearValue = WGPUColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);

            // Prepate the depth attachment
            WGPURenderPassDepthStencilAttachment render_pass_depth_attachment = {};
            render_pass_depth_attachment.view = eye_depth_texture_view[i];
            render_pass_depth_attachment.depthClearValue = 0.0f;
            render_pass_depth_attachment.depthLoadOp = WGPULoadOp_Clear;
            render_pass_depth_attachment.depthStoreOp = WGPUStoreOp_Store;
            render_pass_depth_attachment.depthReadOnly = false;
            render_pass_depth_attachment.stencilClearValue = 0; // Stencil config necesary, even if unused
            render_pass_depth_attachment.stencilLoadOp = WGPULoadOp_Undefined;
            render_pass_depth_attachment.stencilStoreOp = WGPUStoreOp_Undefined;
            render_pass_depth_attachment.stencilReadOnly = true;

            WGPURenderPassDescriptor render_pass_descr = {};
            render_pass_descr.colorAttachmentCount = 1;
            render_pass_descr.colorAttachments = &render_pass_color_attachment;
            render_pass_descr.depthStencilAttachment = &render_pass_depth_attachment;

            // Create & fill the render pass (encoder)
            WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(global_command_encoder, &render_pass_descr);

            if (custom_pre_opaque_pass) {
                custom_pre_opaque_pass(custom_pass_user_data, render_pass, i * xr_camera_buffer_stride);
            }

            render_opaque(render_pass, render_camera_bind_group, i * xr_camera_buffer_stride);

            if (custom_post_opaque_pass) {
                custom_post_opaque_pass(custom_pass_user_data, render_pass, i * xr_camera_buffer_stride);
            }

            if (custom_pre_transparent_pass) {
                custom_pre_transparent_pass(custom_pass_user_data, render_pass, i * xr_camera_buffer_stride);
            }

            render_transparent(render_pass, render_camera_bind_group, i * xr_camera_buffer_stride);

            if (custom_post_transparent_pass) {
                custom_post_transparent_pass(custom_pass_user_data, render_pass, i * xr_camera_buffer_stride);
            }

            if (custom_pre_2d_pass) {
                custom_pre_2d_pass(custom_pass_user_data, render_pass, i * xr_camera_buffer_stride);
            }

            render_2D(render_pass, render_camera_bind_group_2d);

            if (custom_post_2d_pass) {
                custom_post_2d_pass(custom_pass_user_data, render_pass, i * xr_camera_buffer_stride);
            }

            wgpuRenderPassEncoderEnd(render_pass);
            wgpuRenderPassEncoderRelease(render_pass);
        }

        xr_context->release_swapchain(i);
    }
}
#endif

void Renderer::init_lighting_bind_group()
{
    // delete if already created
    if (std::holds_alternative<WGPUTextureView>(irradiance_texture_uniform.data)) {
        wgpuTextureViewRelease(std::get<WGPUTextureView>(irradiance_texture_uniform.data));
        wgpuSamplerRelease(std::get<WGPUSampler>(ibl_sampler_uniform.data));
        wgpuBindGroupRelease(lighting_bind_group);
    }
    else {
        // only created once
        brdf_lut_uniform.data = webgpu_context->brdf_lut_texture->get_view();
        brdf_lut_uniform.binding = 1;
    }

    if (irradiance_texture) {

        irradiance_texture_uniform.data = irradiance_texture->get_view(WGPUTextureViewDimension_Cube, 0, 6, 0, 6);
        irradiance_texture_uniform.binding = 0;

        ibl_sampler_uniform.data = webgpu_context->create_sampler(
            WGPUAddressMode_ClampToEdge,
            WGPUAddressMode_ClampToEdge,
            WGPUAddressMode_ClampToEdge,
            WGPUFilterMode_Linear,
            WGPUFilterMode_Linear,
            WGPUMipmapFilterMode_Linear,
            static_cast<float>(irradiance_texture->get_mipmap_count())
        );

        ibl_sampler_uniform.binding = 2;
    }

    if (std::holds_alternative<WGPUBuffer>(lights_buffer.data)) {
        wgpuBufferDestroy(std::get<WGPUBuffer>(lights_buffer.data));
        lights_buffer.data = {};
    }

    lights_buffer.data = webgpu_context->create_buffer(sizeof(sLightUniformData) * MAX_LIGHTS, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &lights_uniform_data[0], "lights_buffer");
    lights_buffer.binding = 3;
    lights_buffer.buffer_size = sizeof(sLightUniformData) * MAX_LIGHTS;

    num_lights_buffer.data = webgpu_context->create_buffer(sizeof(int), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &num_lights, "num_lights_buffer");
    num_lights_buffer.binding = 4;
    num_lights_buffer.buffer_size = sizeof(int);

    std::vector<Uniform*> uniforms = { &irradiance_texture_uniform, &brdf_lut_uniform, &ibl_sampler_uniform, &lights_buffer, &num_lights_buffer };
    lighting_bind_group = webgpu_context->create_bind_group(uniforms, RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path), 3);
}

void Renderer::init_depth_buffers()
{
    uint8_t num_textures = is_openxr_available ? 2 : 1;
    for (int i = 0; i < num_textures; ++i)
    {
        eye_depth_textures[i].create(
            WGPUTextureDimension_2D,
            WGPUTextureFormat_Depth32Float,
            { webgpu_context->render_width, webgpu_context->render_height, 1 },
            WGPUTextureUsage_RenderAttachment,
            1, msaa_count, nullptr);

        if (eye_depth_texture_view[i]) {
            wgpuTextureViewRelease(eye_depth_texture_view[i]);
        }

        // Generate Texture views of depth buffers
        eye_depth_texture_view[i] = eye_depth_textures[i].get_view();
    }
}

void Renderer::init_multisample_textures()
{
    WGPUTextureFormat swapchain_format = is_openxr_available ? webgpu_context->xr_swapchain_format : webgpu_context->swapchain_format;

    uint8_t num_textures = is_openxr_available ? 2 : 1;
    for (int i = 0; i < num_textures; ++i) {
        multisample_textures[i].create(
            WGPUTextureDimension_2D,
            swapchain_format,
            { webgpu_context->render_width, webgpu_context->render_height, 1 },
            WGPUTextureUsage_RenderAttachment,
            1, msaa_count, nullptr);

        if (multisample_textures_views[i]) {
            wgpuTextureViewRelease(multisample_textures_views[i]);
        }

        multisample_textures_views[i] = multisample_textures[i].get_view();
    }
}

void Renderer::init_timestamp_queries()
{
    timestamp_query_set = webgpu_context->create_query_set(maximum_query_sets);
    timestamp_query_buffer = webgpu_context->create_buffer(sizeof(uint64_t) * maximum_query_sets, WGPUBufferUsage_QueryResolve | WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, nullptr);
}

void Renderer::resolve_query_set(WGPUCommandEncoder encoder, uint8_t first_query)
{
    wgpuCommandEncoderResolveQuerySet(encoder, timestamp_query_set, first_query, query_index, timestamp_query_buffer, 0);
}

std::vector<float> Renderer::get_timestamps()
{
    webgpu_context->read_buffer(timestamp_query_buffer, sizeof(uint64_t) * maximum_query_sets, timestamps_buffer);

    std::vector<float> time_diffs;
    for (int i = 0; i < query_index; i += 2) {
        uint64_t diff = timestamps_buffer[i + 1] - timestamps_buffer[i];
        float milliseconds = (float)diff * 1e-6;
        time_diffs.push_back(milliseconds);
    }

    return time_diffs;
}

void Renderer::set_msaa_count(uint8_t msaa_count)
{
    bool recreate = msaa_count != this->msaa_count && multisample_textures[0].get_texture() != nullptr;

    this->msaa_count = msaa_count;

    if (recreate) {
        init_depth_buffers();
        init_multisample_textures();
    }

    RendererStorage::reload_all_render_pipelines();
}

void Renderer::set_frustum_camera_paused(bool value)
{
    frustum_camera_paused = value;
}

bool Renderer::get_frustum_camera_paused()
{
    return frustum_camera_paused;
}

uint8_t Renderer::get_msaa_count()
{
    return msaa_count;
}

void Renderer::prepare_instancing(const glm::vec3& camera_position)
{
    update_lights();

    // Get all surfaces from entity meshes
    for (auto render_list_data : render_entity_list)
    {
        MeshInstance* mesh_instance = render_list_data.mesh_instance;
        glm::mat4x4 global_matrix = render_list_data.global_matrix;

        const std::vector<Surface*>& surfaces = mesh_instance->get_surfaces();

        for (Surface* surface : surfaces) {

            Material* material_override = mesh_instance->get_surface_material_override(surface);

            Material* material = material_override ? material_override : surface->get_material();

            if (!material || !material->get_shader()) {
                continue;
            }

            if (!material->get_is_2D() && mesh_instance->get_frustum_culling_enabled()) {

                const AABB& surface_aabb = surface->get_aabb();

                AABB aabb_transformed = surface_aabb.transform(global_matrix);

                if (!is_inside_frustum(aabb_transformed.center - aabb_transformed.half_size, aabb_transformed.center + aabb_transformed.half_size)) {
                    continue;
                }
            }

            RendererStorage::instance->register_material_bind_group(webgpu_context, mesh_instance, material);

            RendererStorage::register_render_pipeline(material);

            eRenderListType list = RENDER_LIST_OPAQUE;

            if (material->get_is_2D()) {
                list = material->get_transparency_type() == ALPHA_BLEND ? RENDER_LIST_2D_TRANSPARENT : RENDER_LIST_2D;
            }
            else if (material->get_transparency_type() == ALPHA_BLEND) {
                list = RENDER_LIST_TRANSPARENT;
            }

            render_list[list].push_back({ surface, 1, global_matrix, mesh_instance });
        }
    }

    for (int i = 0; i < RENDER_LIST_SIZE; ++i) {

        instance_data[i].clear();
        instance_data[i].resize(render_list[i].size());


        /*if (i != RENDER_LIST_TRANSPARENT)*/ {
            // Sort opaques render_list
            std::sort(render_list[i].begin(), render_list[i].end(), [](auto& lhs, auto& rhs) {

                Material* lhs_ov_mat = lhs.mesh_instance_ref->get_surface_material_override(lhs.surface);
                Material* rhs_ov_mat = rhs.mesh_instance_ref->get_surface_material_override(rhs.surface);

                Material* lhs_mat = lhs_ov_mat ? lhs_ov_mat : lhs.surface->get_material();
                Material* rhs_mat = rhs_ov_mat ? rhs_ov_mat : rhs.surface->get_material();

                bool equal_priority = lhs_mat->get_priority() == rhs_mat->get_priority();
                bool equal_shader = lhs_mat->get_shader() == rhs_mat->get_shader();
                bool equal_diffuse = lhs_mat->get_diffuse_texture() == rhs_mat->get_diffuse_texture();
                bool equal_normal = lhs_mat->get_normal_texture() == rhs_mat->get_normal_texture();
                bool equal_metallic_rougness = lhs_mat->get_metallic_roughness_texture() == rhs_mat->get_metallic_roughness_texture();

                if (lhs_mat->get_priority() > rhs_mat->get_priority()) return true;
                if (equal_priority && lhs_mat->get_shader() > rhs_mat->get_shader()) return true;
                if (equal_priority && equal_shader && lhs_mat->get_diffuse_texture() > rhs_mat->get_diffuse_texture()) return true;
                if (equal_priority && equal_shader && equal_diffuse && lhs_mat->get_normal_texture() > rhs_mat->get_normal_texture()) return true;
                if (equal_priority && equal_shader && equal_diffuse && equal_normal && lhs_mat->get_metallic_roughness_texture() > rhs_mat->get_metallic_roughness_texture()) return true;
                if (equal_priority && equal_shader && equal_diffuse && equal_normal && equal_metallic_rougness && lhs_mat->get_emissive_texture() > rhs_mat->get_emissive_texture()) return true;

                return false;
            });
        }
        //else {
        //    // Sort transparent render_list by distance to camera
        //    std::sort(render_list[i].begin(), render_list[i].end(), [&](auto& lhs, auto& rhs) {
        //        glm::vec3 lhs_pos = glm::vec3(lhs.global_matrix[3]);
        //        glm::vec3 rhs_pos = glm::vec3(rhs.global_matrix[3]);

        //        float lhs_dist = glm::distance2(lhs_pos, camera_position);
        //        float rhs_dist = glm::distance2(rhs_pos, camera_position);

        //        if (lhs_dist > rhs_dist) return true;

        //        return false;
        //    });
        //}

        // Check instances
        {
            const Surface* prev_surface = nullptr;
            Shader* prev_shader = nullptr;
            glm::vec4 prev_color = {};
            Texture* prev_diffuse = nullptr;
            Texture* prev_normal = nullptr;
            Texture* prev_metallic_roughness = nullptr;
            Texture* prev_emissive = nullptr;

            uint32_t repeats = 0;
            for (uint32_t j = 0; j < render_list[i].size(); ++j) {

                const sRenderData& render_data = render_list[i][j];

                Material* material_override = render_data.mesh_instance_ref->get_surface_material_override(render_data.surface);

                Material* material = material_override ? material_override : render_data.surface->get_material();

                // Repeated MeshInstance3D, must be instanced
                if (prev_surface == render_data.surface && prev_shader == material->get_shader() &&
                    prev_color == material->get_color() &&
                    prev_diffuse == material->get_diffuse_texture() &&
                    prev_normal == material->get_normal_texture() &&
                    prev_metallic_roughness == material->get_metallic_roughness_texture() &&
                    prev_emissive == material->get_emissive_texture() &&
                    !(material->get_is_2D())) {
                    repeats++;
                }
                else {
                    if (repeats > 0) {
                        for (uint32_t k = 1; k <= repeats; k++) {
                            render_list[i][j - k].repeat = k;
                        }
                    }
                    repeats = 1;
                }

                prev_surface = render_data.surface;
                prev_shader = material->get_shader_ref();
                prev_color = material->get_color();
                prev_diffuse = material->get_diffuse_texture();
                prev_normal = material->get_normal_texture();
                prev_metallic_roughness = material->get_metallic_roughness_texture();
                prev_emissive = material->get_emissive_texture();

                // Fill instance_data
                instance_data[i][j] = { render_data.global_matrix };
            }

            if (repeats > 0) {
                for (uint32_t k = 1; k <= repeats; k++) {
                    render_list[i][render_list[i].size() - k].repeat = k;
                }
            }
        }

        // Fill instance buffers
        uint32_t instances = static_cast<uint32_t>(instance_data[i].size());

        if (instances > instance_data_uniform[i].buffer_size / sizeof(sUniformData)) {

            //std::vector<sUniformData> default_data = { instances, { glm::mat4x4(1.0f), glm::vec4(1.0f) } };

            instance_data_uniform[i].data = webgpu_context->create_buffer(sizeof(sUniformData) * instances, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, instance_data[i].data(), "instance_mesh_buffer");
            instance_data_uniform[i].binding = 0;
            instance_data_uniform[i].buffer_size = sizeof(sUniformData) * instances;

            // Recreate bind groups
            std::vector<Uniform*> uniforms = { &instance_data_uniform[i] };
            Shader* prev_shader = nullptr;
            for (uint32_t j = 0; j < render_list[i].size(); ) {

                const sRenderData& render_data = render_list[i][j];

                const Material* material_override = render_data.mesh_instance_ref->get_surface_material_override(render_data.surface);

                const Shader* shader = material_override ? material_override->get_shader() : render_data.surface->get_material()->get_shader_ref();

                if (bind_groups[i]) {
                    wgpuBindGroupRelease(bind_groups[i]);
                }

                bind_groups[i] = webgpu_context->create_bind_group(uniforms, shader, 0);

                j += render_data.repeat;
            }

        }
        else
            if (instances > 0) {
                webgpu_context->update_buffer(std::get<WGPUBuffer>(instance_data_uniform[i].data), 0, instance_data[i].data(), sizeof(sUniformData) * instances);
            }
    }
}

void Renderer::render_render_list(int list_index, WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_camera_bind_group, uint32_t camera_buffer_stride)
{
    const Pipeline* prev_pipeline = nullptr;

    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, bind_groups[list_index], 0, nullptr);
    wgpuRenderPassEncoderSetBindGroup(render_pass, 1, render_camera_bind_group, 1, &camera_buffer_stride);

    const Material* prev_material = nullptr;

    for (int i = 0; i < render_list[list_index].size(); ) {

        const sRenderData& render_data = render_list[list_index][i];

        const Material* material_override = render_data.mesh_instance_ref->get_surface_material_override(render_data.surface);

        const Material* material = material_override ? material_override : render_data.surface->get_material();

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

        if (material != prev_material) {
            wgpuRenderPassEncoderSetBindGroup(render_pass, 2, renderer_storage->get_material_bind_group(material), 0, nullptr);
            prev_material = material;

            if (material->get_type() == MATERIAL_PBR) {
                wgpuRenderPassEncoderSetBindGroup(render_pass, 3, lighting_bind_group, 0, nullptr);
            }
        }

//#ifndef NDEBUG
//        wgpuRenderPassEncoderPushDebugGroup(render_pass, render_data.surface->get_name().c_str());
//#endif

        if (material->get_type() == MATERIAL_UI) {
            WGPUBindGroup ui_bind_group = renderer_storage->get_ui_widget_bind_group(render_data.mesh_instance_ref);
            if (ui_bind_group) {
                wgpuRenderPassEncoderSetBindGroup(render_pass, 3, ui_bind_group, 0, nullptr);
            }
        }

        // Set vertex buffer while encoding the render pass
        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, render_data.surface->get_vertex_buffer(), 0, render_data.surface->get_vertices_byte_size());

        WGPUBuffer index_buffer = render_data.surface->get_index_buffer();

        if (index_buffer) {
            wgpuRenderPassEncoderSetIndexBuffer(render_pass, index_buffer, WGPUIndexFormat_Uint32, 0, render_data.surface->get_indices_byte_size());

            wgpuRenderPassEncoderDrawIndexed(render_pass, render_data.surface->get_index_count(), render_data.repeat, 0, 0, i);
        }
        else {
            wgpuRenderPassEncoderDraw(render_pass, render_data.surface->get_vertex_count(), render_data.repeat, 0, i);
        }


//#ifndef NDEBUG
//        wgpuRenderPassEncoderPopDebugGroup(render_pass);
//#endif

        prev_pipeline = pipeline;

        i += render_data.repeat;
    }
}

void Renderer::render_opaque(WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_camera_bind_group, uint32_t camera_buffer_stride)
{
#ifndef NDEBUG
    wgpuRenderPassEncoderPushDebugGroup(render_pass, "Opaque");
#endif

    render_render_list(RENDER_LIST_OPAQUE, render_pass, render_camera_bind_group, camera_buffer_stride);

#ifndef NDEBUG
    wgpuRenderPassEncoderPopDebugGroup(render_pass);
#endif
}

void Renderer::render_transparent(WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_camera_bind_group, uint32_t camera_buffer_stride)
{
#ifndef NDEBUG
    wgpuRenderPassEncoderPushDebugGroup(render_pass, "Transparent");
#endif

    render_render_list(RENDER_LIST_TRANSPARENT, render_pass, render_camera_bind_group, camera_buffer_stride);

#ifndef NDEBUG
    wgpuRenderPassEncoderPopDebugGroup(render_pass);
#endif
}

void Renderer::render_2D(WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_camera_bind_group)
{
#ifndef NDEBUG
    wgpuRenderPassEncoderPushDebugGroup(render_pass, "2D");
#endif

    render_render_list(RENDER_LIST_2D, render_pass, render_camera_bind_group);

    render_render_list(RENDER_LIST_2D_TRANSPARENT, render_pass, render_camera_bind_group);

#ifndef NDEBUG
    wgpuRenderPassEncoderPopDebugGroup(render_pass);
#endif
}

bool Renderer::is_inside_frustum(const glm::vec3& minp, const glm::vec3& maxp) const
{
    return frustum_cull.is_box_visible(minp, maxp);
}

uint8_t Renderer::timestamp(WGPUCommandEncoder encoder, const char* label)
{
    //wgpuCommandEncoderWriteTimestamp(encoder, timestamp_query_set, query_index);
    queries_label_map[query_index] = std::string(label);

    assert(query_index + 1 < maximum_query_sets);

    return query_index++;
}

void Renderer::add_renderable(MeshInstance* mesh_instance, const glm::mat4x4& global_matrix)
{
    render_entity_list.push_back({ mesh_instance, global_matrix });
}

void Renderer::clear_renderables()
{
    render_entity_list.clear();

    for (int i = 0; i < RENDER_LIST_SIZE; ++i) {
        render_list[i].clear();
    }

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        lights_uniform_data[i] = {};
    }

    num_lights = 0;

    query_index = 0;
}

void Renderer::update_lights()
{
    // update uniform data

    uint64_t buffer_size = sizeof(sLightUniformData) * num_lights;

    webgpu_context->update_buffer(std::get<WGPUBuffer>(lights_buffer.data), 0, &lights_uniform_data[0], buffer_size);

    webgpu_context->update_buffer(std::get<WGPUBuffer>(num_lights_buffer.data), 0, &num_lights, sizeof(int));
}

void Renderer::add_light(Light3D* new_light)
{
    lights_uniform_data[num_lights] = new_light->get_uniform_data();
    num_lights++;
}

void Renderer::resize_window(int width, int height)
{
    webgpu_context->create_swapchain(width, height);

    if (!is_openxr_available) {
        webgpu_context->render_width = webgpu_context->screen_width;
        webgpu_context->render_height = webgpu_context->screen_height;

        if (camera) {
            camera->set_perspective(glm::radians(45.0f), webgpu_context->render_width / static_cast<float>(webgpu_context->render_height), z_near, z_far);
        }

        if (camera_2d) {
            camera_2d->set_orthographic(0.0f, webgpu_context->render_width, webgpu_context->render_height, 0.0f, -1.0f, 1.0f);
        }
    }

    init_depth_buffers();
    init_multisample_textures();
}

void Renderer::set_irradiance_texture(Texture* texture)
{
    irradiance_texture = texture;

    init_lighting_bind_group();
}

#ifdef XR_SUPPORT
OpenXRContext* Renderer::get_openxr_context()
{
    return (is_openxr_available ? xr_context : nullptr);
}
#endif

WebGPUContext* Renderer::get_webgpu_context()
{
    return webgpu_context;
}

GLFWwindow* Renderer::get_glfw_window()
{
    return webgpu_context->window;
};

#if defined(XR_SUPPORT) && defined(USE_MIRROR_WINDOW)

void Renderer::init_mirror_pipeline()
{
    mirror_shader = RendererStorage::get_shader_from_source(shaders::quad_mirror::source, shaders::quad_mirror::path);

    quad_surface.create_quad(2.0f, 2.0f);

    WGPUTextureFormat swapchain_format = webgpu_context->swapchain_format;

    WGPUColorTargetState color_target = {};
    color_target.format = swapchain_format;
    color_target.blend = nullptr;
    color_target.writeMask = WGPUColorWriteMask_All;

    // Generate uniforms from the swapchain
    for (uint8_t i = 0; i < xr_context->swapchains[0].images.size(); i++) {
        Uniform swapchain_uni;

        swapchain_uni.data = xr_context->swapchains[0].images[i].textureView;
        swapchain_uni.binding = 0;

        swapchain_uniforms.push_back(swapchain_uni);
    }

    linear_sampler_uniform.data = webgpu_context->create_sampler(WGPUAddressMode_ClampToEdge, WGPUAddressMode_ClampToEdge, WGPUAddressMode_ClampToEdge, WGPUFilterMode_Linear, WGPUFilterMode_Linear);
    linear_sampler_uniform.binding = 1;

    // Generate bindgroups from the swapchain
    for (uint8_t i = 0; i < swapchain_uniforms.size(); i++) {
        Uniform swapchain_uni;

        std::vector<Uniform*> uniforms = { &swapchain_uniforms[i], &linear_sampler_uniform };

        swapchain_bind_groups.push_back(webgpu_context->create_bind_group(uniforms, mirror_shader, 0));
    }

    mirror_pipeline.create_render(mirror_shader, color_target, { .use_depth = false, .allow_msaa = false });
}

void Renderer::render_mirror(WGPUTextureView screen_surface_texture_view)
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
            WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(global_command_encoder, &render_pass_descr);

            // Bind Pipeline
            if (!mirror_pipeline.set(render_pass)) {
                wgpuRenderPassEncoderEnd(render_pass);
                wgpuRenderPassEncoderRelease(render_pass);
                return;
            }

            // Set binding group
            wgpuRenderPassEncoderSetBindGroup(render_pass, 0, swapchain_bind_groups[xr_context->swapchains[0].image_index], 0, nullptr);

            // Set vertex buffer while encoding the render pass
            wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, quad_surface.get_vertex_buffer(), 0, quad_surface.get_vertices_byte_size());

            // Submit drawcall
            wgpuRenderPassEncoderDraw(render_pass, 6, 1, 0, 0);

            wgpuRenderPassEncoderEnd(render_pass);
            wgpuRenderPassEncoderRelease(render_pass);
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

        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(global_command_encoder, &render_pass_desc);

        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);

        wgpuRenderPassEncoderEnd(pass);
        wgpuRenderPassEncoderRelease(pass);
    }
}

#endif

void Renderer::init_camera_bind_group()
{
    xr_camera_buffer_stride = std::max(static_cast<uint32_t>(sizeof(sCameraData)), webgpu_context->required_limits.limits.minUniformBufferOffsetAlignment);

    camera_uniform.data = webgpu_context->create_buffer(xr_camera_buffer_stride * EYE_COUNT, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, nullptr, "camera_buffer");
    camera_uniform.binding = 0;
    camera_uniform.buffer_size = sizeof(sCameraData);

    camera_2d_uniform.data = webgpu_context->create_buffer(sizeof(sCameraData), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, nullptr, "camera_2d_buffer");
    camera_2d_uniform.binding = 0;
    camera_2d_uniform.buffer_size = sizeof(sCameraData);

    std::vector<Uniform*> uniforms = { &camera_uniform };
    render_camera_bind_group = webgpu_context->create_bind_group(uniforms, RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path), 1);

    WGPUBindGroupLayoutEntry entry = {};
    entry.binding = 0;
    entry.buffer.type = WGPUBufferBindingType_Uniform;
    entry.buffer.hasDynamicOffset = true;
    entry.visibility = WGPUShaderStage_Compute;

    WGPUBindGroupLayout compute_camera_bind_group_layout = webgpu_context->create_bind_group_layout({ entry });
    compute_camera_bind_group = webgpu_context->create_bind_group(uniforms, compute_camera_bind_group_layout);

    uniforms = { &camera_2d_uniform };
    render_camera_bind_group_2d = webgpu_context->create_bind_group(uniforms, RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path), 1);
}

glm::vec3 Renderer::get_camera_eye()
{
#if defined(XR_SUPPORT)
    if (is_openxr_available) {
        return xr_context->per_view_data[0].position; // return left eye
    }
#endif

    return camera->get_eye();
}

glm::vec3 Renderer::get_camera_front()
{
#if defined(XR_SUPPORT)
    if (is_openxr_available) {
        glm::mat4x4 view = xr_context->per_view_data[0].view_matrix; // use left eye
        return { view[2].x, view[2].y, -view[2].z };
    }
#endif

    Camera* camera = get_camera();
    return glm::normalize(camera->get_center() - camera->get_eye());
}
