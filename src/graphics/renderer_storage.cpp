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

void RendererStorage::register_material_bind_group(WebGPUContext* webgpu_context, MeshInstance* mesh_instance, Material* material)
{
    if (material_bind_groups.contains(material)) {

        if (material->get_dirty_flags() & PROP_RELOAD_NEEDED) {
            delete_material_bind_group(webgpu_context, material);
            const Shader* old_shader = material->get_shader();
            // TODO: try to cache shaders and use as resource
            auto& libraries = shader_library_references[old_shader->get_path()];
            material->set_shader(get_shader_from_source(engine_shaders_refs[old_shader->get_path()], old_shader->get_path(), libraries, material));
        } else
        if (material->get_dirty_flags() & PROP_UPDATE_NEEDED) {
            update_material_bind_group(webgpu_context, mesh_instance, material);
            return;
        }
        else {
            return;
        }
    }

    bool uses_textures = false;
    uint32_t binding = 0;

    std::vector<Uniform*>& uniforms = material_bind_groups[material].uniforms;
    std::unordered_map<eMaterialProperties, uint8_t>& uniform_indices = material_bind_groups[material].uniform_indices;
    const Texture* texture_ref = nullptr;

    Texture* diffuse_texture = material->get_diffuse_texture();
    if (diffuse_texture) {
        Uniform* u = new Uniform();
        uint32_t array_layers = diffuse_texture->get_array_layers();
        WGPUTextureViewDimension view_dimension = array_layers > 1 ? WGPUTextureViewDimension_Cube : WGPUTextureViewDimension_2D;
        if (diffuse_texture->get_dimension() == WGPUTextureDimension_3D) {
            view_dimension = WGPUTextureViewDimension_3D;
            array_layers = 1;
        }
        u->data = diffuse_texture->get_view(view_dimension, 0, diffuse_texture->get_mipmap_count(), 0, array_layers);
        u->binding = 0;
        uniforms.push_back(u);
        uses_textures |= true;
        texture_ref = diffuse_texture;
    }

    {
        Uniform* u = new Uniform();
        const glm::vec4& color = material->get_color();
        u->data = webgpu_context->create_buffer(sizeof(glm::vec4), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &color, "mat_albedo");
        u->binding = 1;
        u->buffer_size = sizeof(glm::vec4);
        uniform_indices[eMaterialProperties::PROP_COLOR] = uniforms.size();
        uniforms.push_back(u);
    }

    Texture* metallic_roughness_texture = material->get_metallic_roughness_texture();
    if (metallic_roughness_texture) {
        Uniform* u = new Uniform();
        u->data = metallic_roughness_texture->get_view(WGPUTextureViewDimension_2D, 0, metallic_roughness_texture->get_mipmap_count());
        u->binding = 2;
        uniforms.push_back(u);
        uses_textures |= true;
        texture_ref = metallic_roughness_texture;
    }

    if (material->get_type() == MATERIAL_PBR) {
        Uniform* u = new Uniform();
        glm::vec3 occlusion_roughness_metallic = { material->get_occlusion(), material->get_roughness(), material->get_metallic() };
        u->data = webgpu_context->create_buffer(sizeof(glm::vec3), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &occlusion_roughness_metallic, "mat_occlusion_roughness_metallic");
        u->binding = 3;
        u->buffer_size = sizeof(glm::vec3);
        uniform_indices[eMaterialProperties::PROP_OCLUSSION_ROUGHNESS_METALLIC] = uniforms.size();
        uniforms.push_back(u);
    }

    Texture* normal_texture = material->get_normal_texture();
    if (normal_texture) {
        Uniform* u = new Uniform();
        u->data = normal_texture->get_view(WGPUTextureViewDimension_2D, 0, normal_texture->get_mipmap_count());
        u->binding = 4;
        uniforms.push_back(u);
        uses_textures |= true;
        texture_ref = normal_texture;
    }

    Texture* emissive_texture = material->get_emissive_texture();
    if (emissive_texture) {
        Uniform* u = new Uniform();
        u->data = emissive_texture->get_view(WGPUTextureViewDimension_2D, 0, emissive_texture->get_mipmap_count());
        u->binding = 5;
        uniforms.push_back(u);
        uses_textures |= true;
        texture_ref = emissive_texture;
    }

    if (material->get_type() == MATERIAL_PBR) {
        Uniform* u = new Uniform();
        const glm::vec3& emissive = material->get_emissive();
        u->data = webgpu_context->create_buffer(sizeof(glm::vec3), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &emissive, "mat_emissive");
        u->binding = 6;
        u->buffer_size = sizeof(glm::vec3);
        uniform_indices[eMaterialProperties::PROP_EMISSIVE] = uniforms.size();
        uniforms.push_back(u);
    }

    Texture* occlusion_texture = material->get_occlusion_texture();
    if (occlusion_texture) {
        Uniform* u = new Uniform();
        u->data = occlusion_texture->get_view(WGPUTextureViewDimension_2D, 0, occlusion_texture->get_mipmap_count());
        u->binding = 9;
        uniforms.push_back(u);
        uses_textures |= true;
        texture_ref = occlusion_texture;
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

    if (material->get_transparency_type() == ALPHA_MASK) {
        Uniform* u = new Uniform();
        float alpha_mask = material->get_alpha_mask();
        u->data = webgpu_context->create_buffer(sizeof(float), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &alpha_mask, "mat_alpha_cutoff");
        u->binding = 8;
        u->buffer_size = sizeof(float);
        uniform_indices[eMaterialProperties::PROP_ALPHA_MASK] = uniforms.size();
        uniforms.push_back(u);
    }

    if (material->get_use_skinning()) {

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

    material->reset_dirty_flags();

    material_bind_groups[material].bind_group = webgpu_context->create_bind_group(uniforms, material->get_shader(), 2);
}

WGPUBindGroup RendererStorage::get_material_bind_group(const Material* material)
{
    auto it = material_bind_groups.find(material);
    if (it == material_bind_groups.end()) {
        assert(false);
    }

    return it->second.bind_group;
}

void RendererStorage::delete_material_bind_group(WebGPUContext* webgpu_context, Material* material)
{
    auto it = material_bind_groups.find(material);

    if (it != material_bind_groups.end()) {
        wgpuBindGroupRelease(it->second.bind_group);

        for (auto uniform : it->second.uniforms) {
            uniform->destroy();
        }

        material_bind_groups.erase(it);
    }
}

void RendererStorage::update_material_bind_group(WebGPUContext* webgpu_context, MeshInstance* mesh_instance, Material* material)
{
    std::vector<Uniform*>& uniforms = material_bind_groups[material].uniforms;
    std::unordered_map<eMaterialProperties, uint8_t>& uniform_indices = material_bind_groups[material].uniform_indices;

    uint32_t dirty_flags = material->get_dirty_flags();

    if (dirty_flags & eMaterialProperties::PROP_COLOR) {
        Uniform* u = uniforms[uniform_indices[eMaterialProperties::PROP_COLOR]];
        const glm::vec4& color = material->get_color();
        webgpu_context->update_buffer(std::get<WGPUBuffer>(u->data), 0, &color, sizeof(glm::vec4));
    }

    if (dirty_flags & eMaterialProperties::PROP_OCLUSSION_ROUGHNESS_METALLIC && material->get_type() == MATERIAL_PBR) {
        Uniform* u = uniforms[uniform_indices[eMaterialProperties::PROP_OCLUSSION_ROUGHNESS_METALLIC]];
        glm::vec3 occlusion_roughness_metallic = { material->get_occlusion(), material->get_roughness(), material->get_metallic() };
        webgpu_context->update_buffer(std::get<WGPUBuffer>(u->data), 0, &occlusion_roughness_metallic, sizeof(glm::vec3));
    }

    if (dirty_flags & eMaterialProperties::PROP_EMISSIVE && material->get_type() == MATERIAL_PBR) {
        Uniform* u = uniforms[uniform_indices[eMaterialProperties::PROP_EMISSIVE]];
        const glm::vec3& emissive = material->get_emissive();
        webgpu_context->update_buffer(std::get<WGPUBuffer>(u->data), 0, &emissive, sizeof(glm::vec3));
    }

    if (dirty_flags & eMaterialProperties::PROP_ALPHA_MASK && material->get_transparency_type() == ALPHA_MASK) {
        Uniform* u = uniforms[uniform_indices[eMaterialProperties::PROP_ALPHA_MASK]];
        float alpha_mask = material->get_alpha_mask();
        webgpu_context->update_buffer(std::get<WGPUBuffer>(u->data), 0, &alpha_mask, sizeof(float));
    }

    material->reset_dirty_flags();
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

    RendererStorage::sBindingData& binding_data = ui_widget_bind_groups[entity_mesh];
    binding_data.uniforms = ui_widget_bind_groups[entity_mesh].uniforms;

    binding_data.uniforms.push_back(data_uniform);

    binding_data.bind_group = webgpu_context->create_bind_group(binding_data.uniforms, shader, bind_group_id);
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

void RendererStorage::delete_ui_widget(WebGPUContext* webgpu_context, void* entity_mesh)
{
    auto it = ui_widget_bind_groups.find(entity_mesh);

    if (it != ui_widget_bind_groups.end()) {
        wgpuBindGroupRelease(it->second.bind_group);

        for (auto uniform : it->second.uniforms) {
            uniform->destroy();
        }

        ui_widget_bind_groups.erase(it);
    }
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

Shader* RendererStorage::get_shader_from_source(const char* source, const std::string& name,
    const std::vector<std::string>& libraries,
    const Material* material,
    const std::vector<std::string>& custom_define_specializations)
{
    std::vector<std::string> define_specializations = get_common_define_specializations(material);

    // concatenate
    define_specializations.insert(define_specializations.end(), custom_define_specializations.begin(), custom_define_specializations.end());

    return get_shader_from_source(source, name, libraries, define_specializations);
}

Shader* RendererStorage::get_shader_from_source(const char* source, const std::string& name,
    const std::vector<std::string>& libraries,
    const std::vector<std::string>& custom_define_specializations)
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

    if (!sh->load_from_source(source, name, libraries, specialized_name, custom_define_specializations)) {
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

void RendererStorage::reload_engine_shader(const std::string& shader_path)
{
    std::filesystem::path fs_shader_path = std::filesystem::path(shader_path);
    std::string name = fs_shader_path.filename().string();

    // Check if already loaded
    for (auto& [shader_name, shader] : shaders) {
        if (shader_name.find(name) != std::string::npos) {
            shader->reload(shader_path);
        }
    }

    // If it is not a shader, check if it is a library
    std::string folder = fs_shader_path.parent_path().string();
    Shader::reload_engine_library(folder, name);

    auto it1 = shader_library_references.find(name);
    if (it1 != shader_library_references.end())
    {
        const std::vector<std::string> library_refs = shader_library_references[name];
        shader_library_references.erase(it1);
        for (auto& library_ref : library_refs) {
            for (auto& [shader_to_reload_name, shader] : shaders) {
                if (shader_to_reload_name.find(library_ref) != std::string::npos) {
                    Shader* shader = shaders[shader_to_reload_name];
                    shader->reload(fs_shader_path.parent_path().string() + "/" + library_ref);
                }
            }
        }
    }
}

Texture* RendererStorage::get_texture(const std::string& texture_path, TextureStorageFlags flags)
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
        bool is_srgb = flags & TEXTURE_STORAGE_SRGB;
        tx->load(texture_path, is_srgb);

        // Ref to keep memory alive
        if (flags & TEXTURE_STORAGE_KEEP_MEMORY) {
            tx->ref();
        }
    }

    // register in map
    textures[name] = tx;

    tx->set_name(name);

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

    return nullptr;
}

void RendererStorage::register_basic_surfaces()
{
    // Quad
    Surface* quad_mesh = new Surface();
    quad_mesh->create_quad();
    quad_mesh->ref();
    surfaces["quad"] = quad_mesh;

    // Box
    Surface* box_mesh = new Surface();
    box_mesh->create_box();
    box_mesh->ref();
    surfaces["box"] = box_mesh;

    // Rounded Box
    Surface* rounded_box_mesh = new Surface();
    rounded_box_mesh->create_rounded_box();
    rounded_box_mesh->ref();
    surfaces["rounded_box"] = rounded_box_mesh;

    // Sphere
    Surface* sphere_mesh = new Surface();
    sphere_mesh->create_sphere();
    sphere_mesh->ref();
    surfaces["sphere"] = sphere_mesh;

    // Cone
    Surface* cone_mesh = new Surface();
    cone_mesh->create_cone();
    cone_mesh->ref();
    surfaces["cone"] = cone_mesh;

    // Cylinder
    Surface* cylinder_mesh = new Surface();
    cylinder_mesh->create_cylinder();
    cylinder_mesh->ref();
    surfaces["cylinder"] = cylinder_mesh;

    // Capsule
    Surface* capsule_mesh = new Surface();
    capsule_mesh->create_capsule();
    capsule_mesh->ref();
    surfaces["capsule"] = capsule_mesh;

    // Torus
    Surface* torus_mesh = new Surface();
    torus_mesh->create_torus();
    torus_mesh->ref();
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

    if (material->get_diffuse_texture()) {
        define_specializations.push_back("ALBEDO_TEXTURE");
    }

    if (material->get_metallic_roughness_texture()) {
        define_specializations.push_back("METALLIC_ROUGHNESS_TEXTURE");
    }

    if (material->get_normal_texture()) {
        define_specializations.push_back("NORMAL_TEXTURE");
    }

    if (material->get_emissive_texture()) {
        define_specializations.push_back("EMISSIVE_TEXTURE");
    }

    if (material->get_occlusion_texture()) {
        define_specializations.push_back("OCLUSSION_TEXTURE");
    }

    if (!define_specializations.empty()) {
        define_specializations.push_back("USE_SAMPLER");
    }

    switch (material->get_topology_type()) {
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

    switch (material->get_cull_type()) {
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

    switch (material->get_transparency_type()) {
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

    if (material->get_depth_read()) {
        define_specializations.push_back("DEPTH_READ");
    }

    if (material->get_depth_write()) {
        define_specializations.push_back("DEPTH_WRITE");
    }

    if (material->get_use_skinning()) {
        define_specializations.push_back("USE_SKINNING");
    }

    if (material->get_type() == MATERIAL_UNLIT) {
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
    if (material->get_shader() && material->get_shader()->get_pipeline()) {
        return;
    }

    RenderPipelineKey key = get_render_pipeline_key(material);

    if (registered_render_pipelines.contains(key)) {
        material->set_shader_pipeline(registered_render_pipelines[key]);
        return;
    }

    Pipeline* render_pipeline = new Pipeline();
    render_pipeline->create_render_async(material->get_shader_ref(), key.color_target, key.description);
    registered_render_pipelines[key] = render_pipeline;
}

//void RendererStorage::register_compute_pipeline(Shader* shader, WGPUPipelineLayout pipeline_layout)
//{
//    Pipeline* compute_pipeline = new Pipeline();
//    compute_pipeline->create_compute(shader, pipeline_layout);
//    registered_compute_pipelines[shader] = compute_pipeline;
//}

RenderPipelineKey RendererStorage::get_render_pipeline_key(Material* material)
{

    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    RenderPipelineDescription description = {};

    switch (material->get_topology_type()) {
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

    if (material->get_is_2D()) {
        description.depth_write = WGPUOptionalBool_False;
    }
    else {
        description.depth_write = material->get_depth_write() ? WGPUOptionalBool_True : WGPUOptionalBool_False;
    }

    switch (material->get_cull_type()) {
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

    switch (material->get_transparency_type()) {
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

        description.depth_write = WGPUOptionalBool_False;
        description.blending_enabled = true;
        break;
    }
    case ALPHA_MASK:
        break;
    case ALPHA_HASH:
        break;
    }

    description.depth_read = material->get_depth_read();
    description.sample_count = Renderer::instance->get_msaa_count();

    return { material->get_shader(), color_target, description, material->get_shader()->get_pipeline_layout() };
}

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
