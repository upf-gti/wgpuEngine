#include "renderer.h"

#ifdef XR_SUPPORT

#include "dawnxr/dawnxr_internal.h"

#endif

#include "raw_shaders.h"
#include <intrin.h>

int Renderer::initialize(GLFWwindow* window, bool use_mirror_screen)
{
    this->use_mirror_screen = use_mirror_screen;

    webgpu_context.create_instance();

#ifdef XR_SUPPORT
    is_openxr_available = xr_context.isOpenXRAvailable();

    if (is_openxr_available && xr_context.createInstance()) {
        std::cout << "Could not create OpenXR instance" << std::endl;
        return 1;
    }

    // Create internal vulkan instance
    if (is_openxr_available) {

        dawn::native::AdapterDiscoveryOptionsBase** options = new dawn::native::AdapterDiscoveryOptionsBase * ();
        dawnxr::internal::createVulkanAdapterDiscoveryOptions(xr_context.instance, xr_context.system_id, options);
        webgpu_context.instance->DiscoverAdapters(*options);
    }
    else {
        webgpu_context.instance->DiscoverDefaultAdapters();
    }

#else
    // Create internal vulkan instance
    //webgpu_context.instance->DiscoverDefaultAdapters();
#endif

    if (webgpu_context.initialize(window, !is_openxr_available || (is_openxr_available && use_mirror_screen))) {
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

    initRenderPipeline();
    initComputePipeline();

#ifdef XR_SUPPORT
    if (is_openxr_available && use_mirror_screen) {
        initMirrorPipeline();
    }
#endif

    return 0;
}

void Renderer::clean()
{
}

void Renderer::render()
{

#if defined(XR_SUPPORT)
    renderXr();
#else
    renderScreen();
#endif

#if defined(XR_SUPPORT)
    if (use_mirror_screen && is_openxr_available && xr_context.initialized) {
        renderMirror();
    }
#endif
}

void Renderer::renderScreen()
{
    glm::mat4x4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4x4 projection = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);

    glm::mat4x4 view_projection = projection * view;
    render(webgpu_context.screen_swapchain.getCurrentTextureView(), render_bind_group_left_eye, view_projection);
    webgpu_context.screen_swapchain.present();
}

#if defined(XR_SUPPORT)

void Renderer::renderXr()
{
    if (is_openxr_available && xr_context.initialized) {
        xr_context.initFrame();

        for (int i = 0; i < xr_context.view_count; ++i) {

            xr_context.acquireSwapchain(i);

            const sSwapchainData& swapchainData = xr_context.swapchains[i];

            const glm::mat4x4& view = xr_context.per_view_data[i].view_matrix;
            glm::mat4x4& projection = xr_context.per_view_data[i].projection_matrix;
            projection[1][1] *= -1;

            glm::mat4x4 view_projection = projection * view;

            wgpu::BindGroup bind_group = i == 0 ? render_bind_group_left_eye : render_bind_group_right_eye;

            render(swapchainData.images[swapchainData.image_index].textureView, bind_group, view_projection);

            xr_context.releaseSwapchain(i);
        }

        xr_context.endFrame();
    }
    else {
        renderScreen();
    }
}
#endif

void Renderer::render(wgpu::TextureView swapchain_view, wgpu::BindGroup bind_group, const glm::mat4x4& view_projection)
{
    compute();

    // Create the command encoder
    wgpu::CommandEncoderDescriptor encoder_desc = {};
    wgpu::CommandEncoder device_command_encoder = webgpu_context.device.createCommandEncoder(encoder_desc);
    
    // Prepare the color attachment
    wgpu::RenderPassColorAttachment render_pass_color_attachment = {};
    render_pass_color_attachment.view = swapchain_view;
    render_pass_color_attachment.loadOp = wgpu::LoadOp::Clear;
    render_pass_color_attachment.storeOp = wgpu::StoreOp::Store;
    render_pass_color_attachment.clearValue = wgpu::Color(0.0f, 0.0f, 0.0f, 1.0f);

    wgpu::RenderPassDescriptor render_pass_descr = {};
    render_pass_descr.colorAttachmentCount = 1;
    render_pass_descr.colorAttachments = &render_pass_color_attachment;

    {
        // Update uniform buffer
        webgpu_context.device_queue.writeBuffer(std::get<WGPUBuffer>(u_buffer_viewprojection.data), 0, &(view_projection), sizeof(glm::mat4x4));

        // Create & fill the render pass (encoder)
        wgpu::RenderPassEncoder render_pass = device_command_encoder.beginRenderPass(render_pass_descr);

        // Bind Pipeline
        render_pass.setPipeline(render_pipeline);

        // Set binding group
        render_pass.setBindGroup(0, bind_group, 0, nullptr);

        // Set vertex buffer while encoding the render pass
        render_pass.setVertexBuffer(0, quad_vertex_buffer, 0, quad_vertex_buffer.getSize());

        // Submit drawcall
        render_pass.draw(6, 1, 0, 0);

        render_pass.end();
        //render_pass.Release();
    }

    wgpu::CommandBufferDescriptor cmd_buff_descriptor = {};
    cmd_buff_descriptor.nextInChain = NULL;
    cmd_buff_descriptor.label = "Command buffer";

    wgpu::CommandBuffer commander = device_command_encoder.finish(cmd_buff_descriptor);
    //webgpu_context.device_command_encoder.Release();
    webgpu_context.device_queue.submit(1, &commander);

    // Check validation errors
    webgpu_context.printErrors();

}

void Renderer::compute()
{
    // Initialize a command encoder
    wgpu::CommandEncoderDescriptor encoder_desc = {};
    wgpu::CommandEncoder encoder = webgpu_context.device.createCommandEncoder(encoder_desc);

    // Create compute pass
    wgpu::ComputePassDescriptor compute_pass_desc;
    compute_pass_desc.timestampWriteCount = 0;
    compute_pass_desc.timestampWrites = nullptr;
    wgpu::ComputePassEncoder compute_pass = encoder.beginComputePass(compute_pass_desc);

    // Use compute pass
    compute_pass.setPipeline(compute_pipeline);
    compute_pass.setBindGroup(0, compute_bind_group, 0, nullptr);

    uint32_t workgroupSize = 16;
    // This ceils invocationCount / workgroupSize
    uint32_t workgroupWidth = (render_width + workgroupSize - 1) / workgroupSize;
    uint32_t workgroupHeight = (render_height + workgroupSize - 1) / workgroupSize;
    compute_pass.dispatchWorkgroups(workgroupWidth, workgroupHeight, 1);

    // Finalize compute pass
    compute_pass.end();

    wgpu::CommandBufferDescriptor cmd_buff_descriptor = {};

    // Encode and submit the GPU commands
    wgpu::CommandBuffer commands = encoder.finish(cmd_buff_descriptor);
    webgpu_context.device_queue.submit(1, &commands);

    // Check validation errors
    webgpu_context.printErrors();
}

#if defined(XR_SUPPORT) && defined(USE_MIRROR_WINDOW)

void Renderer::renderMirror()
{
    // Get the current texture in the swapchain
    wgpu::TextureView current_texture_view = webgpu_context.screen_swapchain.GetCurrentTextureView();
    assert_msg(current_texture_view != NULL, "Error, dont resize the window please!!");

    // Create the command encoder
    wgpu::CommandEncoderDescriptor encoder_desc;
    encoder_desc.label = "Device command encoder";

    wgpu::CommandEncoder device_command_encoder = webgpu_context.device.CreateCommandEncoder(&encoder_desc);
    
    // Create & fill the render pass (encoder)
    wgpu::RenderPassEncoder render_pass;
    {
        // Prepare the color attachment
        wgpu::RenderPassColorAttachment render_pass_color_attachment = {
            .view = current_texture_view,
            .loadOp = wgpu::LoadOp::Clear,
            .storeOp = wgpu::StoreOp::Store,
            .clearValue = {0.0f,0.0f,0.0f,1.0f}
        };
        wgpu::RenderPassDescriptor render_pass_descr = {
            .colorAttachmentCount = 1,
            .colorAttachments = &render_pass_color_attachment,
        };
        {
            render_pass = device_command_encoder.BeginRenderPass(&render_pass_descr);

            // Bind Pipeline
            render_pass.SetPipeline(mirror_pipeline);

            // Set binding group
            render_pass.SetBindGroup(0, mirror_bind_group, 0, nullptr);

            // Set vertex buffer while encoding the render pass
            render_pass.SetVertexBuffer(0, quad_vertex_buffer, 0, quad_vertex_buffer.GetSize());

            // Submit drawcall
            render_pass.Draw(6, 1, 0, 0);

            render_pass.End();
            //render_pass.Release();
        }
    }

    //
    {
        wgpu::CommandBufferDescriptor cmd_buff_descriptor = {
            .nextInChain = NULL,
            .label = "Command buffer"
        };

        wgpu::CommandBuffer commander = device_command_encoder.Finish(&cmd_buff_descriptor);
        //webgpu_context.device_command_encoder.Release();
        webgpu_context.device_queue.Submit(1, &commander);

        //commander.Release();
        //current_texture_view.Release();
    }

    // Submit frame to mirror window
    {
        webgpu_context.screen_swapchain.Present();
    }

    // Check validation errors
    webgpu_context.printErrors();
}

#endif

void Renderer::initRenderPipeline()
{
    left_eye_texture = webgpu_context.create_texture(
        wgpu::TextureDimension::_2D,
        wgpu::TextureFormat::RGBA8Unorm,
        { render_width, render_height, 1 },
        wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::StorageBinding,
        1);

    right_eye_texture = webgpu_context.create_texture(
        wgpu::TextureDimension::_2D,
        wgpu::TextureFormat::RGBA8Unorm,
        { render_width, render_height, 1 },
        wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::StorageBinding,
        1);

    u_render_texture_left_eye.data = webgpu_context.create_texture_view(left_eye_texture, wgpu::TextureViewDimension::_2D, wgpu::TextureFormat::RGBA8Unorm);
    u_render_texture_left_eye.binding = 0;
    u_render_texture_left_eye.visibility = wgpu::ShaderStage::Fragment;
    u_render_texture_left_eye.type =  Uniform::TEXTURE_VIEW;

    u_render_texture_right_eye.data = webgpu_context.create_texture_view(right_eye_texture, wgpu::TextureViewDimension::_2D, wgpu::TextureFormat::RGBA8Unorm);
    u_render_texture_right_eye.binding = 0;
    u_render_texture_right_eye.visibility = wgpu::ShaderStage::Fragment;
    u_render_texture_right_eye.type = Uniform::TEXTURE_VIEW;

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
    wgpu::VertexAttribute vertex_attrib_position;
    vertex_attrib_position.shaderLocation = 0;
    vertex_attrib_position.format = wgpu::VertexFormat::Float32x2;
    vertex_attrib_position.offset = 0;

    wgpu::VertexAttribute vertex_attrib_uv;
    vertex_attrib_uv.shaderLocation = 1;
    vertex_attrib_uv.format = wgpu::VertexFormat::Float32x2;
    vertex_attrib_uv.offset = 2 * sizeof(float);

    quad_vertex_attributes = { vertex_attrib_position, vertex_attrib_uv };
    quad_vertex_layout = webgpu_context.create_vertex_buffer_layout(quad_vertex_attributes, 4 * sizeof(float), wgpu::VertexStepMode::Vertex);

    std::vector<float> vertexData = {
        //  position    uv
            -1.0, 1.0,  0.0, 1.0,
            -1.0,-1.0,  0.0, 0.0,
             1.0,-1.0,  1.0, 0.0,

            -1.0, 1.0,  0.0, 1.0,
             1.0,-1.0,  1.0, 0.0,
             1.0, 1.0,  1.0, 1.0
    };

    quad_vertex_buffer = webgpu_context.create_buffer(vertexData.size() * sizeof(float), wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex, vertexData.data());

    wgpu::TextureFormat swapchain_format = is_openxr_available ? webgpu_context.xr_swapchain_format : webgpu_context.swapchain_format;

    wgpu::BlendState blend_state;
    blend_state.color = {
            .operation = wgpu::BlendOperation::Add,
            .srcFactor = wgpu::BlendFactor::SrcAlpha,
            .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha,
    };
    blend_state.alpha = {
            .operation = wgpu::BlendOperation::Add,
            .srcFactor = wgpu::BlendFactor::Zero,
            .dstFactor = wgpu::BlendFactor::One,
    };

    wgpu::ColorTargetState color_target;
    color_target.format = swapchain_format;
    color_target.blend = &blend_state;
    color_target.writeMask = wgpu::ColorWriteMask::All;

    render_pipeline = webgpu_context.create_render_pipeline({ quad_vertex_layout }, color_target, render_shader_module, render_pipeline_layout);
}

void Renderer::initComputePipeline()
{
    u_buffer_viewprojection.data = webgpu_context.create_buffer(sizeof(glm::mat4x4), wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform, nullptr);
    u_buffer_viewprojection.binding = 0;

    u_compute_texture_left_eye.data = webgpu_context.create_texture_view(left_eye_texture, wgpu::TextureViewDimension::_2D, wgpu::TextureFormat::RGBA8Unorm);
    u_compute_texture_left_eye.binding = 0;
    u_compute_texture_left_eye.visibility = wgpu::ShaderStage::Compute;
    u_compute_texture_left_eye.is_storage_texture = true;
    u_compute_texture_left_eye.storage_texture_binding_layout.access = wgpu::StorageTextureAccess::WriteOnly;
    u_compute_texture_left_eye.storage_texture_binding_layout.format = wgpu::TextureFormat::RGBA8Unorm;
    u_compute_texture_left_eye.storage_texture_binding_layout.viewDimension = wgpu::TextureViewDimension::_2D;
    u_compute_texture_left_eye.type = Uniform::TEXTURE_VIEW;

    u_compute_texture_right_eye.data = webgpu_context.create_texture_view(right_eye_texture, wgpu::TextureViewDimension::_2D, wgpu::TextureFormat::RGBA8Unorm);
    u_compute_texture_right_eye.binding = 1;
    u_compute_texture_right_eye.visibility = wgpu::ShaderStage::Compute;
    u_compute_texture_right_eye.is_storage_texture = true;
    u_compute_texture_right_eye.storage_texture_binding_layout.access = wgpu::StorageTextureAccess::WriteOnly;
    u_compute_texture_right_eye.storage_texture_binding_layout.format = wgpu::TextureFormat::RGBA8Unorm;
    u_compute_texture_right_eye.storage_texture_binding_layout.viewDimension = wgpu::TextureViewDimension::_2D;
    u_compute_texture_right_eye.type = Uniform::TEXTURE_VIEW;

    // Load compute shader
    compute_shader_module = webgpu_context.create_shader_module(RAW_SHADERS::compute_shader);
    
    std::vector<Uniform*> uniforms = { &u_compute_texture_left_eye, &u_compute_texture_right_eye };

    compute_bind_group_layout = webgpu_context.create_bind_group_layout(uniforms);
    compute_pipeline_layout = webgpu_context.create_pipeline_layout({ compute_bind_group_layout });
    compute_bind_group = webgpu_context.create_bind_group(uniforms, compute_bind_group_layout);

    compute_pipeline = webgpu_context.create_compute_pipeline(compute_shader_module, compute_pipeline_layout);
}

bool Renderer::isOpenXRAvailable()
{
    return is_openxr_available;
}

#if defined(XR_SUPPORT) && defined(USE_MIRROR_WINDOW)

void Renderer::initMirrorPipeline()
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

    wgpu::TextureFormat swapchain_format = webgpu_context.swapchain_format;

    wgpu::BlendState blend_state = {
        .color = {
            .operation = wgpu::BlendOperation::Add,
            .srcFactor = wgpu::BlendFactor::SrcAlpha,
            .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha,
        },
        .alpha = {
            .operation = wgpu::BlendOperation::Add,
            .srcFactor = wgpu::BlendFactor::Zero,
            .dstFactor = wgpu::BlendFactor::One,
        }
    };

    wgpu::ColorTargetState color_target = {
        .format = swapchain_format,
        .blend = &blend_state,
        .writeMask = wgpu::ColorWriteMask::All
    };

    mirror_pipeline = webgpu_context.create_render_pipeline({ quad_vertex_layout }, color_target, mirror_shader_module, mirror_pipeline_layout);
}

#endif