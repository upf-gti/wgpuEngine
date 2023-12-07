#include "renderer_storage.h"

#include "mesh.h"
#include "texture.h"
#include "shader.h"
#include "uniform.h"

#include <filesystem>

RendererStorage* RendererStorage::instance = nullptr;

std::map<std::string, Mesh*> RendererStorage::meshes;
std::map<std::string, Texture*> RendererStorage::textures;
std::map<std::string, Shader*> RendererStorage::shaders;
Texture* RendererStorage::current_skybox_texture = nullptr;
std::map<std::string, std::vector<std::string>> RendererStorage::shader_library_references;
std::unordered_map<Material, RendererStorage::sBindingData> RendererStorage::material_bind_groups;
std::unordered_map<void*, RendererStorage::sBindingData> RendererStorage::ui_widget_bind_groups;

RendererStorage::RendererStorage()
{
    instance = this;
}

void RendererStorage::register_material(WebGPUContext* webgpu_context, Material& material)
{
    if (material_bind_groups.contains(material)) {
        return;
    }

    bool uses_textures = false;
    uint32_t binding = 0;

    std::vector<Uniform*>& uniforms = material_bind_groups[material].uniforms;

    if (material.diffuse_texture) {
        Uniform* u = new Uniform();
        u->data = material.diffuse_texture->get_view();
        u->binding = 0;
        uniforms.push_back(u);
        uses_textures |= true;
    } else
    if (material.flags & MATERIAL_PBR) {
        Uniform* u = new Uniform();
        u->data = webgpu_context->create_buffer(sizeof(glm::vec4), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &material.color, "mat_albedo");
        u->binding = 0;
        u->buffer_size = sizeof(glm::vec4);
        uniforms.push_back(u);
    }

    if (material.metallic_roughness_texture) {
        Uniform* u = new Uniform();
        u->data = material.metallic_roughness_texture->get_view();
        u->binding = 1;
        uniforms.push_back(u);
        uses_textures |= true;
    } else
    if (material.flags & MATERIAL_PBR) {
        Uniform* u = new Uniform();
        glm::vec2 metallic_roughness = { material.metalness, material.roughness };
        u->data = webgpu_context->create_buffer(sizeof(glm::vec2), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &metallic_roughness, "mat_metallic_roughness");
        u->binding = 1;
        u->buffer_size = sizeof(glm::vec2);
        uniforms.push_back(u);
    }

    if (material.normal_texture) {
        Uniform* u = new Uniform();
        u->data = material.normal_texture->get_view();
        u->binding = 2;
        uniforms.push_back(u);
        uses_textures |= true;
    }

    if (material.emissive_texture) {
        Uniform* u = new Uniform();
        u->data = material.emissive_texture->get_view();
        u->binding = 3;
        uniforms.push_back(u);
        uses_textures |= true;
    } else
    if (material.flags & MATERIAL_PBR) {
        Uniform* u = new Uniform();
        u->data = webgpu_context->create_buffer(sizeof(glm::vec4), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, &material.emissive, "mat_emissive");
        u->binding = 3;
        u->buffer_size = sizeof(glm::vec4);
        uniforms.push_back(u);
    }

    // Add a sampler for basic 2d textures if there's any texture as uniforms
    if (uses_textures)
    {
        Uniform* sampler_uniform = new Uniform();
        sampler_uniform->data = webgpu_context->create_sampler(
            material.diffuse_texture->get_wrap_u(),
            material.diffuse_texture->get_wrap_v(),
            WGPUAddressMode_ClampToEdge,
            WGPUFilterMode_Linear,
            WGPUFilterMode_Linear,
            WGPUMipmapFilterMode_Linear,
            static_cast<float>(material.diffuse_texture->get_mipmap_count()),
            4
        );
        sampler_uniform->binding = 4;
        uniforms.push_back(sampler_uniform);
    }

    material_bind_groups[material].bind_group = webgpu_context->create_bind_group(uniforms, material.shader, 2);
}

WGPUBindGroup RendererStorage::get_material_bind_group(const Material& material)
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

WGPUBindGroup RendererStorage::get_ui_widget_bind_group(void* widget)
{
    if (!ui_widget_bind_groups.contains(widget)) {
        assert(false);
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

Shader* RendererStorage::get_shader(const std::string& shader_path, std::vector<std::string> define_specializations)
{
    std::string name = std::filesystem::relative(std::filesystem::path(shader_path)).string();

    std::string specialized_name = name;
    for (const std::string& specialization : define_specializations) {
        specialized_name += "_" + specialization;
    }

    // check if already loaded
    std::map<std::string, Shader*>::iterator it = shaders.find(specialized_name);
    if (it != shaders.end())
        return it->second;

    Shader* sh = new Shader();

    if (!sh->load(name, define_specializations)) {
        return nullptr;
    }

    // register in map
    shaders[specialized_name] = sh;

    return sh;
}

std::vector<std::string> RendererStorage::get_shader_for_reload(const std::string& shader_path)
{
    std::string name = shader_path;

    // Check if already loaded
    auto it0 = shaders.find(shader_path);
    if (it0 != shaders.end())
        return { shader_path };

    // If it is not a shader, check if it is a library
    auto it1 = shader_library_references.find(shader_path);
    if (it1 != shader_library_references.end())
        return shader_library_references[shader_path];

    // The shader is not being used
    return {};
}

Texture* RendererStorage::get_texture(const std::string& texture_path)
{
    std::string name = texture_path;

    // check if already loaded
    std::map<std::string, Texture*>::iterator it = textures.find(texture_path);
    if (it != textures.end())
        return it->second;

    Texture* tx = new Texture();

    std::string extension = texture_path.substr(texture_path.find_last_of(".") + 1);

    if (extension != "hdre")
    {
        tx->load(texture_path);
    }
    else
    {
        HDRE* hdre = HDRE::Get(texture_path.c_str());
        tx->load_from_hdre(hdre);
        current_skybox_texture = tx;
    }

    // register in map
    textures[name] = tx;

    return tx;
}

Mesh* RendererStorage::get_mesh(const std::string& mesh_path)
{
    std::string name = mesh_path;

    // check if already loaded
    std::map<std::string, Mesh*>::iterator it = meshes.find(mesh_path);
    if (it != meshes.end())
        return it->second;

    Mesh* new_mesh = new Mesh();

    // register in map
    meshes[name] = new_mesh;

    return new_mesh;
}

void RendererStorage::register_basic_meshes()
{
    // Quad
    Mesh* quad_mesh = new Mesh();
    quad_mesh->create_quad();
    meshes["quad"] = quad_mesh;

    // Box
    Mesh* box_mesh = new Mesh();
    box_mesh->create_box();
    meshes["box"] = box_mesh;

    // Rounded Box
    Mesh* rounded_box_mesh = new Mesh();
    rounded_box_mesh->create_rounded_box();
    meshes["rounded_box"] = rounded_box_mesh;

    // Sphere
    Mesh* sphere_mesh = new Mesh();
    sphere_mesh->create_sphere();
    meshes["sphere"] = sphere_mesh;

    // Cone
    Mesh* cone_mesh = new Mesh();
    cone_mesh->create_cone();
    meshes["cone"] = cone_mesh;

    // Cylinder
    Mesh* cylinder_mesh = new Mesh();
    cylinder_mesh->create_cylinder();
    meshes["cylinder"] = cylinder_mesh;

    // Capsule
    Mesh* capsule_mesh = new Mesh();
    capsule_mesh->create_capsule();
    meshes["capsule"] = capsule_mesh;

    // Torus
    Mesh* torus_mesh = new Mesh();
    torus_mesh->create_torus();
    meshes["torus"] = torus_mesh;
}
