#pragma once

#include "includes.h"

#include <map>
#include <unordered_map>
#include <string>
#include <vector>

#include "webgpu_context.h"
#include "material.h"

class Mesh;
class Texture;
class Shader;
struct Uniform;

class RendererStorage {

public:

    RendererStorage();

    // Singleton
    static RendererStorage* instance;

    static std::map<std::string, Shader*> shaders;
    static std::map<std::string, std::vector<std::string>> shader_library_references;
    static std::map<std::string, Texture*> textures;
    static std::map<std::string, Mesh*> meshes;

    struct sUIData {
        float num_group_items = 2;
        float is_selected = 0.f;
        float is_hovered = 0.f;
        float is_color_button = 0.f;
    };

    struct sBindingData {
        std::vector<Uniform*> uniforms;
        WGPUBindGroup bind_group;
    };

    static std::unordered_map<Material, sBindingData> material_bind_groups;
    static std::unordered_map<void*, sBindingData> ui_widget_bind_groups;

    static void register_material(WebGPUContext* webgpu_context, const Material& material);
    static WGPUBindGroup get_material_bind_group(const Material& material);

    static void register_ui_widget(WebGPUContext* webgpu_context, Shader* shader, void* widget, const sUIData& ui_data, uint8_t bind_group_id);
    static WGPUBindGroup get_ui_widget_bind_group(void* widget);
    static void update_ui_widget(WebGPUContext* webgpu_context, void* widget, const sUIData& ui_data);

    static Shader* get_shader(const std::string& shader_path);
    static std::vector<std::string> get_shader_for_reload(const std::string& shader_path);

    static Texture* get_texture(const std::string& texture_path);

    static Mesh* get_mesh(const std::string& mesh_path);

};
