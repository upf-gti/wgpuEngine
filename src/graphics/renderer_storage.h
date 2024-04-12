#pragma once

#include "includes.h"

#include <map>
#include <unordered_map>
#include <string>
#include <vector>

#include "webgpu_context.h"
#include "material.h"
#include "graphics/mesh_instance.h"
#include "framework/animation/skeleton.h"
#include "framework/animation/animation.h"

class Surface;
class Texture;
class Shader;
struct Uniform;

class RendererStorage {

public:

    RendererStorage();

    struct AnimationData {
        Animation* animation;
        std::string node_path;
        AnimationType animation_type = AnimationType::ANIM_TYPE_SIMPLE;
    };

    // Singleton
    static RendererStorage* instance;

    static std::map<std::string, Shader*> shaders;
    static std::map<std::string, std::vector<std::string>> shader_library_references;
    static std::map<std::string, Texture*> textures;
    static std::map<std::string, Surface*> surfaces;
    static std::map<std::string, Skeleton*> skeletons;
    static std::map<std::string, RendererStorage::AnimationData*> animations;

    static Texture* current_skybox_texture;

    struct sUIData {
        // Common
        float is_hovered = 0.f;
        // Groups
        float num_group_items = 2; // combo buttons use this prop by now to the index in combo
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

    static void register_material(WebGPUContext* webgpu_context, MeshInstance* mesh_instance, const Material& material);
    static WGPUBindGroup get_material_bind_group(const Material& material);

    static void register_ui_widget(WebGPUContext* webgpu_context, Shader* shader, void* widget, const sUIData& ui_data, uint8_t bind_group_id);
    static WGPUBindGroup get_ui_widget_bind_group(const void* widget);
    static void update_ui_widget(WebGPUContext* webgpu_context, void* entity_mesh, const sUIData& ui_data);

    static Shader* get_shader(const std::string& shader_path, const Material& material = {},
        const std::vector<std::string>& custom_define_specializations = {});

    static Shader* get_shader(const std::string& shader_path, const std::vector<std::string>& custom_define_specializations);

    static Shader* get_shader_from_source(const std::string& source, const std::string& name = "", const Material& material = {},
        const std::vector<std::string>& custom_define_specializations = {});

    static Shader* get_shader_from_source(const std::string& source, const std::string& name = "",
        const std::vector<std::string>& custom_define_specializations = {});

    static void reload_shader(const std::string& shader_path);

    static Texture* get_texture(const std::string& texture_path, bool is_srgb = false);

    static Surface* get_surface(const std::string& mesh_path);

    static void register_basic_surfaces();

    static std::vector<std::string> get_common_define_specializations(const Material& material);

    static void reload_all_render_pipelines();

    static void register_skeleton(const std::string& node_path, Skeleton* skeleton);
    static Skeleton* get_skeleton(const std::string& node_path);

    static void register_animation(const std::string& animation_path, Animation* animation, const std::string& node_path, AnimationType type);
    static RendererStorage::AnimationData* get_animation(const std::string& animation_path);

};
