#pragma once

#include "includes.h"

#include "node_3d.h"

#include "graphics/uniforms_structs.h"
#include "framework/camera/camera.h"

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
    Camera light_camera;
    bool cast_shadows = false;
    float shadow_bias = 0.0001f;

    WGPUTexture shadow_depth_texture = nullptr;
    WGPUTextureView shadow_depth_texture_view = nullptr;

public:

    Light3D();
    ~Light3D();

    virtual void render() override;

    void clone(Node* new_node, bool copy = true) override;

    void render_gui() override;

    virtual void get_uniform_data(sLightUniformData& data);
    LightType get_type() const { return type; }
    float get_intensity() const { return intensity; }
    const glm::vec3& get_color() { return color; }
    glm::vec3 get_color() const { return color; }
    bool get_fading_enabled() const { return fading_enabled; }
    bool get_cast_shadows() const { return cast_shadows; }
    float get_range() const { return range; }
    float get_shadow_bias() const { return shadow_bias; }

    virtual void create_debug_meshes() = 0;

    virtual void set_color(const glm::vec3& color);
    void set_intensity(float value);
    virtual void set_range(float value);
    void set_fading_enabled(bool value);
    void set_cast_shadows(bool value);
    void set_shadow_bias(float new_shadow_bias);

    void create_shadow_data();

    void on_set_color();
    virtual void on_set_range() {};

    const Camera& get_light_camera() { return light_camera; }
    WGPUTexture get_shadow_depth_texture() { return shadow_depth_texture; }
    WGPUTextureView get_shadow_depth_texture_view() { return shadow_depth_texture_view; }

    void serialize(std::ofstream& binary_scene_file) override;
    void parse(std::ifstream& binary_scene_file) override;
};
