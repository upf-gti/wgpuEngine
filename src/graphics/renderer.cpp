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
#endif

    initPipeline();

#ifdef USE_MIRROR_WINDOW
    initMirrorPipeline();
#endif

    return 0;
}

void Renderer::clean()
{
}

void Renderer::render()
{

    if (xr_context.initialized) {

        xr_context.initFrame();

        for (int i = 0; i < xr_context.view_count; ++i) {

            xr_context.acquireSwapchain(i);

            renderXr(i);

            xr_context.releaseSwapchain(i);
        }

        xr_context.endFrame();
    }

#ifdef USE_MIRROR_WINDOW
    renderMirror();
#endif
}

void Renderer::renderXr(int swapchain_index)
{
    // Create the command encoder
    wgpu::CommandEncoderDescriptor encoder_desc;
    encoder_desc.label = "Device command encoder";

    wgpu::CommandEncoder device_command_encoder = webgpu_context.device.CreateCommandEncoder(&encoder_desc);
    
    // Create & fill the render pass (encoder)
    wgpu::RenderPassEncoder render_pass;

    const sSwapchainData& swapchainData = xr_context.swapchains[swapchain_index];

    // Prepare the color attachment
    wgpu::RenderPassColorAttachment render_pass_color_attachment = {
        .view = swapchainData.images[swapchainData.image_index].textureView,
        .loadOp = wgpu::LoadOp::Clear,
        .storeOp = wgpu::StoreOp::Store,
        .clearValue = wgpu::Color(0.0f, 0.0f, 1.0f, 1.0f)
    };
    wgpu::RenderPassDescriptor render_pass_descr = {
        .colorAttachmentCount = 1,
        .colorAttachments = &render_pass_color_attachment,
    };
    {
        const glm::mat4x4& view = xr_context.per_view_data[swapchain_index].view_matrix;
        glm::mat4x4& projection = xr_context.per_view_data[swapchain_index].projection_matrix;
        projection[1][1] *= -1;

        glm::mat4x4 view_projection = projection * view;

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

#ifdef USE_MIRROR_WINDOW

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
            .clearValue = {0.0f,0.0f,1.0f,1.0f}
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

void Renderer::initPipeline()
{
    // Create uniform buffer
    uniform_viewprojection.data = webgpu_context.create_buffer(sizeof(glm::mat4x4), wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform, nullptr);
    uniform_viewprojection.binding = 0;

    shader_module = webgpu_context.create_shader_module(RAW_SHADERS::simple_shaders);

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

    render_pipeline = webgpu_context.create_render_pipeline({}, color_target, shader_module, render_pipeline_layout);
}

#ifdef USE_MIRROR_WINDOW

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