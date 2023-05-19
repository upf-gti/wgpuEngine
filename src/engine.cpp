#include "engine.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "utils.h"

int Engine::initialize(GLFWwindow *window) {

    if (xr_context.initialize()) {
        std::cout << "Could not initialize OpenXR context" << std::endl;
    }
    
    if (webgpu_context.initialize(&xr_context, window)) {
        std::cout << "Could not initialize WebGPU context" << std::endl;
        return 1;
    }

    return 0;
}

void Engine::clean() {
    xr_context.clean();
}

void Engine::render_frame() {

    if (xr_context.initialized) {
        xr_context.initFrame();

        renderXr();

        xr_context.endFrame();
    }

#ifdef USE_MIRROR_WINDOW
    renderMirror();
#endif
}

void Engine::renderXr()
{
    // Create the command encoder
    {
        wgpu::CommandEncoderDescriptor encoder_desc;
        encoder_desc.label = "Device command encoder";

        webgpu_context.device_command_encoder = webgpu_context.device.CreateCommandEncoder(&encoder_desc);
    }

    // Create & fill the render pass (encoder)
    wgpu::RenderPassEncoder render_pass;

    // Prepare the color attachment
    wgpu::RenderPassColorAttachment render_pass_color_attachment = {
        .view = webgpu_context.images[xr_context.swapchains[0].image_index].textureView,
        .loadOp = wgpu::LoadOp::Clear,
        .storeOp = wgpu::StoreOp::Store,
        .clearValue = wgpu::Color(0.0f, 0.0f, 1.0f, 1.0f)
    };
    wgpu::RenderPassDescriptor render_pass_descr = {
        .colorAttachmentCount = 1,
        .colorAttachments = &render_pass_color_attachment,
    };
    {
        render_pass = webgpu_context.device_command_encoder.BeginRenderPass(&render_pass_descr);

        // Bind Pipeline
        render_pass.SetPipeline(webgpu_context.render_pipeline);
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

void Engine::renderMirror()
{
    // Get the current texture in the swapchain
    wgpu::TextureView current_texture_view = webgpu_context.mirror_swapchain.GetCurrentTextureView();
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
            render_pass.SetPipeline(webgpu_context.mirror_render_pipeline);
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
        webgpu_context.mirror_swapchain.Present();
    }

    // Check validation errors
    dawn::native::InstanceProcessEvents(webgpu_context.instance->Get());
}
