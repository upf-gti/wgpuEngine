#include "renderer.h"

#include "graphics/webgpu_context.h"

#ifdef XR_SUPPORT

#include "xr/openxr_context.h"

#include "dawnxr/dawnxr_internal.h"

#if defined(BACKEND_DX12)
#include <dawn/native/D3D12Backend.h>
#elif defined(BACKEND_VULKAN)
#include "dawn/native/VulkanBackend.h"
#endif

#endif

#include "graphics/texture.h"
#include "framework/camera/camera.h"
#include "framework/nodes/light_3d.h"
#include "graphics/mesh_instance.h"
#include "graphics/pipeline.h"
#include "graphics/shader.h"
#include "graphics/debug/renderdoc_capture.h"
#include "graphics/renderer_storage.h"

#include <algorithm>

#include "glm/gtx/quaternion.hpp"

#include "spdlog/spdlog.h"

Renderer* Renderer::instance = nullptr;

Renderer::Renderer()
{
    instance = this;

    renderer_storage = new RendererStorage();

#ifdef _DEBUG
    RenderdocCapture::init();
#endif

    webgpu_context = new WebGPUContext();

    Pipeline::webgpu_context = webgpu_context;
    Surface::webgpu_context = webgpu_context;
    Texture::webgpu_context = webgpu_context;

#ifdef XR_SUPPORT
    xr_context = new OpenXRContext();
    is_openxr_available = xr_context->create_instance();
#endif
}

Renderer::~Renderer()
{
    delete webgpu_context;

#ifdef XR_SUPPORT
    delete xr_context;
#endif
}

int Renderer::initialize(GLFWwindow* window, bool use_mirror_screen)
{
    bool create_screen_swapchain = true;

    this->use_mirror_screen = use_mirror_screen;

    webgpu_context->create_instance();

    WGPURequestAdapterOptions adapter_opts = {};

    // To choose dedicated GPU on laptops
    adapter_opts.powerPreference = WGPUPowerPreference_HighPerformance;

#ifdef XR_SUPPORT

    xr_context->z_far = z_near;
    xr_context->z_far = z_far;

#if defined(BACKEND_DX12)
    adapter_opts.backendType = WGPUBackendType_D3D12;
#elif defined(BACKEND_VULKAN)
    dawn::native::vulkan::RequestAdapterOptionsOpenXRConfig adapter_opts_xr_config = {};
    adapter_opts.backendType = WGPUBackendType_Vulkan;
#endif

    // Create internal vulkan instance
    if (is_openxr_available) {

#if defined(BACKEND_VULKAN)
        dawnxr::internal::createVulkanOpenXRConfig(xr_context->instance, xr_context->system_id, (void**)&adapter_opts_xr_config.openXRConfig);
        adapter_opts.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&adapter_opts_xr_config);
#endif

        create_screen_swapchain = use_mirror_screen;
    }
    else {
        spdlog::warn("XR not available, fallback to desktop mode");
    }

#else
    // Create internal vulkan instance
    //webgpu_context->instance->DiscoverDefaultAdapters();
#endif

    if (webgpu_context->initialize(adapter_opts, required_limits, window, create_screen_swapchain)) {
        spdlog::error("Could not initialize WebGPU context");
        return 1;
    }

#ifdef XR_SUPPORT
    if (is_openxr_available && !xr_context->init(webgpu_context)) {
        spdlog::error("Could not initialize OpenXR context");
        is_openxr_available = false;
    }

    if (is_openxr_available) {
        webgpu_context->render_width = xr_context->viewconfig_views[0].recommendedImageRectWidth;
        webgpu_context->render_height = xr_context->viewconfig_views[0].recommendedImageRectHeight;
    }
#endif

    if (!is_openxr_available) {
        webgpu_context->render_width = webgpu_context->screen_width;
        webgpu_context->render_height = webgpu_context->screen_height;
    }

    RendererStorage::register_basic_surfaces();

    if (!irradiance_texture) {
        irradiance_texture = RendererStorage::get_texture("data/textures/environments/sky.hdr");
    }

    Shader::set_custom_define("MAX_LIGHTS", MAX_LIGHTS);

    eye_depth_textures = new Texture[EYE_COUNT];
    multisample_textures = new Texture[EYE_COUNT];

#ifndef __EMSCRIPTEN__
    renderdoc_capture = new RenderdocCapture();
#endif


    init_depth_buffers();

    init_lighting_bind_group();

    init_multisample_textures();

    return 0;
}

void Renderer::clean()
{
#ifdef XR_SUPPORT
    xr_context->clean();
#endif

    uint8_t num_textures = is_openxr_available ? 2 : 1;
    for (int i = 0; i < num_textures; ++i)
    {
        wgpuTextureViewRelease(eye_depth_texture_view[i]);
    }

    RendererStorage::clean_registered_pipelines();

    webgpu_context->destroy();

    delete renderer_storage;
    delete[] eye_depth_textures;
    delete[] multisample_textures;

    delete renderdoc_capture;
}

void Renderer::init_lighting_bind_group()
{
    // delete if already created
    if (std::holds_alternative<WGPUTextureView>(irradiance_texture_uniform.data)) {
        wgpuTextureViewRelease(std::get<WGPUTextureView>(irradiance_texture_uniform.data));
        wgpuSamplerRelease(std::get<WGPUSampler>(ibl_sampler_uniform.data));
        wgpuBindGroupRelease(lighting_bind_group);
    }
    else {
        // only created once
        brdf_lut_uniform.data = webgpu_context->brdf_lut_texture->get_view();
        brdf_lut_uniform.binding = 1;
    }

    if (irradiance_texture) {

        irradiance_texture_uniform.data = irradiance_texture->get_view(WGPUTextureViewDimension_Cube, 0, 6, 0, 6);
        irradiance_texture_uniform.binding = 0;

        ibl_sampler_uniform.data = webgpu_context->create_sampler(
            WGPUAddressMode_ClampToEdge,
            WGPUAddressMode_ClampToEdge,
            WGPUAddressMode_ClampToEdge,
            WGPUFilterMode_Linear,
            WGPUFilterMode_Linear,
            WGPUMipmapFilterMode_Linear,
            static_cast<float>(irradiance_texture->get_mipmap_count())
        );

        ibl_sampler_uniform.binding = 2;
    }

    if (std::holds_alternative<WGPUBuffer>(lights_buffer.data)) {
        wgpuBufferDestroy(std::get<WGPUBuffer>(lights_buffer.data));
        lights_buffer.data = {};
    }

    lights_buffer.data = webgpu_context->create_buffer(sizeof(sLightUniformData) * MAX_LIGHTS, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &lights_uniform_data[0], "lights_buffer");
    lights_buffer.binding = 3;
    lights_buffer.buffer_size = sizeof(sLightUniformData) * MAX_LIGHTS;

    num_lights_buffer.data = webgpu_context->create_buffer(sizeof(int), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &num_lights, "num_lights_buffer");
    num_lights_buffer.binding = 4;
    num_lights_buffer.buffer_size = sizeof(int);

    std::vector<Uniform*> uniforms = { &irradiance_texture_uniform, &brdf_lut_uniform, &ibl_sampler_uniform, &lights_buffer, &num_lights_buffer };
    lighting_bind_group = webgpu_context->create_bind_group(uniforms, RendererStorage::get_shader("data/shaders/mesh_pbr.wgsl"), 3);
}

void Renderer::init_depth_buffers()
{
    uint8_t num_textures = is_openxr_available ? 2 : 1;
    for (int i = 0; i < num_textures; ++i)
    {
        eye_depth_textures[i].create(
            WGPUTextureDimension_2D,
            WGPUTextureFormat_Depth32Float,
            { webgpu_context->render_width, webgpu_context->render_height, 1 },
            WGPUTextureUsage_RenderAttachment,
            1, msaa_count, nullptr);

        if (eye_depth_texture_view[i]) {
            wgpuTextureViewRelease(eye_depth_texture_view[i]);
        }

        // Generate Texture views of depth buffers
        eye_depth_texture_view[i] = eye_depth_textures[i].get_view();
    }
}

void Renderer::init_multisample_textures()
{
    WGPUTextureFormat swapchain_format = is_openxr_available ? webgpu_context->xr_swapchain_format : webgpu_context->swapchain_format;

    uint8_t num_textures = is_openxr_available ? 2 : 1;
    for (int i = 0; i < num_textures; ++i) {
        multisample_textures[i].create(
            WGPUTextureDimension_2D,
            swapchain_format,
            { webgpu_context->render_width, webgpu_context->render_height, 1 },
            WGPUTextureUsage_RenderAttachment,
            1, msaa_count, nullptr);

        if (multisample_textures_views[i]) {
            wgpuTextureViewRelease(multisample_textures_views[i]);
        }

        multisample_textures_views[i] = multisample_textures[i].get_view();
    }
}

void Renderer::set_msaa_count(uint8_t msaa_count)
{
    bool recreate = msaa_count != this->msaa_count && multisample_textures[0].get_texture() != nullptr;

    this->msaa_count = msaa_count;

    if (recreate) {
        init_depth_buffers();
        init_multisample_textures();
    }

    RendererStorage::reload_all_render_pipelines();
}

uint8_t Renderer::get_msaa_count()
{
    return msaa_count;
}

void Renderer::prepare_instancing()
{
    update_lights();

    // Get all surfaces from entity meshes
    for (auto render_list_data : render_entity_list)
    {
        MeshInstance* mesh_instance = render_list_data.mesh_instance;
        glm::mat4x4 global_matrix = render_list_data.global_matrix;

        const std::vector<Surface*>& surfaces = mesh_instance->get_surfaces();

        glm::mat4x4 rotation_matrix = glm::toMat4(glm::quat_cast(global_matrix));

        for (Surface* surface : surfaces) {

            Material* material_override = mesh_instance->get_surface_material_override(surface);

            Material& material = material_override ? *material_override : surface->get_material();

            if (!material.shader) {
                continue;
            }

            RendererStorage::instance->register_material(webgpu_context, mesh_instance, material);

            RendererStorage::register_render_pipeline(material);

            eRenderListType list = RENDER_LIST_OPAQUE;

            if (material.flags & MATERIAL_2D) {
                list = material.transparency_type == ALPHA_BLEND ? RENDER_LIST_2D_TRANSPARENT : RENDER_LIST_2D;
            }
            else if (material.transparency_type == ALPHA_BLEND) {
                list = RENDER_LIST_TRANSPARENT;
            }

            render_list[list].push_back({ surface, 1, global_matrix, rotation_matrix, mesh_instance });
        }
    }

    for (int i = 0; i < RENDER_LIST_SIZE; ++i) {

        instance_data[i].clear();
        instance_data[i].resize(render_list[i].size());

        // Sort render_list
        std::sort(render_list[i].begin(), render_list[i].end(), [](auto& lhs, auto& rhs) {

            Material* lhs_ov_mat = lhs.mesh_instance_ref->get_surface_material_override(lhs.surface);
            Material* rhs_ov_mat = rhs.mesh_instance_ref->get_surface_material_override(rhs.surface);

            const Material& lhs_mat = lhs_ov_mat ? *lhs_ov_mat : lhs.surface->get_material();
            const Material& rhs_mat = rhs_ov_mat ? *rhs_ov_mat : rhs.surface->get_material();

            bool equal_priority = lhs_mat.priority == rhs_mat.priority;
            bool equal_shader = lhs_mat.shader == rhs_mat.shader;
            bool equal_diffuse = lhs_mat.diffuse_texture == rhs_mat.diffuse_texture;
            bool equal_normal = lhs_mat.normal_texture == rhs_mat.normal_texture;
            bool equal_metallic_rougness = lhs_mat.metallic_roughness_texture == rhs_mat.metallic_roughness_texture;

            if (lhs_mat.priority > rhs_mat.priority) return true;
            if (equal_priority && lhs_mat.shader > rhs_mat.shader) return true;
            if (equal_priority && equal_shader && lhs_mat.diffuse_texture > rhs_mat.diffuse_texture) return true;
            if (equal_priority && equal_shader && equal_diffuse && lhs_mat.normal_texture > rhs_mat.normal_texture) return true;
            if (equal_priority && equal_shader && equal_diffuse && equal_normal && lhs_mat.metallic_roughness_texture > rhs_mat.metallic_roughness_texture) return true;
            if (equal_priority && equal_shader && equal_diffuse && equal_normal && equal_metallic_rougness && lhs_mat.emissive_texture > rhs_mat.emissive_texture) return true;

            return false;
            });

        // Check instances
        {
            const Surface* prev_surface = nullptr;
            Shader* prev_shader = nullptr;
            glm::vec4 prev_color = {};
            Texture* prev_diffuse = nullptr;
            Texture* prev_normal = nullptr;
            Texture* prev_metallic_roughness = nullptr;
            Texture* prev_emissive = nullptr;

            uint32_t repeats = 0;
            for (uint32_t j = 0; j < render_list[i].size(); ++j) {

                const sRenderData& render_data = render_list[i][j];

                Material* material_override = render_data.mesh_instance_ref->get_surface_material_override(render_data.surface);

                const Material& material = material_override ? *material_override : render_data.surface->get_material();

                // Repeated MeshInstance3D, must be instanced
                if (prev_surface == render_data.surface && prev_shader == material.shader &&
                    prev_color == material.color &&
                    prev_diffuse == material.diffuse_texture &&
                    prev_normal == material.normal_texture &&
                    prev_metallic_roughness == material.metallic_roughness_texture &&
                    prev_emissive == material.emissive_texture &&
                    !(material.flags & MATERIAL_2D)) {
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

                prev_surface = render_data.surface;
                prev_shader = material.shader;
                prev_color = material.color;
                prev_diffuse = material.diffuse_texture;
                prev_normal = material.normal_texture;
                prev_metallic_roughness = material.metallic_roughness_texture;
                prev_emissive = material.emissive_texture;

                // Fill instance_data
                instance_data[i][j] = { render_data.global_matrix };
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

            instance_data_uniform[i].data = webgpu_context->create_buffer(sizeof(sUniformData) * instances, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, instance_data[i].data(), "instance_mesh_buffer");
            instance_data_uniform[i].binding = 0;
            instance_data_uniform[i].buffer_size = sizeof(sUniformData) * instances;

            // Recreate bind groups
            std::vector<Uniform*> uniforms = { &instance_data_uniform[i] };
            Shader* prev_shader = nullptr;
            for (uint32_t j = 0; j < render_list[i].size(); ) {

                const sRenderData& render_data = render_list[i][j];

                Material* material_override = render_data.mesh_instance_ref->get_surface_material_override(render_data.surface);

                Shader* shader = material_override ? material_override->shader : render_data.surface->get_material().shader;

                if (bind_groups[i]) {
                    wgpuBindGroupRelease(bind_groups[i]);
                }

                bind_groups[i] = webgpu_context->create_bind_group(uniforms, shader, 0);

                j += render_data.repeat;
            }

        }
        else
            if (instances > 0) {
                webgpu_context->update_buffer(std::get<WGPUBuffer>(instance_data_uniform[i].data), 0, instance_data[i].data(), sizeof(sUniformData) * instances);
            }
    }
}

void Renderer::render_render_list(int list_index, WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_bind_group_camera)
{
    Pipeline* prev_pipeline = nullptr;

    for (int i = 0; i < render_list[list_index].size(); ) {

        const sRenderData& render_data = render_list[list_index][i];

        Material* material_override = render_data.mesh_instance_ref->get_surface_material_override(render_data.surface);

        const Material& material = material_override ? *material_override : render_data.surface->get_material();

        Pipeline* pipeline = material.shader->get_pipeline();

        assert(pipeline);        

        if (pipeline != prev_pipeline) {
            pipeline->set(render_pass);
        }

        // Not initialized
        if (render_data.surface->get_vertex_count() == 0) {
            spdlog::error("Skipping not initialized mesh");
            continue;
        }

        uint8_t bind_group_index = 0;

        // Set bind groups
        wgpuRenderPassEncoderSetBindGroup(render_pass, bind_group_index++, bind_groups[list_index], 0, nullptr);
        wgpuRenderPassEncoderSetBindGroup(render_pass, bind_group_index++, render_bind_group_camera, 0, nullptr);

        wgpuRenderPassEncoderSetBindGroup(render_pass, bind_group_index++, renderer_storage->get_material_bind_group(material), 0, nullptr);

//#ifndef NDEBUG
//        wgpuRenderPassEncoderPushDebugGroup(render_pass, render_data.surface->get_name().c_str());
//#endif

        if (material.flags & MATERIAL_UI) {
            WGPUBindGroup ui_bind_group = renderer_storage->get_ui_widget_bind_group(render_data.mesh_instance_ref);
            if (ui_bind_group) {
                wgpuRenderPassEncoderSetBindGroup(render_pass, bind_group_index++, ui_bind_group, 0, nullptr);
            }
        }

        if (material.flags & MATERIAL_PBR) {
            wgpuRenderPassEncoderSetBindGroup(render_pass, 3, lighting_bind_group, 0, nullptr);
        }

        // Set vertex buffer while encoding the render pass
        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, render_data.surface->get_vertex_buffer(), 0, render_data.surface->get_byte_size());

        // Submit drawcall
        wgpuRenderPassEncoderDraw(render_pass, render_data.surface->get_vertex_count(), render_data.repeat, 0, i);

//#ifndef NDEBUG
//        wgpuRenderPassEncoderPopDebugGroup(render_pass);
//#endif

        prev_pipeline = pipeline;

        i += render_data.repeat;
    }
}

void Renderer::render_opaque(WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_bind_group_camera)
{
#ifndef NDEBUG
    wgpuRenderPassEncoderPushDebugGroup(render_pass, "Opaque");
#endif

    render_render_list(RENDER_LIST_OPAQUE, render_pass, render_bind_group_camera);

#ifndef NDEBUG
    wgpuRenderPassEncoderPopDebugGroup(render_pass);
#endif
}

void Renderer::render_transparent(WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_bind_group_camera)
{
#ifndef NDEBUG
    wgpuRenderPassEncoderPushDebugGroup(render_pass, "Transparent");
#endif

    render_render_list(RENDER_LIST_TRANSPARENT, render_pass, render_bind_group_camera);

#ifndef NDEBUG
    wgpuRenderPassEncoderPopDebugGroup(render_pass);
#endif
}

void Renderer::render_2D(WGPURenderPassEncoder render_pass, const WGPUBindGroup& render_bind_group_camera)
{
#ifndef NDEBUG
    wgpuRenderPassEncoderPushDebugGroup(render_pass, "2D");
#endif

    render_render_list(RENDER_LIST_2D, render_pass, render_bind_group_camera);

    render_render_list(RENDER_LIST_2D_TRANSPARENT, render_pass, render_bind_group_camera);

#ifndef NDEBUG
    wgpuRenderPassEncoderPopDebugGroup(render_pass);
#endif
}

void Renderer::add_renderable(MeshInstance* mesh_instance, glm::mat4x4 global_matrix)
{
    render_entity_list.push_back({ mesh_instance, global_matrix });
}

void Renderer::clear_renderables()
{
    render_entity_list.clear();

    for (int i = 0; i < RENDER_LIST_SIZE; ++i) {
        render_list[i].clear();
    }

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        lights_uniform_data[i] = {};
    }

    num_lights = 0;
}

void Renderer::update_lights()
{
    // update uniform data

    uint64_t buffer_size = sizeof(sLightUniformData) * num_lights;

    webgpu_context->update_buffer(std::get<WGPUBuffer>(lights_buffer.data), 0, &lights_uniform_data[0], buffer_size);

    webgpu_context->update_buffer(std::get<WGPUBuffer>(num_lights_buffer.data), 0, &num_lights, sizeof(int));
}

void Renderer::add_light(Light3D* new_light)
{
    lights_uniform_data[num_lights] = new_light->get_uniform_data();
    num_lights++;
}

void Renderer::resize_window(int width, int height)
{
    webgpu_context->create_swapchain(width, height);

    if (!is_openxr_available) {
        webgpu_context->render_width = webgpu_context->screen_width;
        webgpu_context->render_height = webgpu_context->screen_height;

        if (camera) {
            camera->set_perspective(glm::radians(45.0f), webgpu_context->render_width / static_cast<float>(webgpu_context->render_height), z_near, z_far);
        }
    }

    init_depth_buffers();
    init_multisample_textures();
}

void Renderer::set_irradiance_texture(Texture* texture)
{
    irradiance_texture = texture;

    init_lighting_bind_group();
}

glm::vec3 Renderer::get_camera_eye()
{
#if defined(XR_SUPPORT)
    if (is_openxr_available) {
        return xr_context->per_view_data[0].position; // return left eye
    }
#endif

    return camera->get_eye();
}

#ifdef XR_SUPPORT
OpenXRContext* Renderer::get_openxr_context()
{
    return (is_openxr_available ? xr_context : nullptr);
}
#endif

WebGPUContext* Renderer::get_webgpu_context()
{
    return webgpu_context;
}

GLFWwindow* Renderer::get_glfw_window()
{
    return webgpu_context->window;
};

