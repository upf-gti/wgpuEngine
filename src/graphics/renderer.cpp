#include "renderer.h"

#ifdef XR_SUPPORT
    #include "dawnxr/dawnxr_internal.h"

    #if defined(BACKEND_DX12)
    #include <dawn/native/D3D12Backend.h>
    #elif defined(BACKEND_VULKAN)
    #include "dawn/native/VulkanBackend.h"
    #endif
#endif

#include "graphics/mesh.h"
#include "graphics/texture.h"

#include <algorithm>
#include <iostream>

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

    WGPURequestAdapterOptions adapter_opts = {};

    // To choose dedicated GPU on laptops
    adapter_opts.powerPreference = WGPUPowerPreference_HighPerformance;

#ifdef XR_SUPPORT

    xr_context.z_far = z_near;
    xr_context.z_far = z_far;

#if defined(BACKEND_DX12)
    adapter_opts.backendType = WGPUBackendType_D3D12;
#elif defined(BACKEND_VULKAN)
    dawn::native::vulkan::RequestAdapterOptionsOpenXRConfig adapter_opts_xr_config = {};
    adapter_opts.backendType = WGPUBackendType_Vulkan;
#endif

    // Create internal vulkan instance
    if (is_openxr_available) {

#if defined(BACKEND_VULKAN)
        dawnxr::internal::createVulkanOpenXRConfig(xr_context.instance, xr_context.system_id, (void**)&adapter_opts_xr_config.openXRConfig);
        adapter_opts.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&adapter_opts_xr_config);
#endif

        create_screen_swapchain = use_mirror_screen;
    }

#else
    // Create internal vulkan instance
    //webgpu_context.instance->DiscoverDefaultAdapters();
#endif

    if (webgpu_context.initialize(adapter_opts, required_limits, window, create_screen_swapchain)) {
        spdlog::error("Could not initialize WebGPU context");
        return 1;
    }

#ifdef XR_SUPPORT
    if (is_openxr_available && xr_context.init(&webgpu_context)) {
        spdlog::error("Could not initialize OpenXR context");
        is_openxr_available = false;
    }

    if (is_openxr_available) {
        webgpu_context.render_width = xr_context.viewconfig_views[0].recommendedImageRectWidth;
        webgpu_context.render_height = xr_context.viewconfig_views[0].recommendedImageRectHeight;
    }
#endif

    if (!is_openxr_available) {
        webgpu_context.render_width = webgpu_context.screen_width;
        webgpu_context.render_height = webgpu_context.screen_height;
    }

    RendererStorage::register_basic_meshes();

    return 0;
}

void Renderer::clean()
{
#ifdef XR_SUPPORT
    xr_context.clean();
#endif

    Pipeline::clean_registered_pipelines();

    webgpu_context.destroy();
}

void Renderer::prepare_instancing()
{
    for (int i = 0; i < RENDER_LIST_SIZE; ++i) {

        instance_data[i].clear();
        instance_data[i].resize(render_list[i].size());

        // Sort render_list
        std::sort(render_list[i].begin(), render_list[i].end(), [](auto& lhs, auto& rhs) {
            auto& lhs_mat = lhs.entity_mesh->get_material();
            auto& rhs_mat = rhs.entity_mesh->get_material();

            auto* lhs_mesh = lhs.entity_mesh->get_mesh();
            auto* rhs_mesh = rhs.entity_mesh->get_mesh();

            if (lhs_mat.priority > rhs_mat.priority) return true;
            if (lhs_mat.priority == rhs_mat.priority && lhs_mat.shader > rhs_mat.shader) return true;
            if (lhs_mat.priority == rhs_mat.priority && lhs_mat.shader == rhs_mat.shader && lhs_mesh > rhs_mesh) return true;
            if (lhs_mat.priority == rhs_mat.priority && lhs_mat.shader == rhs_mat.shader && lhs_mesh == rhs_mesh && lhs_mat.diffuse > rhs_mat.diffuse) return true;

            return false;
            });

        // Check instances
        {
            Mesh* prev_mesh = nullptr;
            Shader* prev_shader = nullptr;
            Texture* prev_texture = nullptr;

            uint32_t repeats = 0;
            for (uint32_t j = 0; j < render_list[i].size(); ++j) {

                EntityMesh* entity_mesh = render_list[i][j].entity_mesh;
                Material& material = entity_mesh->get_material();

                // Repeated EntityMesh, must be instanced
                if (prev_mesh == entity_mesh->get_mesh() && prev_shader == material.shader && prev_texture == material.diffuse && !(material.flags & MATERIAL_UI)) {
                    repeats++;
                }
                else {
                    if (repeats > 0) {
                        for (uint32_t k = 1; k <= repeats; k++) {
                            render_list[i][j - k].repeat = k;
                        }
                    }
                    repeats = 1;
                }

                prev_mesh = entity_mesh->get_mesh();
                prev_shader = material.shader;
                prev_texture = material.diffuse;

                // Fill instance_data
                instance_data[i][j] = { entity_mesh->get_model(), material.color };
            }

            if (repeats > 0) {
                for (uint32_t k = 1; k <= repeats; k++) {
                    render_list[i][render_list[i].size() - k].repeat = k;
                }
            }
        }

        // Fill instance buffers
        uint32_t instances = static_cast<uint32_t>(instance_data[i].size());

        if (instances > instance_data_uniform[i].buffer_size / sizeof(sUniformData)) {

            //std::vector<sUniformData> default_data = { instances, { glm::mat4x4(1.0f), glm::vec4(1.0f) } };

            instance_data_uniform[i].data = webgpu_context.create_buffer(sizeof(sUniformData) * instances, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, instance_data[i].data(), "instance_mesh_buffer");
            instance_data_uniform[i].binding = 0;
            instance_data_uniform[i].buffer_size = sizeof(sUniformData) * instances;

            // Recreate bind groups
            std::vector<Uniform*> uniforms = { &instance_data_uniform[i] };
            Shader* prev_shader = nullptr;
            for (uint32_t j = 0; j < render_list[i].size(); ) {

                sRenderData& render_data = render_list[i][j];
                Shader* shader = render_data.entity_mesh->get_material().shader;

                if (bind_groups[i]) {
                    wgpuBindGroupRelease(bind_groups[i]);
                }

                bind_groups[i] = webgpu_context.create_bind_group(uniforms, shader, 0);

                j += render_data.repeat;
            }

        } else
        if (instances > 0) {
            webgpu_context.update_buffer(std::get<WGPUBuffer>(instance_data_uniform[i].data), 0, instance_data[i].data(), sizeof(sUniformData) * instances);
        }
    }
}

void Renderer::render(WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_bind_group_camera)
{

    Pipeline* prev_pipeline = nullptr;

    for (int i = 0; i < RENDER_LIST_SIZE; ++i) {

        for (int j = 0; j < render_list[i].size(); ) {

            sRenderData& render_data = render_list[i][j];
            EntityMesh* entity_mesh = render_data.entity_mesh;
            Mesh* mesh = entity_mesh->get_mesh();
            Material& material = entity_mesh->get_material();

            Pipeline* pipeline = material.shader->get_pipeline();
            if (pipeline != prev_pipeline) {
                pipeline->set(render_pass);
            }

            // Not initialized
            if (mesh->get_vertex_count() == 0) {
                std::cerr << "Skipping not initialized mesh" << std::endl;
                continue;
            }

            uint8_t bind_group_index = 0;

            // Set bind groups
            wgpuRenderPassEncoderSetBindGroup(render_pass, bind_group_index++, bind_groups[i], 0, nullptr);
            wgpuRenderPassEncoderSetBindGroup(render_pass, bind_group_index++, render_bind_group_camera, 0, nullptr);

            if (material.flags & MATERIAL_DIFFUSE) {
                wgpuRenderPassEncoderSetBindGroup(render_pass, bind_group_index++, renderer_storage.get_material_bind_group(material), 0, nullptr);
            }

            if (material.flags & MATERIAL_UI) {
                wgpuRenderPassEncoderSetBindGroup(render_pass, bind_group_index++, renderer_storage.get_ui_widget_bind_group(entity_mesh), 0, nullptr);
            }

            // Set vertex buffer while encoding the render pass
            wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, mesh->get_vertex_buffer(), 0, mesh->get_byte_size());

            // Submit drawcall
            wgpuRenderPassEncoderDraw(render_pass, mesh->get_vertex_count(), render_data.repeat, 0, j);

            prev_pipeline = pipeline;

            j += render_data.repeat;
        }
    }
}

void Renderer::add_renderable(EntityMesh* entity_mesh)
{
    const Material& material = entity_mesh->get_material();

    if (material.flags & MATERIAL_TRANSPARENT) {
        render_list[RENDER_LIST_ALPHA].push_back({ entity_mesh, 1 });
    } else
    if (material.flags & MATERIAL_UI) {
        render_list[RENDER_LIST_UI].push_back({ entity_mesh, 1 });
    }
    else {
        render_list[RENDER_LIST_OPAQUE].push_back({ entity_mesh, 1 });
    }
}

void Renderer::clear_renderables()
{
    for (int i = 0; i < RENDER_LIST_SIZE; ++i) {
        render_list[i].clear();
    }
}

void Renderer::resize_window(int width, int height)
{
    webgpu_context.create_swapchain(width, height);

    if (!is_openxr_available) {
        webgpu_context.render_width = webgpu_context.screen_width;
        webgpu_context.render_height = webgpu_context.screen_height;
    }
}
