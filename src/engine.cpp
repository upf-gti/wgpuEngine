#include "engine.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "utils.h"

WGPUSurface get_surface(GLFWwindow *window, WGPUInstance wgpuInstance) {
#if WGPU_TARGET == WGPU_TARGET_WINDOWS
    HWND hwnd = glfwGetWin32Window(window);
    HINSTANCE hinstance = GetModuleHandle(NULL);

    const WGPUSurfaceDescriptorFromWindowsHWND chained = {
                    .chain = {
                            .next = NULL,
                            .sType = WGPUSType_SurfaceDescriptorFromWindowsHWND,
                        },
                    .hinstance = hinstance,
                    .hwnd = hwnd,
                };
    
    WGPUSurfaceDescriptor descriptor = {
            .nextInChain =
                (const WGPUChainedStruct *)&chained,
                .label = NULL,
    };

    WGPUSurface surface = wgpuInstanceCreateSurface(wgpuInstance, 
                                                    &descriptor);
#endif
    assert_msg(surface, "Error creating surface");

    return surface;
}

int Engine::initialize(GLFWwindow *window) {

    if (xr_context.initialize()) {
        std::cout << "Could not initialize OpenXR context" << std::endl;
        return 1;
    }
    
    if (webgpu_context.initialize(&xr_context)) {
        std::cout << "Could not initialize WebGPU context" << std::endl;
        return 1;
    }
}

void Engine::clean() {
    //wgpuInstance.Release();
    //swapchain.Release();
    //device.Release();
    //adapter.Release();
}

void Engine::render_frame() {

    // --- Begin frame
    xr_context.initFrame();

    // Get the current texture in the swapchain
    {
        webgpu_context.current_texture_view = webgpu_context.images[1].textureView;
        assert_msg(webgpu_context.current_texture_view != NULL, "Error, dont resize the window please!!");
    }

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
            .view = webgpu_context.current_texture_view,
            .resolveTarget = nullptr, // for MS
            .loadOp = wgpu::LoadOp::Clear,
            .storeOp = wgpu::StoreOp::Store,
            .clearValue = {0.0f,0.0f,1.0f,1.0f}
        };
        wgpu::RenderPassDescriptor render_pass_descr = {
            .colorAttachmentCount = 1,
            .colorAttachments = &render_pass_color_attachment,
            .depthStencilAttachment = nullptr,
        };
        render_pass = webgpu_context.device_command_encoder.BeginRenderPass(&render_pass_descr);
        {
            // Bind Pipeline
            render_pass.SetPipeline(webgpu_context.render_pipeline);
            // Submit drawcall
            render_pass.Draw(3, 1, 0, 0);
        }
        render_pass.End();
        render_pass.Release();
    }

    //
    {
        wgpu::CommandBufferDescriptor cmd_buff_descriptor = {
            .nextInChain = NULL,
            .label = "Command buffer"
        };

        wgpu::CommandBuffer commander = webgpu_context.device_command_encoder.Finish(&cmd_buff_descriptor);
        webgpu_context.device_command_encoder.Release();
        webgpu_context.device_queue.Submit(1, &commander);

        commander.Release();
        //current_texture_view.Release();
    }

    // Submit frame
    {
        //wgpu::SwapChainPresent(swapchain);
    }

    xr_context.endFrame();
}
