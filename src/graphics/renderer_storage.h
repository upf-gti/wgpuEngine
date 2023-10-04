#pragma once

#include <map>
#include <string>
#include <vector>

class Mesh;
class Texture;
class Shader;

class RendererStorage {

public:

    RendererStorage();

    // Singleton
    static RendererStorage* instance;

    static std::map<std::string, Shader*> shaders;
    static std::map<std::string, std::vector<std::string>> shader_library_references;
    static std::map<std::string, Texture*> textures;
    static std::map<std::string, Mesh*> meshes;

    static Shader* get_shader(const std::string& shader_path);
    static std::vector<std::string> get_shader_for_reload(const std::string& shader_path);

    static Texture* get_texture(const std::string& texture_path);

    static Mesh* get_mesh(const std::string& mesh_path);

};
