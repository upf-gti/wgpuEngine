#include "renderer.h"

#include "dawnxr/dawnxr_internal.h"
#include "raw_shaders.h"

int Renderer::initialize(GLFWwindow* window)
{

    webgpu_context.create_instance();

#ifdef USE_XR
    if (xr_context.createInstance()) {
        std::cout << "Could not create OpenXR instance" << std::endl;
        return 1;
    }

    // Create internal vulkan instance
    dawn::native::AdapterDiscoveryOptionsBase** options = new dawn::native::AdapterDiscoveryOptionsBase * ();
    dawnxr::internal::createVulkanAdapterDiscoveryOptions(xr_context.instance, xr_context.system_id, options);
    webgpu_context.instance->DiscoverAdapters(*options);

#else
    // Create internal vulkan instance
    webgpu_context.instance->DiscoverDefaultAdapters();
#endif

    if (webgpu_context.initialize(window)) {
        std::cout << "Could not initialize WebGPU context" << std::endl;
        return 1;
    }

#ifdef USE_XR
    if (xr_context.initialize(&webgpu_context)) {
        std::cout << "Could not initialize OpenXR context" << std::endl;
        return 1;
    }

    render_width = xr_context.viewconfig_views[0].recommendedImageRectWidth;
    render_height = xr_context.viewconfig_views[0].recommendedImageRectHeight;
#else
    render_width = webgpu_context.screen_width;
    render_height = webgpu_context.screen_height;
#endif

    initRenderPipeline();
    initComputePipeline();

#if defined(USE_XR) && defined(USE_MIRROR_WINDOW)
    initMirrorPipeline();
#endif

    return 0;
}

void Renderer::clean()
{
}

void Renderer::render()
{

#ifdef USE_XR
    if (xr_context.initialized) {

        xr_context.initFrame();

        for (int i = 0; i < xr_context.view_count; ++i) {

            xr_context.acquireSwapchain(i);

            const sSwapchainData& swapchainData = xr_context.swapchains[i];

            const glm::mat4x4& view = xr_context.per_view_data[i].view_matrix;
            glm::mat4x4& projection = xr_context.per_view_data[i].projection_matrix;
            projection[1][1] *= -1;

            glm::mat4x4 view_projection = projection * view;

            render(swapchainData.images[swapchainData.image_index].textureView, view_projection);

            xr_context.releaseSwapchain(i);
        }

        xr_context.endFrame();
    }
#else
    glm::mat4x4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4x4 projection = glm::perspective(glm::radians(45.0f), 16.0f/9.0f, 0.1f, 100.0f);

    glm::mat4x4 view_projection = projection * view;
    render(webgpu_context.screen_swapchain.GetCurrentTextureView(), view_projection);
    webgpu_context.screen_swapchain.Present();
#endif

#if defined(USE_XR) && defined(USE_MIRROR_WINDOW)
    renderMirror();
#endif
}

void Renderer::render(wgpu::TextureView swapchain_view, const glm::mat4x4& view_projection)
{
    compute();

    // Create the command encoder
    wgpu::CommandEncoderDescriptor encoder_desc = {};
    wgpu::CommandEncoder device_command_encoder = webgpu_context.device.CreateCommandEncoder(&encoder_desc);
    
    // Create & fill the render pass (encoder)
    wgpu::RenderPassEncoder render_pass;

    // Prepare the color attachment
    wgpu::RenderPassColorAttachment render_pass_color_attachment = {
        .view = swapchain_view,
        .loadOp = wgpu::LoadOp::Clear,
        .storeOp = wgpu::StoreOp::Store,
        .clearValue = wgpu::Color(0.0f, 0.0f, 0.0f, 1.0f)
    };
    wgpu::RenderPassDescriptor render_pass_descr = {
        .colorAttachmentCount = 1,
        .colorAttachments = &render_pass_color_attachment,
    };
    {
        // Update uniform buffer
        webgpu_context.device_queue.WriteBuffer(std::get<wgpu::Buffer>(uniform_viewprojection.data), 0, &(view_projection), sizeof(glm::mat4x4));

        render_pass = device_command_encoder.BeginRenderPass(&render_pass_descr);

        // Bind Pipeline
        render_pass.SetPipeline(render_pipeline);

        // Set binding group
        render_pass.SetBindGroup(0, render_bind_group, 0, nullptr);

        // Submit drawcall
        render_pass.Draw(3, 1, 0, 0);

        render_pass.End();
        //render_pass.Release();
    }

    wgpu::CommandBufferDescriptor cmd_buff_descriptor = {
        .nextInChain = NULL,
        .label = "Command buffer"
    };

    wgpu::CommandBuffer commander = device_command_encoder.Finish(&cmd_buff_descriptor);
    //webgpu_context.device_command_encoder.Release();
    webgpu_context.device_queue.Submit(1, &commander);

    // Check validation errors
    dawn::native::InstanceProcessEvents(webgpu_context.instance->Get());
}

void Renderer::compute()
{
    // Initialize a command encoder
    wgpu::CommandEncoderDescriptor encoder_desc = {};
    wgpu::CommandEncoder encoder = webgpu_context.device.CreateCommandEncoder(&encoder_desc);

    // Create compute pass
    wgpu::ComputePassDescriptor compute_pass_desc;
    compute_pass_desc.timestampWriteCount = 0;
    compute_pass_desc.timestampWrites = nullptr;
    wgpu::ComputePassEncoder compute_pass = encoder.BeginComputePass(&compute_pass_desc);

    // Use compute pass
    compute_pass.SetPipeline(compute_pipeline);
    compute_pass.SetBindGroup(0, compute_bind_group, 0, nullptr);

    uint32_t workgroupSize = 16;
    // This ceils invocationCount / workgroupSize
    uint32_t workgroupWidth = (render_width + workgroupSize - 1) / workgroupSize;
    uint32_t workgroupHeight = (render_height + workgroupSize - 1) / workgroupSize;
    compute_pass.DispatchWorkgroups(workgroupWidth, workgroupHeight, 1);

    // Finalize compute pass
    compute_pass.End();

    wgpu::CommandBufferDescriptor cmd_buff_descriptor = {};

    // Encode and submit the GPU commands
    wgpu::CommandBuffer commands = encoder.Finish(&cmd_buff_descriptor);
    webgpu_context.device_queue.Submit(1, &commands);

    dawn::native::InstanceProcessEvents(webgpu_context.instance->Get());
}

#if defined(USE_XR) && defined(USE_MIRROR_WINDOW)

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
            render_pass.SetVertexBuffer(0, vertex_buffer, 0, vertex_buffer.GetSize());

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
    dawn::native::InstanceProcessEvents(webgpu_context.instance->Get());
}

#endif

void Renderer::initRenderPipeline()
{
    // Create uniform buffer
    uniform_viewprojection.data = webgpu_context.create_buffer(sizeof(glm::mat4x4), wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform, nullptr);
    uniform_viewprojection.binding = 0;

    render_shader_module = webgpu_context.create_shader_module(RAW_SHADERS::simple_shaders);

    // Layout descriptor (bind goups, buffers, uniforms)
    {
        std::vector<Uniform> uniforms = { uniform_viewprojection };

        render_bind_group_layout = webgpu_context.create_bind_group_layout(uniforms);
        render_pipeline_layout = webgpu_context.create_pipeline_layout({ render_bind_group_layout });
        render_bind_group = webgpu_context.create_bind_group(uniforms, render_bind_group_layout);
    }

#ifdef USE_XR
    wgpu::TextureFormat swapchain_format = webgpu_context.xr_swapchain_format;
#else
    wgpu::TextureFormat swapchain_format = webgpu_context.swapchain_format;
#endif

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

    render_pipeline = webgpu_context.create_render_pipeline({}, color_target, render_shader_module, render_pipeline_layout);
}

void Renderer::initComputePipeline()
{
    // Create uniform buffer
    compute_texture = webgpu_context.create_texture(
        wgpu::TextureDimension::e2D,
        wgpu::TextureFormat::RGBA8Unorm,
        { render_width, render_height, 1 },
        wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::StorageBinding,
        1);

    uniform_compute_texture.data = webgpu_context.create_texture_view(compute_texture, wgpu::TextureViewDimension::e2D, wgpu::TextureFormat::RGBA8Unorm);
    uniform_compute_texture.binding = 0;
    uniform_compute_texture.visibility = wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute;
    uniform_compute_texture.is_storage_texture = true;

    // Load compute shader
    compute_shader_module = webgpu_context.create_shader_module(RAW_SHADERS::compute_shader);

    std::vector<Uniform> uniforms = { uniform_compute_texture };

    compute_bind_group_layout = webgpu_context.create_bind_group_layout(uniforms);
    compute_pipeline_layout = webgpu_context.create_pipeline_layout({ compute_bind_group_layout });
    compute_bind_group = webgpu_context.create_bind_group(uniforms, compute_bind_group_layout);

    compute_pipeline = webgpu_context.create_compute_pipeline(compute_shader_module, compute_pipeline_layout);
}

#if defined(USE_XR) && defined(USE_MIRROR_WINDOW)

void Renderer::initMirrorPipeline()
{
    // Create uniform for left eye texture view
    uniform_left_eye_view.data = xr_context.swapchains[0].images[0].textureView;
    uniform_left_eye_view.binding = 0;

    mirror_shader_module = webgpu_context.create_shader_module(RAW_SHADERS::mirror_shaders);

    // Layout descriptor (bind goups, buffers, uniforms)
    {
        std::vector<Uniform> uniforms = { uniform_left_eye_view };

        mirror_bind_group_layout = webgpu_context.create_bind_group_layout(uniforms);
        mirror_pipeline_layout = webgpu_context.create_pipeline_layout({ mirror_bind_group_layout });
        mirror_bind_group = webgpu_context.create_bind_group(uniforms, mirror_bind_group_layout);
    }

    // Vertex attributes
    wgpu::VertexAttribute vertex_attrib_position;
    vertex_attrib_position.shaderLocation = 0;
    vertex_attrib_position.format = wgpu::VertexFormat::Float32x2;
    vertex_attrib_position.offset = 0;

    wgpu::VertexAttribute vertex_attrib_uv;
    vertex_attrib_uv.shaderLocation = 1;
    vertex_attrib_uv.format = wgpu::VertexFormat::Float32x2;
    vertex_attrib_uv.offset = 2 * sizeof(float);

    const std::vector<wgpu::VertexAttribute> vertex_attributes = { vertex_attrib_position, vertex_attrib_uv };
    wgpu::VertexBufferLayout vertex_layout = webgpu_context.create_vertex_buffer_layout(vertex_attributes, 4 * sizeof(float), wgpu::VertexStepMode::Vertex);

    std::vector<float> vertexData = {
    //  position    uv
        -1.0, 1.0,  0.0, 1.0,
        -1.0,-1.0,  0.0, 0.0,
         1.0,-1.0,  1.0, 0.0,

        -1.0, 1.0,  0.0, 1.0,
         1.0,-1.0,  1.0, 0.0,
         1.0, 1.0,  1.0, 1.0
    };

    vertex_buffer = webgpu_context.create_buffer(vertexData.size() * sizeof(float), wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex, vertexData.data());

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

    mirror_pipeline = webgpu_context.create_render_pipeline({ vertex_layout }, color_target, mirror_shader_module, mirror_pipeline_layout);
}

#endif