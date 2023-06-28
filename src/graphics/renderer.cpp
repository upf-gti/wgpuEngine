#include "renderer.h"

#ifdef XR_SUPPORT
#include "dawnxr/dawnxr_internal.h"
#endif

Renderer::Renderer()
{
    Shader::webgpu_context = &webgpu_context;

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
