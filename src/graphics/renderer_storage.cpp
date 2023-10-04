#include "renderer_storage.h"

#include "mesh.h"
#include "texture.h"
#include "shader.h"

RendererStorage* RendererStorage::instance = nullptr;

std::map<std::string, Mesh*> RendererStorage::meshes;
std::map<std::string, Texture*> RendererStorage::textures;
std::map<std::string, Shader*> RendererStorage::shaders;
std::map<std::string, std::vector<std::string>> RendererStorage::shader_library_references;

RendererStorage::RendererStorage()
{
    instance = this;
}

Shader* RendererStorage::get_shader(const std::string& shader_path)
{
    std::string name = shader_path;

    // check if already loaded
    std::map<std::string, Shader*>::iterator it = shaders.find(shader_path);
    if (it != shaders.end())
        return it->second;

    Shader* sh = new Shader();

    if (!sh->load(shader_path)) {
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
