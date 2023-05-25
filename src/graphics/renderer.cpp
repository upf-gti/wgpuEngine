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
    // Create the swapchain for mirror mode
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    auto surfaceChainedDesc = wgpu::glfw::SetupWindowAndGetSurfaceDescriptor(window);
    wgpu::SurfaceDescriptor surfaceDesc;
    surfaceDesc.nextInChain = surfaceChainedDesc.get();
    surface = get_surface(window);

    wgpu::SwapChainDescriptor swapChainDesc = {};
    swapChainDesc.usage = wgpu::TextureUsage::RenderAttachment;
    swapChainDesc.format = wgpu::TextureFormat::BGRA8Unorm;
    swapChainDesc.width = width;
    swapChainDesc.height = height;
    swapChainDesc.presentMode = wgpu::PresentMode::Mailbox;
    mirror_swapchain = device.CreateSwapChain(surface, &swapChainDesc);

    mirror_swapchain_format = wgpu::TextureFormat::BGRA8Unorm;
    config_mirror_render_pipeline();
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
    {
        wgpu::CommandEncoderDescriptor encoder_desc;
        encoder_desc.label = "Device command encoder";

        webgpu_context.device_command_encoder = webgpu_context.device.CreateCommandEncoder(&encoder_desc);
    }

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

        render_pass = webgpu_context.device_command_encoder.BeginRenderPass(&render_pass_descr);

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

    wgpu::CommandBuffer commander = webgpu_context.device_command_encoder.Finish(&cmd_buff_descriptor);
    //webgpu_context.device_command_encoder.Release();
    webgpu_context.device_queue.Submit(1, &commander);

    // Check validation errors
    dawn::native::InstanceProcessEvents(webgpu_context.instance->Get());
}

void Renderer::renderMirror()
{
    /*
    // Get the current texture in the swapchain
    wgpu::TextureView current_texture_view = mirror_swapchain.GetCurrentTextureView();
    assert_msg(current_texture_view != NULL, "Error, dont resize the window please!!");

    // Create the command encoder
    {
        wgpu::CommandEncoderDescriptor encoder_desc;
        encoder_desc.label = "Device command encoder";

        webgpu_context.device_command_encoder = webgpu_context.device.CreateCommandEncoder(&encoder_desc);
    }

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
            render_pass = webgpu_context.device_command_encoder.BeginRenderPass(&render_pass_descr);

            // Bind Pipeline
            render_pass.SetPipeline(mirror_render_pipeline);
            // Submit drawcall
            render_pass.Draw(3, 1, 0, 0);

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

        wgpu::CommandBuffer commander = webgpu_context.device_command_encoder.Finish(&cmd_buff_descriptor);
        //webgpu_context.device_command_encoder.Release();
        webgpu_context.device_queue.Submit(1, &commander);

        //commander.Release();
        //current_texture_view.Release();
    }

    // Submit frame to mirror window
    {
        mirror_swapchain.Present();
    }

    // Check validation errors
    dawn::native::InstanceProcessEvents(webgpu_context.instance->Get());
    */
}

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

    render_pipeline = webgpu_context.create_render_pipeline(color_target, shader_module, render_pipeline_layout);
}

void Renderer::initMirrorPipeline()
{

}
