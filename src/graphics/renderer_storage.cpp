#include "renderer_storage.h"

#include "surface.h"
#include "texture.h"
#include "shader.h"
#include "uniform.h"
#include "pipeline.h"
#include "renderer.h"
#include "mesh_instance.h"

#include "framework/nodes/mesh_instance_3d.h"
#include "framework/nodes/skeleton_instance_3d.h"
#include "framework/animation/animation.h"

#include <filesystem>

RendererStorage* RendererStorage::instance = nullptr;

std::map<std::string, Surface*> RendererStorage::surfaces;
std::map<std::string, Texture*> RendererStorage::textures;
std::map<std::string, Shader*> RendererStorage::shaders;
std::map<std::string, const char*> RendererStorage::engine_shaders_refs;
std::map<std::string, Animation*> RendererStorage::animations;

Texture* RendererStorage::current_skybox_texture = nullptr;
std::map<std::string, std::vector<std::string>> RendererStorage::shader_library_references;
std::unordered_map<const Material*, RendererStorage::sBindingData> RendererStorage::material_bind_groups;
std::unordered_map<const void*, RendererStorage::sBindingData> RendererStorage::ui_widget_bind_groups;

std::unordered_map<RenderPipelineKey, Pipeline*> RendererStorage::registered_render_pipelines;
std::unordered_map<Shader*, Pipeline*> RendererStorage::registered_compute_pipelines;

RendererStorage::RendererStorage()
{
    instance = this;
}

void RendererStorage::register_material_bind_group(WebGPUContext* webgpu_context, MeshInstance* mesh_instance, const Material* material)
{
    if (material_bind_groups.contains(material)) {
        return;
    }

    bool uses_textures = false;
    uint32_t binding = 0;

    std::vector<Uniform*>& uniforms = material_bind_groups[material].uniforms;
    Texture* texture_ref = nullptr;

    if (material->diffuse_texture) {
        Uniform* u = new Uniform();
        uint32_t array_layers = material->diffuse_texture->get_array_layers();
        WGPUTextureViewDimension view_dimension = array_layers > 1 ? WGPUTextureViewDimension_Cube : WGPUTextureViewDimension_2D;
        if (material->diffuse_texture->get_dimension() == WGPUTextureDimension_3D) {
            view_dimension = WGPUTextureViewDimension_3D;
            array_layers = 1;
        }
        u->data = material->diffuse_texture->get_view(view_dimension, 0, material->diffuse_texture->get_mipmap_count(), 0, array_layers);
        u->binding = 0;
        uniforms.push_back(u);
        uses_textures |= true;
        texture_ref = material->diffuse_texture;
    }

    {
        Uniform* u = new Uniform();
        u->data = webgpu_context->create_buffer(sizeof(glm::vec4), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &material->color, "mat_albedo");
        u->binding = 1;
        u->buffer_size = sizeof(glm::vec4);
        uniforms.push_back(u);
    }

    if (material->metallic_roughness_texture) {
        Uniform* u = new Uniform();
        u->data = material->metallic_roughness_texture->get_view(WGPUTextureViewDimension_2D, 0, material->metallic_roughness_texture->get_mipmap_count());
        u->binding = 2;
        uniforms.push_back(u);
        uses_textures |= true;
        texture_ref = material->metallic_roughness_texture;
    }

    if (material->type == MATERIAL_PBR) {
        Uniform* u = new Uniform();
        glm::vec3 occlusion_roughness_metallic = { material->occlusion, material->roughness, material->metalness };
        u->data = webgpu_context->create_buffer(sizeof(glm::vec3), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &occlusion_roughness_metallic, "mat_occlusion_roughness_metallic");
        u->binding = 3;
        u->buffer_size = sizeof(glm::vec3);
        uniforms.push_back(u);
    }

    if (material->normal_texture) {
        Uniform* u = new Uniform();
        u->data = material->normal_texture->get_view(WGPUTextureViewDimension_2D, 0, material->normal_texture->get_mipmap_count());
        u->binding = 4;
        uniforms.push_back(u);
        uses_textures |= true;
        texture_ref = material->normal_texture;
    }

    if (material->emissive_texture) {
        Uniform* u = new Uniform();
        u->data = material->emissive_texture->get_view();
        u->binding = 5;
        uniforms.push_back(u);
        uses_textures |= true;
        texture_ref = material->emissive_texture;
    }

    if (material->oclussion_texture) {
        Uniform* u = new Uniform();
        u->data = material->oclussion_texture->get_view();
        u->binding = 9;
        uniforms.push_back(u);
        uses_textures |= true;
        texture_ref = material->oclussion_texture;
    }

    if (material->type == MATERIAL_PBR) {
        Uniform* u = new Uniform();
        u->data = webgpu_context->create_buffer(sizeof(glm::vec4), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &material->emissive, "mat_emissive");
        u->binding = 6;
        u->buffer_size = sizeof(glm::vec4);
        uniforms.push_back(u);
    }

    // Add a sampler for basic 2d textures if there's any texture as uniforms
    if (uses_textures)
    {
        Uniform* sampler_uniform = new Uniform();
        sampler_uniform->data = webgpu_context->create_sampler(
            texture_ref->get_wrap_u(),
            texture_ref->get_wrap_v(),
            WGPUAddressMode_ClampToEdge,
            WGPUFilterMode_Linear,
            WGPUFilterMode_Linear,
            WGPUMipmapFilterMode_Linear,
            static_cast<float>(texture_ref->get_mipmap_count()),
            4
        );
        sampler_uniform->binding = 7;
        uniforms.push_back(sampler_uniform);
    }

    if (material->transparency_type == ALPHA_MASK) {
        Uniform* u = new Uniform();
        u->data = webgpu_context->create_buffer(sizeof(float), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &material->alpha_mask, "mat_alpha_cutoff");
        u->binding = 8;
        u->buffer_size = sizeof(float);
        uniforms.push_back(u);
    }

    if (material->use_skinning) {

        MeshInstance3D* instance_3d = static_cast<MeshInstance3D*>(mesh_instance);
        SkeletonInstance3D* skeleton_instance = dynamic_cast<SkeletonInstance3D*>(instance_3d->get_parent());
        assert(skeleton_instance);

        if (!skeleton_instance->get_animated_uniform_data()) {
            Uniform* anim_u = new Uniform();

            // Send current animated bones matrices
            const std::vector<glm::mat4x4>& animated_matrices = skeleton_instance->get_animated_data();

            anim_u->data = webgpu_context->create_buffer(sizeof(glm::mat4x4) * animated_matrices.size(), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, animated_matrices.data(), "animated_buffer");
            anim_u->binding = 10;
            anim_u->buffer_size = sizeof(glm::mat4x4) * animated_matrices.size();

            uniforms.push_back(anim_u);

            Uniform* invbind_u = new Uniform();

            // Send bind bones inverse matrices
            const std::vector<glm::mat4x4>& invbind_matrices = skeleton_instance->get_invbind_data();
            invbind_u->data = webgpu_context->create_buffer(sizeof(glm::mat4x4) * invbind_matrices.size(), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, invbind_matrices.data(), "invbind_buffer");
            invbind_u->binding = 11;
            invbind_u->buffer_size = sizeof(glm::mat4x4) * invbind_matrices.size();

            uniforms.push_back(invbind_u);

            skeleton_instance->set_uniform_data(anim_u, invbind_u);
        }
        else {
            uniforms.push_back(skeleton_instance->get_animated_uniform_data());
            uniforms.push_back(skeleton_instance->get_invbind_uniform_data());
        }
    }

    material_bind_groups[material].bind_group = webgpu_context->create_bind_group(uniforms, material->shader, 2);
}

WGPUBindGroup RendererStorage::get_material_bind_group(const Material* material)
{
    if (!material_bind_groups.contains(material)) {
        assert(false);
    }

    return material_bind_groups[material].bind_group;
}

void RendererStorage::register_ui_widget(WebGPUContext* webgpu_context, Shader* shader, void* entity_mesh, const sUIData& ui_data, uint8_t bind_group_id)
{
    if (ui_widget_bind_groups.contains(entity_mesh)) {
        assert(false);
        return;
    }

    Uniform* data_uniform = new Uniform();
    data_uniform->data = webgpu_context->create_buffer(sizeof(sUIData), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &ui_data, "ui_buffer");
    data_uniform->binding = 0;
    data_uniform->buffer_size = sizeof(sUIData);

    std::vector<Uniform*>& uniforms = ui_widget_bind_groups[entity_mesh].uniforms;

    uniforms.push_back(data_uniform);

    ui_widget_bind_groups[entity_mesh].bind_group = webgpu_context->create_bind_group(uniforms, shader, bind_group_id);
}

WGPUBindGroup RendererStorage::get_ui_widget_bind_group(const void* widget)
{
    if (!ui_widget_bind_groups.contains(widget)) {
        return nullptr;
    }

    return ui_widget_bind_groups[widget].bind_group;
}

void RendererStorage::update_ui_widget(WebGPUContext* webgpu_context, void* widget, const sUIData& ui_data)
{
    if (!ui_widget_bind_groups.contains(widget)) {
        assert(false);
        return;
    }

    Uniform* data_uniform = ui_widget_bind_groups[widget].uniforms[0];
    webgpu_context->update_buffer(std::get<WGPUBuffer>(data_uniform->data), 0, &ui_data, sizeof(sUIData));
}

Shader* RendererStorage::get_shader(const std::string& shader_path, const Material* material,
    const std::vector<std::string> &custom_define_specializations)
{
    std::vector<std::string> define_specializations = get_common_define_specializations(material);

    // concatenate
    define_specializations.insert(define_specializations.end(), custom_define_specializations.begin(), custom_define_specializations.end());

    return get_shader(shader_path, define_specializations);
}

Shader* RendererStorage::get_shader(const std::string& shader_path, const std::vector<std::string>& custom_define_specializations)
{
    std::string name = std::filesystem::relative(std::filesystem::path(shader_path)).string();

    std::string specialized_name = name;
    for (const std::string& specialization : custom_define_specializations) {
        specialized_name += "_" + specialization;
    }

    // check if already loaded
    std::map<std::string, Shader*>::iterator it = shaders.find(specialized_name);
    if (it != shaders.end())
        return it->second;

    Shader* sh = new Shader();

    if (!sh->load_from_file(name, specialized_name, custom_define_specializations)) {
        return nullptr;
    }

    // register in map
    shaders[specialized_name] = sh;

    return sh;
}

Shader* RendererStorage::get_shader_from_source(const char* source, const std::string& name, const Material* material, const std::vector<std::string>& custom_define_specializations)
{
    std::vector<std::string> define_specializations = get_common_define_specializations(material);

    // concatenate
    define_specializations.insert(define_specializations.end(), custom_define_specializations.begin(), custom_define_specializations.end());

    return get_shader_from_source(source, name, define_specializations);
}

Shader* RendererStorage::get_shader_from_source(const char* source, const std::string& name, const std::vector<std::string>& custom_define_specializations)
{
    std::string specialized_name = name;
    for (const std::string& specialization : custom_define_specializations) {
        specialized_name += "_" + specialization;
    }

    // check if already loaded
    std::map<std::string, Shader*>::iterator it = shaders.find(specialized_name);
    if (it != shaders.end())
        return it->second;

    Shader* sh = new Shader();

    if (!sh->load_from_source(source, name, specialized_name, custom_define_specializations)) {
        return nullptr;
    }

    // register in map
    shaders[specialized_name] = sh;
    engine_shaders_refs[name] = source;

    return sh;
}

void RendererStorage::reload_shader(const std::string& shader_path)
{
    std::string name = shader_path;

    // Check if already loaded
    for (auto& [shader_name, shader] : shaders) {
        if (shader_name.find(shader_path) != std::string::npos) {
            shader->reload();
        }
    }

    // If it is not a shader, check if it is a library
    auto it1 = shader_library_references.find(shader_path);
    if (it1 != shader_library_references.end())
    {
        for (auto& shader_name : shader_library_references[shader_path]) {
            if (shaders.contains(shader_name)) {
                Shader* shader = shaders[shader_name];
                shader->reload();
            }
        }
    }
}

Texture* RendererStorage::get_texture(const std::string& texture_path, bool is_srgb)
{
    std::string name = texture_path;

    // check if already loaded
    std::map<std::string, Texture*>::iterator it = textures.find(texture_path);
    if (it != textures.end())
        return it->second;

    Texture* tx = new Texture();

    std::string extension = texture_path.substr(texture_path.find_last_of(".") + 1);

    if (extension == "hdr")
    {
        Texture* hdr_texture = new Texture();
        hdr_texture->load_hdr(texture_path);

        tx->create(WGPUTextureDimension_2D, WGPUTextureFormat_RGBA32Float, { ENVIRONMENT_RESOLUTION, ENVIRONMENT_RESOLUTION, 6 },
            static_cast<WGPUTextureUsage>(WGPUTextureUsage_StorageBinding | WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst), 6, 1, nullptr);

        Renderer::instance->get_webgpu_context()->generate_prefiltered_env_texture(tx, hdr_texture);
    } else
    if (extension == "hdre")
    {
        HDRE* hdre = HDRE::Get(texture_path.c_str());
        if (!hdre) {
            return nullptr;
        }
        tx->load_from_hdre(hdre);
        current_skybox_texture = tx;
    }
    else
    {
        tx->load(texture_path, is_srgb);
    }

    // register in map
    textures[name] = tx;

    return tx;
}

Surface* RendererStorage::get_surface(const std::string& mesh_path)
{
    std::string name = mesh_path;

    // check if already loaded
    std::map<std::string, Surface*>::iterator it = surfaces.find(mesh_path);
    if (it != surfaces.end())
        return it->second;

    Surface* new_surface = new Surface();

    // register in map
    surfaces[name] = new_surface;

    return new_surface;
}

void RendererStorage::register_animation(const std::string& animation_path, Animation* animation)
{
    // register in map
    animations[animation_path] = animation;
}

Animation* RendererStorage::get_animation(const std::string& animation_path)
{
    // check if already loaded
    std::map<std::string, Animation*>::iterator it = animations.find(animation_path);
    if (it != animations.end())
        return it->second;    

    return NULL;
}

void RendererStorage::register_basic_surfaces()
{
    // Quad
    Surface* quad_mesh = new Surface();
    quad_mesh->create_quad();
    surfaces["quad"] = quad_mesh;

    // Box
    Surface* box_mesh = new Surface();
    box_mesh->create_box();
    surfaces["box"] = box_mesh;

    // Rounded Box
    Surface* rounded_box_mesh = new Surface();
    rounded_box_mesh->create_rounded_box();
    surfaces["rounded_box"] = rounded_box_mesh;

    // Sphere
    Surface* sphere_mesh = new Surface();
    sphere_mesh->create_sphere();
    surfaces["sphere"] = sphere_mesh;

    // Cone
    Surface* cone_mesh = new Surface();
    cone_mesh->create_cone();
    surfaces["cone"] = cone_mesh;

    // Cylinder
    Surface* cylinder_mesh = new Surface();
    cylinder_mesh->create_cylinder();
    surfaces["cylinder"] = cylinder_mesh;

    // Capsule
    Surface* capsule_mesh = new Surface();
    capsule_mesh->create_capsule();
    surfaces["capsule"] = capsule_mesh;

    // Torus
    Surface* torus_mesh = new Surface();
    torus_mesh->create_torus();
    surfaces["torus"] = torus_mesh;
}

std::vector<std::string> RendererStorage::get_common_define_specializations(const Material* material)
{
    bool default_material = false;

    if (!material) {
        material = new Material();
        default_material = true;
    }

    std::vector<std::string> define_specializations;

    if (material->diffuse_texture) {
        define_specializations.push_back("ALBEDO_TEXTURE");
    }

    if (material->metallic_roughness_texture) {
        define_specializations.push_back("METALLIC_ROUGHNESS_TEXTURE");
    }

    if (material->normal_texture) {
        define_specializations.push_back("NORMAL_TEXTURE");
    }

    if (material->emissive_texture) {
        define_specializations.push_back("EMISSIVE_TEXTURE");
    }

    if (material->oclussion_texture) {
        define_specializations.push_back("OCLUSSION_TEXTURE");
    }

    if (!define_specializations.empty()) {
        define_specializations.push_back("USE_SAMPLER");
    }

    switch (material->topology_type) {
    case TOPOLOGY_TRIANGLE_LIST:
        define_specializations.push_back("TRIANGLE_LIST");
        break;
    case TOPOLOGY_TRIANGLE_STRIP:
        define_specializations.push_back("TRIANGLE_STRIP");
        break;
    case TOPOLOGY_LINE_LIST:
        define_specializations.push_back("LINE_LIST");
        break;
    case TOPOLOGY_LINE_STRIP:
        define_specializations.push_back("LINE_STRIP");
        break;
    case TOPOLOGY_POINT_LIST:
        define_specializations.push_back("POINT_LIST");
        break;
    default:
        assert(0);
    }

    switch (material->cull_type) {
    case CULL_NONE:
        define_specializations.push_back("CULL_NONE");
        break;
    case CULL_BACK:
        define_specializations.push_back("CULL_BACK");
        break;
    case CULL_FRONT:
        define_specializations.push_back("CULL_FRONT");
        break;
    default:
        assert(0);
    }

    switch (material->transparency_type) {
    case ALPHA_OPAQUE:
        define_specializations.push_back("ALPHA_OPAQUE");
        break;
    case ALPHA_BLEND:
        define_specializations.push_back("ALPHA_BLEND");
        break;
    case ALPHA_MASK:
        define_specializations.push_back("ALPHA_MASK");
        break;
    case ALPHA_HASH:
        define_specializations.push_back("ALPHA_HASH");
        break;
    }

    if (material->depth_read) {
        define_specializations.push_back("DEPTH_READ");
    }

    if (material->depth_write) {
        define_specializations.push_back("DEPTH_WRITE");
    }

    if (material->use_skinning) {
        define_specializations.push_back("USE_SKINNING");
    }

    if (material->type == MATERIAL_UNLIT) {
        define_specializations.push_back("UNLIT_MATERIAL");
    }

    if (default_material) {
        delete material;
    }

    return define_specializations;
}

void RendererStorage::reload_all_render_pipelines()
{
    for (auto& shader_pair : shaders) {

        Shader* shader = shader_pair.second;
        const Pipeline* pipeline = shader->get_pipeline();

        if (pipeline && pipeline->is_render_pipeline() && pipeline->is_msaa_allowed()) {
            shader->reload();
        }
    }
}

void RendererStorage::register_render_pipeline(Material* material)
{
    if (material->shader && material->shader->get_pipeline()) {
        return;
    }

    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    PipelineDescription description = {};

    switch (material->topology_type) {
    case TOPOLOGY_TRIANGLE_LIST:
        description.topology = WGPUPrimitiveTopology_TriangleList;
        break;
    case TOPOLOGY_TRIANGLE_STRIP:
        description.topology = WGPUPrimitiveTopology_TriangleStrip;
        break;
    case TOPOLOGY_LINE_LIST:
        description.topology = WGPUPrimitiveTopology_LineList;
        break;
    case TOPOLOGY_LINE_STRIP:
        description.topology = WGPUPrimitiveTopology_LineStrip;
        break;
    case TOPOLOGY_POINT_LIST:
        description.topology = WGPUPrimitiveTopology_PointList;
        break;
    default:
        assert(0);
    }

    if (material->is_2D) {
        description.depth_write = false;
    }
    else {
        description.depth_write = material->depth_write;
    }

    switch (material->cull_type) {
    case CULL_NONE:
        description.cull_mode = WGPUCullMode_None;
        break;
    case CULL_BACK:
        description.cull_mode = WGPUCullMode_Back;
        break;
    case CULL_FRONT:
        description.cull_mode = WGPUCullMode_Front;
        break;
    default:
        assert(0);
    }

    bool is_openxr_available = Renderer::instance->get_openxr_available();
    WGPUTextureFormat swapchain_format = is_openxr_available ? webgpu_context->xr_swapchain_format : webgpu_context->swapchain_format;

    WGPUColorTargetState color_target = {};
    color_target.format = swapchain_format;
    color_target.writeMask = WGPUColorWriteMask_All;

    switch (material->transparency_type) {
    case ALPHA_OPAQUE:
        break;
    case ALPHA_BLEND: {
        WGPUBlendState* blend_state = new WGPUBlendState;
        blend_state->color = {
                .operation = WGPUBlendOperation_Add,
                .srcFactor = WGPUBlendFactor_SrcAlpha,
                .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
        };
        blend_state->alpha = {
                .operation = WGPUBlendOperation_Add,
                .srcFactor = WGPUBlendFactor_Zero,
                .dstFactor = WGPUBlendFactor_One,
        };

        color_target.blend = blend_state;

        description.depth_write = false;
        description.blending_enabled = true;
        break;
    }
    case ALPHA_MASK:
        break;
    case ALPHA_HASH:
        break;
    }

    description.depth_read = material->depth_read;
    description.sample_count = Renderer::instance->get_msaa_count();

    RenderPipelineKey key = { material->shader, color_target, description };

    if (registered_render_pipelines.contains(key)) {
        material->shader->set_pipeline(registered_render_pipelines[key]);
        return;
    }

    Pipeline* render_pipeline = new Pipeline();
    render_pipeline->create_render_async(material->shader, color_target, description);
    registered_render_pipelines[key] = render_pipeline;
}

//void RendererStorage::register_compute_pipeline(Shader* shader, WGPUPipelineLayout pipeline_layout)
//{
//    Pipeline* compute_pipeline = new Pipeline();
//    compute_pipeline->create_compute(shader, pipeline_layout);
//    registered_compute_pipelines[shader] = compute_pipeline;
//}

void RendererStorage::clean_registered_pipelines()
{
    for (auto [key, pipeline] : registered_render_pipelines) {
        delete pipeline;
    }

    for (auto [shader, pipeline] : registered_compute_pipelines) {
        delete pipeline;
    }

    registered_render_pipelines.clear();
    registered_compute_pipelines.clear();
}
