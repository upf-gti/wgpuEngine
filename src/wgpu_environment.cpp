#include "wgpu_environment.h"
#include "webgpu.h"
#include "wgpu.h"

#include "raw_shaders.h"

#include <GLFW/glfw3.h>
#include <stdint.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windef.h>


WGPUSurface get_surface(GLFWwindow *window, WGPUInstance instance) {
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

    WGPUSurface surface = wgpuInstanceCreateSurface(instance, 
                                                    &descriptor);
#endif
    assert_msg(surface, "Error creating surface");

    return surface;
}

void WGPUEnv::sInstance::initialize(GLFWwindow *window, void *callback) {
    // Create the payload that stores the data during the async generation
    sPayload payload = {
        .wgpu_man_instance = this,
        .window = window, 
        .callback = callback
    };

    // Create instance
    {
        WGPUInstanceDescriptor instance_descr = {
            .nextInChain = NULL,
        };
        instance = wgpuCreateInstance(&instance_descr);

        assert_msg(instance, 
                   "Error creating WebGPU instance");
    }

    // Fetch the render surface
    {
        surface = get_surface(window, instance);
    }

    // Request adapter
    {
        WGPURequestAdapterOptions options = {
            .nextInChain = NULL,
            .compatibleSurface = surface
        };
        wgpuInstanceRequestAdapter(instance, 
                                   &options, 
                                   e_adapter_request_ended,
                                   (void *)&payload);
    }
}

void WGPUEnv::sInstance::_config_render_pipeline() {
    // Load the shader module https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/hello-triangle.html
    {
        WGPUShaderModuleWGSLDescriptor shader_code_desc = {
            .chain = {
                .next = NULL,
                .sType = WGPUSType_ShaderModuleWGSLDescriptor
            },
            .code = RAW_SHADERS::simple_shaders
        };
        WGPUShaderModuleDescriptor shader_descr = {
            .nextInChain = &shader_code_desc.chain,
            .hintCount = 0,
            .hints = NULL,
        };

        shader_module = wgpuDeviceCreateShaderModule(device, &shader_descr);
    }

    // Config the render target
    WGPUColorTargetState color_target;
    WGPUBlendState blend_state;
    {
        blend_state = {
            .color = {
                .operation = WGPUBlendOperation_Add,
                .srcFactor = WGPUBlendFactor_SrcAlpha,
                .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
            },
            .alpha = {
                .operation = WGPUBlendOperation_Add,
                .srcFactor = WGPUBlendFactor_Zero,
                .dstFactor = WGPUBlendFactor_One,
            }
        };

        color_target = {
            .format = swapchain_format,
            .blend = &blend_state,
            .writeMask = WGPUColorWriteMask_All
        };
    }

    // Layout descriptor (bind goups, buffers, uniforms)
    {
        WGPUPipelineLayoutDescriptor layout_descr = {
            .nextInChain = NULL,
            .bindGroupLayoutCount = 0,
            .bindGroupLayouts = NULL,
        };

        render_pipeline_layout = wgpuDeviceCreatePipelineLayout(device, &layout_descr);
    }

    // Config the render pipeline
    {   
        WGPUFragmentState fragment_state = {
            .module = shader_module,
            .entryPoint = "fs_main",
            .constantCount = 0,
            .constants = NULL,
            .targetCount = 1,
            .targets = &color_target
        };

        WGPURenderPipelineDescriptor pipeline_descr = {
            .nextInChain = NULL,
            .layout = render_pipeline_layout,
            .vertex = {
                .module = shader_module,
                .entryPoint = "vs_main",
                .constantCount = 0,
                .constants = NULL,
                .bufferCount = 0,
                .buffers = NULL,
                
            },
            .primitive = {
                .topology = WGPUPrimitiveTopology_TriangleList,
                .stripIndexFormat = WGPUIndexFormat_Undefined, // order of the connected vertices
                .frontFace = WGPUFrontFace_CCW,
                .cullMode = WGPUCullMode_None
            },
            .depthStencil = NULL,
            .multisample = {
                .count = 1,
                .mask = ~0u,
                .alphaToCoverageEnabled = false
            },
            .fragment = &fragment_state,
        };
        render_pipeline = wgpuDeviceCreateRenderPipeline(device, &pipeline_descr);
    }
}

void WGPUEnv::sInstance::clean() {
    wgpuInstanceRelease(instance);
    wgpuSwapChainDrop(swapchain);
    wgpuDeviceDrop(device);
    wgpuAdapterDrop(adapter);
}


void WGPUEnv::sInstance::render_frame() {
    // Get the current texture in the swapchain
    {
        current_texture_view = wgpuSwapChainGetCurrentTextureView(swapchain);
        assert_msg(current_texture_view != NULL, "Error, dont resize the window please!!");
    }

    // Create the command encoder
    {
        WGPUCommandEncoderDescriptor encoder_desc = {
            .nextInChain = NULL,
            .label = "Device command encoder"
        };
        device_command_encoder = wgpuDeviceCreateCommandEncoder(device, &encoder_desc);
    }

    // Create & fill the render pass (encoder)
    WGPURenderPassEncoder render_pass;
    {
        // Prepare the color attachment
        WGPURenderPassColorAttachment render_pass_color_attachment = {
            .view = current_texture_view,
            .resolveTarget = NULL, // for MS
            .loadOp = WGPULoadOp_Clear,
            .storeOp = WGPUStoreOp_Store,
            .clearValue = {0.0f,0.0f,1.0f,1.0f}
        };
        WGPURenderPassDescriptor render_pass_descr = {
            .nextInChain = NULL,
            .colorAttachmentCount = 1,
            .colorAttachments = &render_pass_color_attachment,
            .depthStencilAttachment = NULL,
            .timestampWriteCount = 0, // for measuing performance
            .timestampWrites = NULL
        };
        render_pass = wgpuCommandEncoderBeginRenderPass(device_command_encoder, &render_pass_descr);
        {
            // Bind Pipeline
            wgpuRenderPassEncoderSetPipeline(render_pass, render_pipeline);
            // Submit drawcall
            wgpuRenderPassEncoderDraw(render_pass, 3, 1, 0, 0);
        }
        wgpuRenderPassEncoderEnd(render_pass);

        wgpuTextureViewDrop(current_texture_view);
    }

    //
    {
        WGPUCommandBufferDescriptor cmd_buff_descriptor = {
            .nextInChain = NULL,
            .label = "Command buffer"
        };

        WGPUCommandBuffer commander = wgpuCommandEncoderFinish(device_command_encoder, &cmd_buff_descriptor);
        wgpuQueueSubmit(device_queue, 1, &commander);
    }

    // Submit frame
    {
        wgpuSwapChainPresent(swapchain);
    }
}


// Events =========================
void  WGPUEnv::sInstance::e_adapter_request_ended(WGPURequestAdapterStatus status, 
                                                  WGPUAdapter adapter, 
                                                  char const* message, 
                                                  void* user_data) {
    assert_msg(status == WGPURequestAdapterStatus_Success, "Error loading adapter");
    ((sPayload*) user_data)->wgpu_man_instance->adapter = adapter;
    
    // Inspect adapter features
    uint32_t function_count = wgpuAdapterEnumerateFeatures(adapter, NULL);

    WGPUFeatureName* features = (WGPUFeatureName*) malloc(sizeof(WGPUFeatureName) * function_count);

    wgpuAdapterEnumerateFeatures(adapter, features);
    
    std::cout << "Features" << std::endl;
    for(uint32_t i = 0; i < function_count; i++) {
        std::cout << features[i] << std::endl;
    }
    std::cout << "======" << std::endl;

    WGPUDeviceDescriptor descriptor = {
        .nextInChain = NULL,
        .label = "GPU",
        .requiredFeaturesCount = 0,
        .requiredLimits = NULL,
        .defaultQueue = {
            .nextInChain = NULL,
            .label = "Default queue"
        }
    };

    wgpuAdapterRequestDevice(adapter, &descriptor, e_device_request_ended, user_data);
}

void WGPUEnv::sInstance::e_device_request_ended(WGPURequestDeviceStatus status, 
                                                WGPUDevice device, 
                                                char const * message, 
                                                void *user_data) {
    //
    WGPUEnv::sInstance *instace = ((sPayload*) user_data)->wgpu_man_instance;
    assert_msg(status == WGPURequestDeviceStatus_Success, "Error loading the device");
    instace->device = device;

    // Set the error callback
    wgpuDeviceSetUncapturedErrorCallback(device, e_device_error, NULL);

    // Create the Device Queue
    {
        instace->device_queue = wgpuDeviceGetQueue(device);
    }

    // Create Swapchain
    {
        instace->swapchain_format = wgpuSurfaceGetPreferredFormat(instace->surface, instace->adapter);
        WGPUSwapChainDescriptor swapchain_descr = {
            .nextInChain = NULL,
            .usage = WGPUTextureUsage_RenderAttachment,
            .format = instace->swapchain_format,
            .width = 640,
            .height = 480,
            .presentMode = WGPUPresentMode_Fifo
        };

        instace->swapchain = wgpuDeviceCreateSwapChain(device, instace->surface, &swapchain_descr);
    }

    instace->is_initialized = true;
}

void WGPUEnv::sInstance::e_device_error(WGPUErrorType type, char const* message, void* user_data) {
    std::cout << "Error on Device: " << type;
    if (message) {
        std::cout << " - " << message;
    }

    std::cout  << std::endl;
}