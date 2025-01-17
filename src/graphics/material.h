#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "framework/resources/resource.h"

#include <string>

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

enum eMaterialProperties : uint32_t {
    PROP_COLOR                          = 1 << 0,
    PROP_OCLUSSION_ROUGHNESS_METALLIC   = 1 << 1,
    PROP_EMISSIVE                       = 1 << 2,
    PROP_DIFFUSE_TEXTURE                = 1 << 3,
    PROP_METALLIC_ROUGHNESS_TEXTURE     = 1 << 4,
    PROP_NORMAL_TEXTURE                 = 1 << 5,
    PROP_EMISSIVE_TEXTURE               = 1 << 6,
    PROP_OCLUSSION_TEXTURE              = 1 << 7,
    PROP_DEPTH_READ                     = 1 << 8,
    PROP_DEPTH_WRITE                    = 1 << 9,
    PROP_USE_SKINNING                   = 1 << 10,
    PROP_TRANSPARENCY_TYPE              = 1 << 11,
    PROP_TOPOLOGY_TYPE                  = 1 << 12,
    PROP_CULL_TYPE                      = 1 << 13,
    PROP_TYPE                           = 1 << 14,
    PROP_PRIORITY                       = 1 << 15,
    PROP_ALPHA_MASK                     = 1 << 16,
    PROP_SHADER                         = 1 << 17,

    PROP_RELOAD_NEEDED                  = PROP_DIFFUSE_TEXTURE | PROP_METALLIC_ROUGHNESS_TEXTURE | PROP_NORMAL_TEXTURE | PROP_EMISSIVE_TEXTURE | PROP_TRANSPARENCY_TYPE |
                                          PROP_OCLUSSION_TEXTURE | PROP_DEPTH_READ | PROP_DEPTH_WRITE | PROP_TOPOLOGY_TYPE | PROP_CULL_TYPE | PROP_TYPE,

    PROP_UPDATE_NEEDED                  = PROP_COLOR | PROP_OCLUSSION_ROUGHNESS_METALLIC | PROP_EMISSIVE | PROP_ALPHA_MASK
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
    void set_emissive(const glm::vec3& emissive);

    void set_diffuse_texture(Texture* diffuse_texture);
    void set_metallic_roughness_texture(Texture* metallic_roughness_texture);
    void set_normal_texture(Texture* normal_texture);
    void set_emissive_texture(Texture* emissive_texture);
    void set_occlusion_texture(Texture* occlusion_texture);

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

    void set_shader(Shader* shader);
    void set_shader_pipeline(Pipeline* pipeline);

    glm::vec4 get_color() const;
    float get_roughness() const;
    float get_metallic() const;
    float get_occlusion() const;
    glm::vec3 get_emissive() const;

    const Texture* get_diffuse_texture() const;
    const Texture* get_metallic_roughness_texture() const;
    const Texture* get_normal_texture() const;
    const Texture* get_emissive_texture() const;
    const Texture* get_occlusion_texture() const;

    Texture* get_diffuse_texture();
    Texture* get_metallic_roughness_texture();
    Texture* get_normal_texture();
    Texture* get_emissive_texture();
    Texture* get_occlusion_texture();

    float get_alpha_mask() const;

    bool get_depth_read() const;
    bool get_depth_write() const;
    bool get_use_skinning() const;
    bool get_is_2D() const;
    bool get_fragment_write() const;

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

    void render_gui();

private:

    uint32_t dirty_flags = 0;

    Shader* shader = nullptr;

    Texture* diffuse_texture = nullptr;
    Texture* metallic_roughness_texture = nullptr;
    Texture* normal_texture = nullptr;
    Texture* emissive_texture = nullptr;
    Texture* occlusion_texture = nullptr;

    glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    float roughness = 1.0f;
    float metallic = 0.0f;
    float occlusion = 1.0f;
    glm::vec3 emissive = {};
    float alpha_mask = 0.5f;

    bool depth_read = true;
    bool depth_write = true;
    bool use_skinning = false;
    bool is_2D = false;
    bool fragment_write = true;

    eTransparencyType transparency_type = ALPHA_OPAQUE;
    eTopologyType topology_type = TOPOLOGY_TRIANGLE_LIST;
    eCullType cull_type = CULL_BACK;

    eMaterialType type = MATERIAL_PBR;
    uint8_t priority = 10;
};
