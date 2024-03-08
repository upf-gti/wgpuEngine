#pragma once

#include "includes.h"

#include <map>
#include <unordered_map>
#include <string>
#include <vector>

#include "webgpu_context.h"
#include "material.h"

class Surface;
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
    static std::map<std::string, Surface*> surfaces;

    static Texture* current_skybox_texture;

    struct sUIData {
        // Common
        float is_hovered = 0.f;
        // Groups
        float num_group_items = 2;
        // Buttons
        float is_selected = 0.f;
        float is_color_button = 0.f;

        // Color Picker
        glm::vec4 picker_color = glm::vec4(1.f);

        // To keep rgb if icon has colors...
        float keep_rgb = 0.f;
        // Slider
        float slider_value = 0.f;
        float slider_max = 1.0f;
        // Disable buttons to use them as group icons
        float is_button_disabled = 0.f;
    };

    struct sBindingData {
        std::vector<Uniform*> uniforms;
        WGPUBindGroup bind_group;
    };

    static std::unordered_map<Material, sBindingData> material_bind_groups;
    static std::unordered_map<const void*, sBindingData> ui_widget_bind_groups;

    static void register_material(WebGPUContext* webgpu_context, const Material& material);
    static WGPUBindGroup get_material_bind_group(const Material& material);

    static void register_ui_widget(WebGPUContext* webgpu_context, Shader* shader, void* widget, const sUIData& ui_data, uint8_t bind_group_id);
    static WGPUBindGroup get_ui_widget_bind_group(const void* widget);
    static void update_ui_widget(WebGPUContext* webgpu_context, void* entity_mesh, const sUIData& ui_data);

    static Shader* get_shader(const std::string& shader_path, const Material& material = {},
        const std::vector<std::string>& custom_define_specializations = {});

    static Shader* get_shader(const std::string& shader_path, const std::vector<std::string>& custom_define_specializations);

    static std::vector<std::string> get_shader_for_reload(const std::string& shader_path);

    static Texture* get_texture(const std::string& texture_path);

    static Surface* get_surface(const std::string& mesh_path);

    static void register_basic_surfaces();

    static std::vector<std::string> get_common_define_specializations(const Material& material);

    static void reload_all_render_pipelines();

};
