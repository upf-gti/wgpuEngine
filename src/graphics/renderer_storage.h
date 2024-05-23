#pragma once

#include "includes.h"

#include <map>
#include <unordered_map>
#include <string>
#include <vector>

#include "webgpu_context.h"
#include "material.h"
#include "graphics/uniforms_structs.h"
#include "framework/utils/hash.h"

class Surface;
class Texture;
class Shader;
class MeshInstance;
class Animation;
struct Uniform;

class RendererStorage {

public:

    RendererStorage();

    // Singleton
    static RendererStorage* instance;

    static std::map<std::string, Shader*> shaders;
    static std::map<std::string, const char*> engine_shaders_refs;
    static std::map<std::string, std::vector<std::string>> shader_library_references;
    static std::map<std::string, Texture*> textures;
    static std::map<std::string, Surface*> surfaces;
    static std::map<std::string, Animation*> animations;

    static std::unordered_map<RenderPipelineKey, Pipeline*> registered_render_pipelines;
    static std::unordered_map<Shader*, Pipeline*> registered_compute_pipelines;

    static Texture* current_skybox_texture;

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

    static Shader* get_shader_from_source(const char* source, const std::string& name, const Material& material = {},
        const std::vector<std::string>& custom_define_specializations = {});

    static Shader* get_shader_from_source(const char* source, const std::string& name,
        const std::vector<std::string>& custom_define_specializations);

    static void reload_shader(const std::string& shader_path);

    static Texture* get_texture(const std::string& texture_path, bool is_srgb = false);

    static Surface* get_surface(const std::string& mesh_path);

    static void register_basic_surfaces();

    static std::vector<std::string> get_common_define_specializations(const Material& material);

    static void reload_all_render_pipelines();

    static void register_animation(const std::string& animation_path, Animation* animation);
    static Animation* get_animation(const std::string& animation_path);

    static void register_render_pipeline(Material& material);
    static void register_compute_pipeline(Shader* shader, WGPUPipelineLayout pipeline_layout);
    static void clean_registered_pipelines();

};
