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
std::map<std::string, std::vector<std::string>> RendererStorage::shader_library_references;
std::unordered_map<Material, RendererStorage::sBindingData> RendererStorage::material_bind_groups;
std::unordered_map<void*, RendererStorage::sBindingData> RendererStorage::ui_widget_bind_groups;

RendererStorage::RendererStorage()
{
    instance = this;
}

void RendererStorage::register_material(WebGPUContext* webgpu_context, const Material& material)
{
    if (material_bind_groups.contains(material)) {
        return;
    }

    if (!material.diffuse) {
        return;
    }

    Uniform* albedo_uniform = new Uniform();
    albedo_uniform->data = material.diffuse->get_view();
    albedo_uniform->binding = 0;

    Uniform* sampler_uniform = new Uniform();
    sampler_uniform->data = webgpu_context->create_sampler(); // Using all default params
    sampler_uniform->binding = 1;

    std::vector<Uniform*>& uniforms = material_bind_groups[material].uniforms;

    uniforms.push_back(albedo_uniform);
    uniforms.push_back(sampler_uniform);

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

Shader* RendererStorage::get_shader(const std::string& shader_path)
{
    std::string name = std::filesystem::relative(std::filesystem::path(shader_path)).string();

    // check if already loaded
    std::map<std::string, Shader*>::iterator it = shaders.find(name);
    if (it != shaders.end())
        return it->second;

    Shader* sh = new Shader();

    if (!sh->load(name)) {
        return nullptr;
    }

    // register in map
    shaders[name] = sh;

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
    tx->load(texture_path);

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

    // Cylinder
    Mesh* cylinder_mesh = new Mesh();
    cylinder_mesh->create_cylinder();
    meshes["cylinder"] = cylinder_mesh;

    // Cone
    Mesh* cone_mesh = new Mesh();
    cone_mesh->create_cone();
    meshes["cone"] = cone_mesh;
}
