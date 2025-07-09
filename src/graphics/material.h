#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>

#include "framework/resources/resource.h"

#include <string>

#define MAX_UV_TRANSFORMS 12u

class Shader;
class Texture;
class Pipeline;

enum eMaterialType {
    MATERIAL_PBR,
    MATERIAL_UNLIT,
    MATERIAL_UI
};

enum eTransparencyType {
    ALPHA_OPAQUE,
    ALPHA_BLEND,
    ALPHA_MASK,
    ALPHA_HASH // TODO
};

enum eTopologyType {
    TOPOLOGY_POINT_LIST,
    TOPOLOGY_LINE_LIST,
    TOPOLOGY_LINE_STRIP,
    TOPOLOGY_TRIANGLE_LIST,
    TOPOLOGY_TRIANGLE_STRIP
};

enum eCullType {
    CULL_NONE,
    CULL_BACK,
    CULL_FRONT
};

enum eTextureUVTransform {
    DIFFUSE_UV_TRANSFORM,
    NORMAL_UV_TRANSFORM,
    METALLIC_ROUGHNESS_UV_TRANSFORM,
    EMISSIVE_UV_TRANSFORM,
    OCCLUSION_UV_TRANSFORM,
    CLEARCOAT_UV_TRANSFORM,
    CLEARCOAT_ROUGHNESS_UV_TRANSFORM,
    CLEARCOAT_NORMAL_UV_TRANSFORM,
    IRIDESCENCE_UV_TRANSFORM,
    IRIDESCENCE_THICKNESS_UV_TRANSFORM,
    ANISOTROPY_UV_TRANSFORM,
    TRANSMISSION_UV_TRANSFORM,
};

enum eMaterialProperties : uint64_t {
    PROP_TYPE                           = 1 << 0,
    PROP_TOPOLOGY_TYPE                  = 1 << 1,
    PROP_CULL_TYPE                      = 1 << 2,
    PROP_PRIORITY                       = 1 << 3,
    PROP_DEPTH_READ                     = 1 << 4,
    PROP_DEPTH_WRITE                    = 1 << 5,
    PROP_FRAGMENT_WRITE                 = 1 << 6,
    PROP_USE_SKINNING                   = 1 << 7,
    PROP_SHADER                         = 1 << 8,
    PROP_COLOR                          = 1 << 9,
    PROP_DIFFUSE_TEXTURE                = 1 << 10,
    PROP_OCLUSSION_ROUGHNESS_METALLIC   = 1 << 11,
    PROP_METALLIC_ROUGHNESS_TEXTURE     = 1 << 12,
    PROP_OCLUSSION_TEXTURE              = 1 << 13,
    PROP_EMISSIVE                       = 1 << 14,
    PROP_EMISSIVE_TEXTURE               = 1 << 15,
    PROP_NORMAL_TEXTURE                 = 1 << 16,
    PROP_NORMAL_SCALE                   = 1 << 17,
    PROP_TRANSPARENCY_TYPE              = 1 << 18,
    PROP_ALPHA_MASK                     = 1 << 19,
    PROP_CLEARCOAT                      = 1 << 20,
    PROP_CLEARCOAT_TEXTURE              = 1 << 21,
    PROP_CLEARCOAT_ROUGHNESS_TEXTURE    = 1 << 22,
    PROP_CLEARCOAT_NORMAL_TEXTURE       = 1 << 23,
    PROP_CLEARCOAT_TOGGLE               = 1 << 24,
    PROP_IRIDESCENCE                    = 1 << 25,
    PROP_IRIDESCENCE_TEXTURE            = 1 << 26,
    PROP_IRIDESCENCE_THICKNESS_TEXTURE  = 1 << 27,
    PROP_IRIDESCENCE_TOGGLE             = 1 << 28,
    PROP_ANISOTROPY                     = 1 << 29,
    PROP_ANISOTROPY_TEXTURE             = 1 << 30,
    PROP_ANISOTROPY_TOGGLE              = 1 << 31,
    PROP_TRANSMISSION                   = 1 << 32,
    PROP_TRANSMISSION_TEXTURE           = 1 << 33,
    PROP_TRANSMISSION_TOGGLE            = 1 << 34,

    PROP_RELOAD_NEEDED                  = PROP_TYPE | PROP_TOPOLOGY_TYPE | PROP_CULL_TYPE | PROP_DEPTH_READ | PROP_DEPTH_WRITE | PROP_FRAGMENT_WRITE | PROP_DIFFUSE_TEXTURE |
                                        PROP_METALLIC_ROUGHNESS_TEXTURE | PROP_OCLUSSION_TEXTURE | PROP_EMISSIVE_TEXTURE | PROP_NORMAL_TEXTURE | PROP_TRANSPARENCY_TYPE |
                                        PROP_CLEARCOAT_TEXTURE | PROP_CLEARCOAT_ROUGHNESS_TEXTURE | PROP_CLEARCOAT_NORMAL_TEXTURE | PROP_CLEARCOAT_TOGGLE | PROP_IRIDESCENCE_TEXTURE |
                                        PROP_IRIDESCENCE_THICKNESS_TEXTURE | PROP_IRIDESCENCE_TOGGLE | PROP_ANISOTROPY_TEXTURE | PROP_ANISOTROPY_TOGGLE,

    PROP_UPDATE_NEEDED                  = PROP_COLOR | PROP_OCLUSSION_ROUGHNESS_METALLIC | PROP_EMISSIVE | PROP_NORMAL_SCALE | PROP_ALPHA_MASK | PROP_CLEARCOAT | PROP_IRIDESCENCE |
                                        PROP_ANISOTROPY
};

class Material : public Resource
{

public:

    Material();
    ~Material();

    void set_color(const glm::vec4& color);
    void set_roughness(float roughness);
    void set_metallic(float metallic);
    void set_occlusion(float occlusion);
    void set_normal_scale(float normal_scale);
    void set_emissive(const glm::vec3& emissive);
    void set_clearcoat_factor(float new_clearcoat_factor);
    void set_clearcoat_roughness(float new_clearcoat_roughness);
    void set_iridescence_factor(float new_iridescence_factor);
    void set_iridescence_ior(float new_iridescence_ior);
    void set_iridescence_thickness_min(float new_iridescence_thickness_min);
    void set_iridescence_thickness_max(float new_iridescence_thickness_max);
    void set_anisotropy_factor(float new_anisotropy_factor);
    void set_anisotropy_rotation(float new_anisotropy_rotation);
    void set_transmission_factor(float new_transmission_factor);
    void set_uv_transform(uint8_t index, const glm::mat3x3& matrix);

    void set_diffuse_texture(Texture* diffuse_texture);
    void set_metallic_roughness_texture(Texture* metallic_roughness_texture);
    void set_normal_texture(Texture* normal_texture);
    void set_emissive_texture(Texture* emissive_texture);
    void set_occlusion_texture(Texture* occlusion_texture);
    void set_clearcoat_texture(Texture* clearcoat_texture);
    void set_clearcoat_roughness_texture(Texture* clearcoat_roughness_texture);
    void set_clearcoat_normal_texture(Texture* clearcoat_normal_texture);
    void set_iridescence_texture(Texture* iridescence_texture);
    void set_iridescence_thickness_texture(Texture* iridescence_thickness_texture);
    void set_anisotropy_texture(Texture* anisotropy_texture);
    void set_transmission_texture(Texture* transmission_texture);

    void set_alpha_mask(float alpha_mask);
    void set_depth_read_write(bool value);
    void set_depth_read(bool depth_read);
    void set_depth_write(bool depth_write);
    void set_use_skinning(bool use_skinning);
    void set_is_2D(bool is_2D);
    void set_fragment_write(bool fragment_write);

    void set_transparency_type(eTransparencyType transparency_type);
    void set_topology_type(eTopologyType topology_type);
    void set_cull_type(eCullType cull_type);
    void set_type(eMaterialType type);
    void set_priority(uint8_t priority);

    void set_use_tangents(bool value);
    void set_use_clearcoat(bool value);
    void set_use_iridescence(bool value);
    void set_use_anisotropy(bool value);
    void set_use_transmission(bool value);

    void set_shader(Shader* shader);
    void set_shader_pipeline(Pipeline* pipeline);

    glm::vec4 get_color() const;
    float get_roughness() const;
    float get_metallic() const;
    float get_occlusion() const;
    float get_normal_scale() const;
    glm::vec3 get_emissive() const;
    float get_clearcoat_factor() const;
    float get_clearcoat_roughness() const;
    float get_iridescence_factor() const;
    float get_iridescence_ior() const;
    float get_iridescence_thickness_min() const;
    float get_iridescence_thickness_max() const;
    float get_anisotropy_factor() const;
    float get_anisotropy_rotation() const;
    float get_transmission_factor() const;
    const std::vector<glm::mat4x4>& get_uv_transforms() const;

    const Texture* get_diffuse_texture() const;
    const Texture* get_metallic_roughness_texture() const;
    const Texture* get_normal_texture() const;
    const Texture* get_emissive_texture() const;
    const Texture* get_occlusion_texture() const;
    const Texture* get_clearcoat_texture() const;
    const Texture* get_clearcoat_roughness_texture() const;
    const Texture* get_clearcoat_normal_texture() const;
    const Texture* get_iridescence_texture() const;
    const Texture* get_iridescence_thickness_texture() const;
    const Texture* get_anisotropy_texture() const;
    const Texture* get_transmission_texture() const;

    Texture* get_diffuse_texture();
    Texture* get_metallic_roughness_texture();
    Texture* get_normal_texture();
    Texture* get_emissive_texture();
    Texture* get_occlusion_texture();
    Texture* get_clearcoat_texture();
    Texture* get_clearcoat_roughness_texture();
    Texture* get_clearcoat_normal_texture();
    Texture* get_iridescence_texture();
    Texture* get_iridescence_thickness_texture();
    Texture* get_anisotropy_texture();
    Texture* get_transmission_texture();

    float get_alpha_mask() const;

    bool get_depth_read() const;
    bool get_depth_write() const;
    bool get_use_skinning() const;
    bool get_is_2D() const;
    bool get_fragment_write() const;
    bool get_use_uv_transforms() const;

    eTransparencyType get_transparency_type() const;
    eTopologyType get_topology_type() const;
    eCullType get_cull_type() const;
    eMaterialType get_type() const;
    uint8_t get_priority() const;

    const Shader* get_shader() const;
    Shader* get_shader_ref();

    void set_dirty_flag(eMaterialProperties property_flag);
    void reset_dirty_flags();
    uint32_t get_dirty_flags() const;

    bool has_tangents() const { return use_tangents; }
    bool has_anisotropy() const { return use_anisotropy; }
    bool has_iridescence() const { return use_iridescence; }
    bool has_clearcoat() const { return use_clearcoat; }
    bool has_transmission() const { return use_transmission; }

    void render_gui();

private:

    uint32_t dirty_flags = 0;

    Shader* shader = nullptr;

    Texture* diffuse_texture = nullptr;
    Texture* metallic_roughness_texture = nullptr;
    Texture* normal_texture = nullptr;
    Texture* emissive_texture = nullptr;
    Texture* occlusion_texture = nullptr;
    Texture* clearcoat_texture = nullptr;
    Texture* clearcoat_roughness_texture = nullptr;
    Texture* clearcoat_normal_texture = nullptr;
    Texture* iridescence_texture = nullptr;
    Texture* iridescence_thickness_texture = nullptr;
    Texture* anisotropy_texture = nullptr;
    Texture* transmission_texture = nullptr;

    std::vector<glm::mat4x4> uv_transforms;

    glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    float roughness = 1.0f;
    float metallic = 0.0f;
    float occlusion = 1.0f;
    glm::vec3 emissive = {};
    float alpha_mask = 0.5f;
    float normal_scale = 1.0f;

    float clearcoat_factor = 0.0f;
    float clearcoat_roughness = 0.0f;

    float iridescence_factor = 0.0f;
    float iridescence_ior = 1.3f;
    float iridescence_thickness_min = 100.0f;
    float iridescence_thickness_max = 400.0f; // nanometers

    float anisotropy_factor = 0.0f;
    float anisotropy_rotation = 0.0f;

    float transmission_factor = 0.0f;

    bool use_tangents = false;
    bool use_clearcoat = false;
    bool use_iridescence = false;
    bool use_anisotropy = false;
    bool use_transmission = false;

    bool depth_read = true;
    bool depth_write = true;
    bool use_skinning = false;
    bool is_2D = false;
    bool fragment_write = true;
    bool use_uv_transforms = false;

    eTransparencyType transparency_type = ALPHA_OPAQUE;
    eTopologyType topology_type = TOPOLOGY_TRIANGLE_LIST;
    eCullType cull_type = CULL_BACK;

    eMaterialType type = MATERIAL_PBR;
    uint8_t priority = 10;
};
