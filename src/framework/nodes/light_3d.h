#pragma once

#include "includes.h"

#include "node_3d.h"

#include "graphics/uniforms_structs.h"

enum LightType {
    LIGHT_UNDEFINED,
    LIGHT_DIRECTIONAL,
    LIGHT_OMNI,
    LIGHT_SPOT
};

class Light3D : public Node3D {

protected:

    LightType   type = LIGHT_UNDEFINED;

    float       intensity = 1.0f;
    glm::vec3   color = { 1.0f, 1.0f, 1.0f };
    float       range = 5.0f;

    // Fading

    bool fading_enabled = false;
    float fade_begin = 10.0f;
    float fade_length = 1.0f;

    // Shadows
    bool cast_shadows = false;
    float shadow_bias = 0.001f;

    WGPUTexture shadow_depth_texture = nullptr;
    WGPUTextureView shadow_depth_texture_view = nullptr;

public:

    Light3D();
    ~Light3D();

    virtual void render() override;

    void clone(Node* new_node, bool copy = true) override;

    void render_gui() override;

    virtual sLightUniformData get_uniform_data() = 0;
    LightType get_type() { return type; };
    float get_intensity() { return intensity; };
    const glm::vec3& get_color() { return color; };
    bool get_fading_enabled() { return fading_enabled; };
    bool get_cast_shadows() { return cast_shadows; };
    float get_range() { return range; };

    virtual void create_debug_meshes() = 0;

    virtual void set_color(const glm::vec3& color);
    void set_intensity(float value);
    virtual void set_range(float value);
    void set_fading_enabled(bool value);
    void set_cast_shadows(bool value);

    void create_shadow_data();

    void on_set_color();

    WGPUTexture get_shadow_depth_texture() { return shadow_depth_texture; }
    WGPUTextureView get_shadow_depth_texture_view() { return shadow_depth_texture_view; }

    void serialize(std::ofstream& binary_scene_file) override;
    void parse(std::ifstream& binary_scene_file) override;
};
