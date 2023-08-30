#include "renderer.h"

#ifdef XR_SUPPORT
#include "dawnxr/dawnxr_internal.h"
#include "dawn/native/VulkanBackend.h"
#endif

#include "graphics/mesh.h"
#include "graphics/texture.h"

Renderer* Renderer::instance = nullptr;

Renderer::Renderer()
{
    instance = this;

    Shader::webgpu_context = &webgpu_context;
    Pipeline::webgpu_context = &webgpu_context;
    Mesh::webgpu_context = &webgpu_context;
    Texture::webgpu_context = &webgpu_context;

#ifdef XR_SUPPORT
    is_openxr_available = xr_context.create_instance();
#endif
}

int Renderer::initialize(GLFWwindow* window, bool use_mirror_screen)
{
    bool create_screen_swapchain = true;

    this->use_mirror_screen = use_mirror_screen;

    webgpu_context.create_instance();

#ifdef XR_SUPPORT

    WGPURequestAdapterOptions adapter_opts = {};

    // To choose dedicated GPU on laptops
    adapter_opts.powerPreference = WGPUPowerPreference_HighPerformance;

    dawn::native::vulkan::RequestAdapterOptionsOpenXRConfig adapter_opts_xr_config = {};

    // Create internal vulkan instance
    if (is_openxr_available) {

        adapter_opts.backendType = WGPUBackendType_Vulkan;

        dawn::native::AdapterDiscoveryOptionsBase** options = new dawn::native::AdapterDiscoveryOptionsBase * ();
        dawnxr::internal::createVulkanOpenXRConfig(xr_context.instance, xr_context.system_id, (void**)&adapter_opts_xr_config.openXRConfig);

        adapter_opts.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&adapter_opts_xr_config);

        create_screen_swapchain = use_mirror_screen;
    }

#else
    // Create internal vulkan instance
    //webgpu_context.instance->DiscoverDefaultAdapters();
#endif

    if (webgpu_context.initialize(adapter_opts, required_limits, window, create_screen_swapchain)) {
        std::cout << "Could not initialize WebGPU context" << std::endl;
        return 1;
    }

    render_width = webgpu_context.render_width;
    render_height = webgpu_context.screen_height;

#ifdef XR_SUPPORT
    if (is_openxr_available && xr_context.init(&webgpu_context)) {
        std::cout << "Could not initialize OpenXR context" << std::endl;
        return 1;
    }

    if (is_openxr_available) {
        render_width = xr_context.viewconfig_views[0].recommendedImageRectWidth;
        render_height = xr_context.viewconfig_views[0].recommendedImageRectHeight;
    }
#endif

    return 0;
}

void Renderer::clean()
{
#ifdef XR_SUPPORT
    xr_context.clean();
#endif

    webgpu_context.destroy();
}
