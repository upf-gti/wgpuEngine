#pragma once

#include "includes.h"

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "framework/utils/hash.h"
#include "graphics/uniforms_structs.h"

class Surface;
class Texture;
class Shader;
class Material;
class Mesh;
class Animation;
struct Uniform;

enum TextureStorageFlags : uint8_t {
    TEXTURE_STORAGE_NONE = 0,
    TEXTURE_STORAGE_SRGB = 1 << 0,
    TEXTURE_STORAGE_KEEP_MEMORY = 1 << 1,
    TEXTURE_STORAGE_STORE_DATA = 1 << 2,
    TEXTURE_STORAGE_UI = TEXTURE_STORAGE_SRGB | TEXTURE_STORAGE_KEEP_MEMORY
};

class RenderStorage {
    friend class RenderManager;

public:
    inline static RenderStorage* get_singleton()
    {
        return singleton_instance;
    }

    struct sBindingData {
        std::vector<Uniform*> uniforms;
        // Depending on material properties, uniforms will have different indices in the array
        std::unordered_map<eMaterialProperties, uint8_t> uniform_indices;
        WGPUBindGroup bind_group;
    };

    void register_material_bind_group(Mesh* mesh, Material* material);
    WGPUBindGroup get_material_bind_group(const Material* material);

    void delete_material_bind_group(Material* material);

    void update_material_bind_group(Mesh* mesh, Material* material);

    void register_ui_widget(Shader* shader, void* widget, const sUIData& ui_data, uint8_t bind_group_id, bool force = false);
    WGPUBindGroup get_ui_widget_bind_group(const void* widget);
    void update_ui_widget(void* entity_mesh, const sUIData& ui_data);

    void delete_ui_widget(void* entity_mesh);

    Shader* get_shader(const std::string& shader_path, const Material* material = nullptr,
            const std::vector<std::string>& custom_define_specializations = {});

    Shader* get_shader(const std::string& shader_path, const std::vector<std::string>& custom_define_specializations);

#ifdef __EMSCRIPTEN__
    Shader* get_shader_from_name(const std::string& name, const Material* material);
#endif

    Shader* get_shader_from_source(const char* source, const std::string& name,
            const std::vector<std::string>& libraries,
            const Material* material = nullptr,
            const std::vector<std::string>& custom_define_specializations = {});

    Shader* get_shader_from_source(const char* source, const std::string& name,
            const std::vector<std::string>& libraries,
            const std::vector<std::string>& custom_define_specializations);

    void reload_shader(const std::string& shader_path);
    void reload_engine_shader(const std::string& shader_path);

    Texture* get_texture(const std::string& texture_path, TextureStorageFlags flags = TEXTURE_STORAGE_NONE);

    std::vector<std::string> get_common_define_specializations(const Material* material);

    void reload_all_render_pipelines();

    void register_animation(const std::string& animation_path, Animation* animation);
    void erase_animation(const std::string& animation_path);
    Animation* get_animation(const std::string& animation_path);

    void register_render_pipeline(Material* material);
    //void register_compute_pipeline(Shader* shader, WGPUPipelineLayout pipeline_layout);

    RenderPipelineKey get_render_pipeline_key(Material* material);
    void clean_registered_pipelines();

private:
    static inline RenderStorage* singleton_instance = nullptr;

    RenderStorage() = default;
    ~RenderStorage() = default;

    Error initialize();
    Error finalize();

    std::map<std::string, Shader*> shaders;
    std::map<std::string, const char*> engine_shaders_refs;
    std::map<std::string, std::vector<std::string>> shader_library_references;
    std::map<std::string, Texture*> textures;
    std::map<std::string, Animation*> animations;

    std::unordered_map<RenderPipelineKey, Pipeline*> registered_render_pipelines;
    std::unordered_map<Shader*, Pipeline*> registered_compute_pipelines;

    Texture* current_skybox_texture;

    std::unordered_map<const Material*, sBindingData> material_bind_groups;
    std::unordered_map<const void*, sBindingData> ui_widget_bind_groups;
};
