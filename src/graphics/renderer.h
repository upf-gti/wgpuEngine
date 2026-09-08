#pragma once

#include "config_structs.h"
#include "framework/math/frustum_cull.h"
#include "graphics/pipeline.h"
#include "graphics/surface.h"
#include "graphics/uniform.h"
#include "graphics/uniforms_structs.h"
#include "includes.h"

#include "framework/camera/camera.h"

#include "glm/mat4x4.hpp"

#include "backends/imgui_impl_wgpu.h"

#include <map>
#include <string>

#define MAX_LIGHTS 32u
#define SHADOW_MAP_SIZE 1024

class Camera;
class Texture;
class Surface;
class Light3D;
class RenderdocCapture;
class RenderStorage;
class Mesh;
class MeshInstance3D;
class GSNode;
struct GLFWwindow;
struct WebGPUContext;
struct XRContext;
struct sLightUniformData;

class Renderer {
protected:
    eCameraType camera_type = CAMERA_FLYOVER;

    void render_shadow_maps();

    std::vector<sUIData> instance_ui_data;
    Uniform instance_ui_data_uniform;

    bool debug_this_frame = false;

    bool initialized = false;

public:
    virtual int post_initialize();
    virtual void clean();

    bool is_initialized() { return initialized; }

    virtual void update(float delta_time);
    virtual void render();

    void submit_global_command_encoder();

    void set_camera_params(eCameraType camera_type, const glm::vec3& camera_eye, const glm::vec3& camera_center);
    eCameraType get_camera_type();

    void add_splat_scene(GSNode* gs_scene);
    void clear_renderables();

    virtual void resize_window(int width, int height);

    void set_irradiance_texture(Texture* texture);
};
