#pragma once

#include "graphics/webgpu_context.h"
#include "framework/math/aabb.h"
#include "material.h"
#include "framework/resources/resource.h"

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

struct sSurfaceData {
    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> indices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec4> tangents;
    std::vector<glm::vec3> colors;
    std::vector<glm::vec4> weights;
    std::vector<glm::ivec4> joints;
};

struct sInterleavedData {
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 normal;
    glm::vec4 tangent;
    glm::vec3 color = glm::vec3(1.0f);
    glm::vec4 weights;
    glm::ivec4 joints;
};

class Surface : public Resource
{
    uint32_t vertex_count = 0;
    uint32_t index_count = 0;

    Material* material = nullptr;

    sSurfaceData* surface_data = nullptr; // optional when loading

    WGPUBuffer vertex_buffer = nullptr;
    WGPUBuffer index_buffer = nullptr;

    static Surface* quad_mesh;

    std::vector<sInterleavedData> generate_quad(float w = 1.f, float h = 1.f, const glm::vec3& position = { 0.f, 0.f, 0.f }, const glm::vec3& normal = { 0.f, 1.f, 0.f }, const glm::vec3& color = { 1.f, 1.f, 1.f }, bool flip_y = false);

    AABB aabb;

public:

    ~Surface();

    static WebGPUContext* webgpu_context;

    void set_material(Material* material);

    Material* get_material();
    const Material* get_material() const;

    void set_surface_data(sSurfaceData* surface_data);

    const sSurfaceData* get_surface_data() const;
    sSurfaceData* get_surface_data();

    const WGPUBuffer& get_vertex_buffer() const;
    const WGPUBuffer& get_index_buffer() const;

    void create_axis(float s = 1.f);
    void create_quad(float w = 1.f, float h = 1.f, bool flip_y = false, bool centered = true, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_subdivided_quad(float w = 1.f, float h = 1.f, uint32_t subdivisions = 16, bool centered = true, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_box(float w = 1.f, float h = 1.f, float d = 1.f, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_rounded_box(float w = 1.f, float h = 1.f, float d = 1.f, float c = 0.2f, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_sphere(float r = 1.f, uint32_t segments = 32, uint32_t rings = 32, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_cone(float r = 1.f, float h = 1.f, uint32_t segments = 32, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_cylinder(float r = 1.f, float h = 1.f, uint32_t segments = 32, bool capped = true, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_capsule(float r = 1.f, float h = 1.f, uint32_t segments = 32, uint32_t rings = 8, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_torus(float r = 1.f, float ir = 0.2f, uint32_t segments_section = 32, uint32_t segments_circle = 32, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_circle(float radius = 1.f, uint32_t segments = 32);
    void create_arrow();
    void create_skybox();

    void create_from_vertices(const std::vector<sInterleavedData>& _vertices);

    void update_vertex_buffer(const std::vector<sInterleavedData>& _vertices);
    void create_vertex_buffer(const std::vector<sInterleavedData>& _vertices);

    void create_index_buffer(const std::vector<uint32_t>& indices);

    uint32_t get_vertex_count() const;
    uint64_t get_vertices_byte_size() const;

    uint32_t get_index_count() const;
    uint64_t get_indices_byte_size() const;

    void render_gui();

    AABB get_aabb() const;
    void set_aabb(AABB aabb);
};
