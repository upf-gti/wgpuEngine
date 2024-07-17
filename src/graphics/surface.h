#pragma once

#include "graphics/webgpu_context.h"
#include "framework/math/aabb.h"
#include "material.h"

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include <string>

namespace normals {
    const glm::vec3 pX = glm::vec3(1.f, 0.f, 0.0);
    const glm::vec3 nX = glm::vec3(-1.f, 0.f, 0.0);
    const glm::vec3 pY = glm::vec3(0.f, 1.f, 0.0);
    const glm::vec3 nY = glm::vec3(0.f, -1.f, 0.0);
    const glm::vec3 pZ = glm::vec3(0.f, 0.f, 1.0);
    const glm::vec3 nZ = glm::vec3(0.f, 0.f, -1.0);
}

struct InterleavedData {
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 color = glm::vec3(1.0f);
    glm::vec4 weights;
    glm::ivec4 joints;
};

class Surface
{
    std::vector<InterleavedData> vertices;
    Material material;

    WGPUBuffer vertex_buffer = nullptr;
    static Surface* quad_mesh;

    std::vector<InterleavedData> generate_quad(float w = 1.f, float h = 1.f, const glm::vec3& position = { 0.f, 0.f, 0.f }, const glm::vec3& normal = { 0.f, 1.f, 0.f }, const glm::vec3& color = { 1.f, 1.f, 1.f });

    AABB aabb;

    uint32_t index;

    std::string name = "";
public:

    ~Surface();

    static WebGPUContext* webgpu_context;

    void set_material_color(const glm::vec4& color);
    void set_material_diffuse(Texture* diffuse);
    void set_material_shader(Shader* shader);
    void set_material_type(eMaterialType type);
    void set_material_priority(uint8_t priority);
    void set_material_transparency_type(eTransparencyType transparency_type);
    void set_material_cull_type(eCullType cull_type);
    void set_material_topology_type(eTopologyType topology_type);
    void set_material_depth_read(bool depth_read);
    void set_material_depth_write(bool depth_write);
    void set_material_is_2d(bool is_2d);
    void set_name(const std::string& new_name) { name = new_name; };
    void set_index(uint32_t new_index) { index = new_index; };

    std::vector<InterleavedData>& get_vertices() { return vertices; }
    Material& get_material();
    const Material& get_material() const;
    const WGPUBuffer& get_vertex_buffer() const;
    uint32_t get_index() const { return index; };

    void create_axis(float s = 1.f);
    void create_quad(float w = 1.f, float h = 1.f, bool centered = true, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_subvidided_quad(float w = 1.f, float h = 1.f, uint32_t subdivisions = 16, bool centered = true, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_box(float w = 1.f, float h = 1.f, float d = 1.f, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_rounded_box(float w = 1.f, float h = 1.f, float d = 1.f, float c = 0.2f, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_sphere(float r = 1.f, uint32_t segments = 32, uint32_t rings = 32, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_cone(float r = 1.f, float h = 1.f, uint32_t segments = 32, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_cylinder(float r = 1.f, float h = 1.f, uint32_t segments = 32, bool capped = true, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_capsule(float r = 1.f, float h = 1.f, uint32_t segments = 32, uint32_t rings = 8, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_torus(float r = 1.f, float ir = 0.2f, uint32_t segments_section = 32, uint32_t segments_circle = 32, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_skybox();

    void create_from_vertices(const std::vector<InterleavedData>& _vertices);

    void update_vertex_buffer(const std::vector<InterleavedData>& _vertices);
    void create_vertex_buffer();

    void* data();
    uint32_t get_vertex_count() const;
    uint64_t get_byte_size() const;
    const std::string& get_name() { return name; };

    AABB get_aabb();
    void set_aabb(AABB aabb);
};
