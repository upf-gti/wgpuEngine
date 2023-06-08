#include "renderer.h"

#ifdef XR_SUPPORT
#include "dawnxr/dawnxr_internal.h"
#endif

#include "raw_shaders.h"

int Renderer::initialize(GLFWwindow* window, bool use_mirror_screen)
{
    bool create_screen_swapchain = true;

    this->use_mirror_screen = use_mirror_screen;

    webgpu_context.create_instance();

#ifdef XR_SUPPORT
    is_openxr_available = xr_context.create_instance();

    // Create internal vulkan instance
    if (is_openxr_available) {

        dawn::native::AdapterDiscoveryOptionsBase** options = new dawn::native::AdapterDiscoveryOptionsBase * ();
        dawnxr::internal::createVulkanAdapterDiscoveryOptions(xr_context.instance, xr_context.system_id, options);
        webgpu_context.instance->DiscoverAdapters(*options);

        create_screen_swapchain = use_mirror_screen;
    }

#else
    // Create internal vulkan instance
    //webgpu_context.instance->DiscoverDefaultAdapters();
#endif

    if (webgpu_context.initialize(window, create_screen_swapchain)) {
        std::cout << "Could not initialize WebGPU context" << std::endl;
        return 1;
    }

    render_width = webgpu_context.screen_width;
    render_height = webgpu_context.screen_height;

#ifdef XR_SUPPORT
    if (is_openxr_available && xr_context.initialize(&webgpu_context)) {
        std::cout << "Could not initialize OpenXR context" << std::endl;
        return 1;
    }

    if (is_openxr_available) {
        render_width = xr_context.viewconfig_views[0].recommendedImageRectWidth;
        render_height = xr_context.viewconfig_views[0].recommendedImageRectHeight;
    }
#endif

    init_render_pipeline();
    init_compute_pipeline();

#ifdef XR_SUPPORT
    if (is_openxr_available && use_mirror_screen) {
        init_mirror_pipeline();
    }
#endif

    return 0;
}

void Renderer::clean()
{
#ifdef XR_SUPPORT
    xr_context.clean();
#endif

    // Uniforms
    u_buffer_viewprojection.destroy();
    u_compute_texture_left_eye.destroy();
    u_compute_texture_right_eye.destroy();
    u_render_texture_left_eye.destroy();
    u_render_texture_right_eye.destroy();

    webgpu_context.destroy();

    // Render pipeline
    wgpuRenderPipelineRelease(render_pipeline);
    wgpuShaderModuleRelease(render_shader_module);
    wgpuPipelineLayoutRelease(render_pipeline_layout);
    wgpuBindGroupLayoutRelease(render_bind_group_layout);
    wgpuBindGroupRelease(render_bind_group_left_eye);
    wgpuBindGroupRelease(render_bind_group_right_eye);

    // Compute pipeline
    wgpuComputePipelineRelease(compute_pipeline);
    wgpuShaderModuleRelease(compute_shader_module);
    wgpuPipelineLayoutRelease(compute_pipeline_layout);
    wgpuBindGroupLayoutRelease(compute_bind_group_layout);
    wgpuBindGroupRelease(compute_bind_group);

    wgpuTextureDestroy(left_eye_texture);
    wgpuTextureDestroy(right_eye_texture);

    // Mesh
    quad_mesh.destroy();
//    std::vector<WGPUVertexAttribute>  quad_vertex_attributes;
//    WGPUVertexBufferLayout            quad_vertex_layout;

    wgpuBufferDestroy(quad_vertex_buffer);

    if (is_openxr_available) {
        wgpuRenderPipelineRelease(mirror_pipeline);
        wgpuPipelineLayoutRelease(mirror_pipeline_layout);
        wgpuBindGroupLayoutRelease(mirror_bind_group_layout);
        wgpuBindGroupRelease(mirror_bind_group);
        wgpuShaderModuleRelease(mirror_shader_module);

        uniform_left_eye_view.destroy();
    }
}

void Renderer::render()
{
    if (!is_openxr_available) {
        render_screen();
    }

#if defined(XR_SUPPORT)
    if (is_openxr_available) {
        render_xr();

        if (use_mirror_screen) {
            render_mirror();
        }
    }
#endif
}

void Renderer::render_screen()
{
    glm::mat4x4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4x4 projection = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);

    glm::mat4x4 view_projection = projection * view;
    render(wgpuSwapChainGetCurrentTextureView(webgpu_context.screen_swapchain), render_bind_group_left_eye, view_projection);

#ifndef __EMSCRIPTEN__
    wgpuSwapChainPresent(webgpu_context.screen_swapchain);
#endif
}

#if defined(XR_SUPPORT)

void Renderer::render_xr()
{
    if (is_openxr_available) {

        xr_context.init_frame();

        for (int i = 0; i < xr_context.view_count; ++i) {

            xr_context.acquire_swapchain(i);

            const sSwapchainData& swapchainData = xr_context.swapchains[i];

            const glm::mat4x4& view = xr_context.per_view_data[i].view_matrix;
            glm::mat4x4& projection = xr_context.per_view_data[i].projection_matrix;
            projection[1][1] *= -1;

            glm::mat4x4 view_projection = projection * view;

            WGPUBindGroup bind_group = i == 0 ? render_bind_group_left_eye : render_bind_group_right_eye;

            render(swapchainData.images[swapchainData.image_index].textureView, bind_group, view_projection);

            xr_context.release_swapchain(i);
        }

        xr_context.end_frame();
    }
    else {
        render_screen();
    }
}
#endif

void Renderer::render(WGPUTextureView swapchain_view, WGPUBindGroup bind_group, const glm::mat4x4& view_projection)
{
    compute();

    // Create the command encoder
    WGPUCommandEncoderDescriptor encoder_desc = {};
    WGPUCommandEncoder command_encoder = wgpuDeviceCreateCommandEncoder(webgpu_context.device, &encoder_desc);
    
    // Prepare the color attachment
    WGPURenderPassColorAttachment render_pass_color_attachment = {};
    render_pass_color_attachment.view = swapchain_view;
    render_pass_color_attachment.loadOp = WGPULoadOp_Clear;
    render_pass_color_attachment.storeOp = WGPUStoreOp_Store;
    render_pass_color_attachment.clearValue = WGPUColor(0.0f, 0.0f, 0.0f, 1.0f);

    WGPURenderPassDescriptor render_pass_descr = {};
    render_pass_descr.colorAttachmentCount = 1;
    render_pass_descr.colorAttachments = &render_pass_color_attachment;

    {
        // Update uniform buffer
        wgpuQueueWriteBuffer(webgpu_context.device_queue, std::get<WGPUBuffer>(u_buffer_viewprojection.data), 0, &(view_projection), sizeof(glm::mat4x4));

        // Create & fill the render pass (encoder)
        WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(command_encoder, &render_pass_descr);

        // Bind Pipeline
        wgpuRenderPassEncoderSetPipeline(render_pass, render_pipeline);

        // Set binding group
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, bind_group, 0, nullptr);

        // Set vertex buffer while encoding the render pass
        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, quad_vertex_buffer, 0, quad_mesh.get_size() * sizeof(float));

        // Submit drawcall
        wgpuRenderPassEncoderDraw(render_pass, 6, 1, 0, 0);

        wgpuRenderPassEncoderEnd(render_pass);
        //render_pass.Release();
    }

    WGPUCommandBufferDescriptor cmd_buff_descriptor = {};
    cmd_buff_descriptor.nextInChain = NULL;
    cmd_buff_descriptor.label = "Command buffer";

    WGPUCommandBuffer commands = wgpuCommandEncoderFinish(command_encoder, &cmd_buff_descriptor);
   
    wgpuQueueSubmit(webgpu_context.device_queue, 1, &commands);
    //webgpu_context.device_command_encoder.Release();

    // Check validation errors
    webgpu_context.printErrors();

}

void Renderer::compute()
{
    // Initialize a command encoder
    WGPUCommandEncoderDescriptor encoder_desc = {};
    WGPUCommandEncoder command_encoder = wgpuDeviceCreateCommandEncoder(webgpu_context.device, &encoder_desc);

    // Create compute pass
    WGPUComputePassDescriptor compute_pass_desc = {};
    compute_pass_desc.timestampWriteCount = 0;
    compute_pass_desc.timestampWrites = nullptr;
    WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(command_encoder, &compute_pass_desc);

    // Use compute pass
    wgpuComputePassEncoderSetPipeline(compute_pass, compute_pipeline);
    wgpuComputePassEncoderSetBindGroup(compute_pass, 0, compute_bind_group, 0, nullptr);

    uint32_t workgroupSize = 16;
    // This ceils invocationCount / workgroupSize
    uint32_t workgroupWidth = (render_width + workgroupSize - 1) / workgroupSize;
    uint32_t workgroupHeight = (render_height + workgroupSize - 1) / workgroupSize;
    wgpuComputePassEncoderDispatchWorkgroups(compute_pass, workgroupWidth, workgroupHeight, 1);

    // Finalize compute pass
    wgpuComputePassEncoderEnd(compute_pass);

    WGPUCommandBufferDescriptor cmd_buff_descriptor = {};

    // Encode and submit the GPU commands
    WGPUCommandBuffer commands = wgpuCommandEncoderFinish(command_encoder, &cmd_buff_descriptor);
    wgpuQueueSubmit(webgpu_context.device_queue, 1, &commands);

    // Check validation errors
    webgpu_context.printErrors();
}

#if defined(XR_SUPPORT) && defined(USE_MIRROR_WINDOW)

void Renderer::render_mirror()
{
    // Get the current texture in the swapchain
    WGPUTextureView current_texture_view = wgpuSwapChainGetCurrentTextureView(webgpu_context.screen_swapchain);
    assert_msg(current_texture_view != NULL, "Error, dont resize the window please!!");

    // Create the command encoder
    WGPUCommandEncoderDescriptor encoder_desc = {};
    encoder_desc.label = "Device command encoder";

    WGPUCommandEncoder command_encoder = wgpuDeviceCreateCommandEncoder(webgpu_context.device, &encoder_desc);

    // Create & fill the render pass (encoder)
    {
        // Prepare the color attachment
        WGPURenderPassColorAttachment render_pass_color_attachment = {};
        render_pass_color_attachment.view = current_texture_view;
        render_pass_color_attachment.loadOp = WGPULoadOp_Clear;
        render_pass_color_attachment.storeOp = WGPUStoreOp_Store;
        render_pass_color_attachment.clearValue = WGPUColor(0.0f, 0.0f, 0.0f, 1.0f);

        WGPURenderPassDescriptor render_pass_descr = {};
        render_pass_descr.colorAttachmentCount = 1;
        render_pass_descr.colorAttachments = &render_pass_color_attachment;

        {
            WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(command_encoder, &render_pass_descr);

            // Bind Pipeline
            wgpuRenderPassEncoderSetPipeline(render_pass, mirror_pipeline);

            // Set binding group
            wgpuRenderPassEncoderSetBindGroup(render_pass, 0, mirror_bind_group, 0, nullptr);

            // Set vertex buffer while encoding the render pass
            wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, quad_vertex_buffer, 0, quad_mesh.get_size() * sizeof(float));

            // Submit drawcall
            wgpuRenderPassEncoderDraw(render_pass, 6, 1, 0, 0);

            wgpuRenderPassEncoderEnd(render_pass);
        }
    }

    //
    {
        WGPUCommandBufferDescriptor cmd_buff_descriptor = {};
        cmd_buff_descriptor.nextInChain = NULL;
        cmd_buff_descriptor.label = "Command buffer";

        WGPUCommandBuffer commands = wgpuCommandEncoderFinish(command_encoder, &cmd_buff_descriptor);
        wgpuQueueSubmit(webgpu_context.device_queue, 1, &commands);

        //commander.Release();
        //current_texture_view.Release();
    }

    // Submit frame to mirror window
    {
        wgpuSwapChainPresent(webgpu_context.screen_swapchain);
    }

    // Check validation errors
    webgpu_context.printErrors();
}

#endif

void Renderer::init_render_pipeline()
{
    left_eye_texture = webgpu_context.create_texture(
        WGPUTextureDimension_2D,
        WGPUTextureFormat_RGBA8Unorm,
        { render_width, render_height, 1 },
        WGPUTextureUsage_TextureBinding | WGPUTextureUsage_StorageBinding,
        1);

    right_eye_texture = webgpu_context.create_texture(
        WGPUTextureDimension_2D,
        WGPUTextureFormat_RGBA8Unorm,
        { render_width, render_height, 1 },
        WGPUTextureUsage_TextureBinding | WGPUTextureUsage_StorageBinding,
        1);

    u_render_texture_left_eye.data = webgpu_context.create_texture_view(left_eye_texture, WGPUTextureViewDimension_2D, WGPUTextureFormat_RGBA8Unorm);
    u_render_texture_left_eye.binding = 0;
    u_render_texture_left_eye.visibility = WGPUShaderStage_Fragment;

    u_render_texture_right_eye.data = webgpu_context.create_texture_view(right_eye_texture, WGPUTextureViewDimension_2D, WGPUTextureFormat_RGBA8Unorm);
    u_render_texture_right_eye.binding = 0;
    u_render_texture_right_eye.visibility = WGPUShaderStage_Fragment;

    render_shader_module = webgpu_context.create_shader_module(RAW_SHADERS::simple_shaders);

    // Left eye bind group
    {
        std::vector<Uniform*> uniforms = { &u_render_texture_left_eye };

        // shared with right eye
        render_bind_group_layout = webgpu_context.create_bind_group_layout(uniforms);

        render_bind_group_left_eye = webgpu_context.create_bind_group(uniforms, render_bind_group_layout);
    }

    // Right eye bind group
    {
        std::vector<Uniform*> uniforms = { &u_render_texture_right_eye };

        // render_bind_group_layout is the same as left eye
        render_bind_group_right_eye = webgpu_context.create_bind_group(uniforms, render_bind_group_layout);
    }

    render_pipeline_layout = webgpu_context.create_pipeline_layout({ render_bind_group_layout });

    // Vertex attributes
    WGPUVertexAttribute vertex_attrib_position;
    vertex_attrib_position.shaderLocation = 0;
    vertex_attrib_position.format = WGPUVertexFormat_Float32x2;
    vertex_attrib_position.offset = 0;

    WGPUVertexAttribute vertex_attrib_uv;
    vertex_attrib_uv.shaderLocation = 1;
    vertex_attrib_uv.format = WGPUVertexFormat_Float32x2;
    vertex_attrib_uv.offset = 2 * sizeof(float);

    quad_vertex_attributes = { vertex_attrib_position, vertex_attrib_uv };
    quad_vertex_layout = webgpu_context.create_vertex_buffer_layout(quad_vertex_attributes, 4 * sizeof(float), WGPUVertexStepMode_Vertex);

    quad_mesh.create_quad();

    quad_vertex_buffer = webgpu_context.create_buffer(quad_mesh.get_size() * sizeof(float), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex, quad_mesh.data());

    WGPUTextureFormat swapchain_format = is_openxr_available ? webgpu_context.xr_swapchain_format : webgpu_context.swapchain_format;

    WGPUBlendState blend_state;
    blend_state.color = {
            .operation = WGPUBlendOperation_Add,
            .srcFactor = WGPUBlendFactor_SrcAlpha,
            .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
    };
    blend_state.alpha = {
            .operation = WGPUBlendOperation_Add,
            .srcFactor = WGPUBlendFactor_Zero,
            .dstFactor = WGPUBlendFactor_One,
    };

    WGPUColorTargetState color_target = {};
    color_target.format = swapchain_format;
    color_target.blend = &blend_state;
    color_target.writeMask = WGPUColorWriteMask_All;

    render_pipeline = webgpu_context.create_render_pipeline({ quad_vertex_layout }, color_target, render_shader_module, render_pipeline_layout);
}

void Renderer::init_compute_pipeline()
{
    u_buffer_viewprojection.data = webgpu_context.create_buffer(sizeof(glm::mat4x4), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, nullptr);
    u_buffer_viewprojection.binding = 0;

    u_compute_texture_left_eye.data = webgpu_context.create_texture_view(left_eye_texture, WGPUTextureViewDimension_2D, WGPUTextureFormat_RGBA8Unorm);
    u_compute_texture_left_eye.binding = 0;
    u_compute_texture_left_eye.visibility = WGPUShaderStage_Compute;
    u_compute_texture_left_eye.is_storage_texture = true;
    u_compute_texture_left_eye.storage_texture_binding_layout.access = WGPUStorageTextureAccess_WriteOnly;
    u_compute_texture_left_eye.storage_texture_binding_layout.format = WGPUTextureFormat_RGBA8Unorm;
    u_compute_texture_left_eye.storage_texture_binding_layout.viewDimension = WGPUTextureViewDimension_2D;

    u_compute_texture_right_eye.data = webgpu_context.create_texture_view(right_eye_texture, WGPUTextureViewDimension_2D, WGPUTextureFormat_RGBA8Unorm);
    u_compute_texture_right_eye.binding = 1;
    u_compute_texture_right_eye.visibility = WGPUShaderStage_Compute;
    u_compute_texture_right_eye.is_storage_texture = true;
    u_compute_texture_right_eye.storage_texture_binding_layout.access = WGPUStorageTextureAccess_WriteOnly;
    u_compute_texture_right_eye.storage_texture_binding_layout.format = WGPUTextureFormat_RGBA8Unorm;
    u_compute_texture_right_eye.storage_texture_binding_layout.viewDimension = WGPUTextureViewDimension_2D;

    // Load compute shader
    compute_shader_module = webgpu_context.create_shader_module(RAW_SHADERS::compute_shader);
    
    std::vector<Uniform*> uniforms = { &u_compute_texture_left_eye, &u_compute_texture_right_eye };

    compute_bind_group_layout = webgpu_context.create_bind_group_layout(uniforms);
    compute_pipeline_layout = webgpu_context.create_pipeline_layout({ compute_bind_group_layout });
    compute_bind_group = webgpu_context.create_bind_group(uniforms, compute_bind_group_layout);

    compute_pipeline = webgpu_context.create_compute_pipeline(compute_shader_module, compute_pipeline_layout);
}

bool Renderer::get_openxr_available()
{
    return is_openxr_available;
}

#if defined(XR_SUPPORT) && defined(USE_MIRROR_WINDOW)

void Renderer::init_mirror_pipeline()
{
    // Create uniform for left eye texture view
    uniform_left_eye_view.data = xr_context.swapchains[0].images[0].textureView;
    uniform_left_eye_view.binding = 0;

    mirror_shader_module = webgpu_context.create_shader_module(RAW_SHADERS::mirror_shaders);

    // Layout descriptor (bind goups, buffers, uniforms)
    {
        std::vector<Uniform*> uniforms = { &uniform_left_eye_view };

        mirror_bind_group_layout = webgpu_context.create_bind_group_layout(uniforms);
        mirror_pipeline_layout = webgpu_context.create_pipeline_layout({ mirror_bind_group_layout });
        mirror_bind_group = webgpu_context.create_bind_group(uniforms, mirror_bind_group_layout);
    }

    WGPUTextureFormat swapchain_format = webgpu_context.swapchain_format;

    WGPUBlendState blend_state;
    blend_state.color = {
            .operation = WGPUBlendOperation_Add,
            .srcFactor = WGPUBlendFactor_SrcAlpha,
            .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
    };
    blend_state.alpha = {
            .operation = WGPUBlendOperation_Add,
            .srcFactor = WGPUBlendFactor_Zero,
            .dstFactor = WGPUBlendFactor_One,
    };

    WGPUColorTargetState color_target = {};
    color_target.format = swapchain_format;
    color_target.blend = &blend_state;
    color_target.writeMask = WGPUColorWriteMask_All;

    mirror_pipeline = webgpu_context.create_render_pipeline({ quad_vertex_layout }, color_target, mirror_shader_module, mirror_pipeline_layout);
}

#endif