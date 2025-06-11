#pragma once

#include "graphics/webgpu_context.h"
#include "framework/math/aabb.h"
#include "material.h"
#include "framework/resources/resource.h"

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include "mikktspace.h"

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

    uint32_t size() const;
    void resize(uint32_t size);
    void append(const sSurfaceData& surface_data);
    void clear();
};

struct sInterleavedData {
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

    sSurfaceData surface_data; // optional when loading

    WGPUBuffer vertex_pos_buffer = nullptr;
    WGPUBuffer vertex_data_buffer = nullptr;
    WGPUBuffer index_buffer = nullptr;

    static Surface* quad_mesh;

    sSurfaceData generate_quad(float w = 1.f, float h = 1.f, const glm::vec3& position = { 0.f, 0.f, 0.f }, const glm::vec3& normal = { 0.f, 1.f, 0.f }, const glm::vec3& color = { 1.f, 1.f, 1.f }, bool flip_y = false);

    AABB aabb;

public:

    ~Surface();

    static WebGPUContext* webgpu_context;

    void set_material(Material* material);

    Material* get_material() const;

    const sSurfaceData& get_surface_data() const;
    sSurfaceData& get_surface_data();

    const WGPUBuffer& get_vertex_buffer() const;
    const WGPUBuffer& get_vertex_data_buffer() const;
    const WGPUBuffer& get_index_buffer() const;

    void create_axis(float s = 1.f);
    void create_quad(float w = 1.f, float h = 1.f, bool flip_y = false, bool centered = true, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_subdivided_quad(float w = 1.f, float h = 1.f, bool flip_y = false, uint32_t subdivisions = 16, bool centered = true, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_box(float w = 1.f, float h = 1.f, float d = 1.f, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_rounded_box(float w = 1.f, float h = 1.f, float d = 1.f, float c = 0.2f, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_sphere(float r = 1.f, uint32_t segments = 32, uint32_t rings = 32, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_cone(float r = 1.f, float h = 1.f, uint32_t segments = 32, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_cylinder(float r = 1.f, float h = 1.f, uint32_t segments = 32, bool capped = true, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_capsule(float r = 1.f, float h = 1.f, uint32_t segments = 32, uint32_t rings = 8, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_torus(float ring_radius = 1.f, float tube_radius = 0.2f, uint32_t rings = 64u, uint32_t ring_segments = 32u, const glm::vec3& color = { 1.f, 1.f, 1.f });
    void create_circle(float radius = 1.f, uint32_t segments = 32);
    void create_arrow();
    void create_skybox();

    std::vector<sInterleavedData> create_interleaved_data(const sSurfaceData& vertices_data);

    void update_vertex_buffer(const std::vector<glm::vec3>& vertices);
    void update_surface_data(const sSurfaceData& vertices_data, bool store_data = false);
    void create_surface_data(const sSurfaceData& vertices_data, bool store_data = false);
    void set_surface_data(const sSurfaceData& vertices_data);

    void create_index_buffer(const std::vector<uint32_t>& indices);

    uint32_t get_vertex_count() const;
    uint64_t get_vertices_byte_size() const;
    uint64_t get_interleaved_data_byte_size() const;

    // Tangent generation
    static int mikkt_get_num_faces(const SMikkTSpaceContext* pContext);
    static int mikkt_get_num_vertices_of_face(const SMikkTSpaceContext* pContext, const int iFace);
    static void mikkt_get_position(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert);
    static void mikkt_get_normal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert);
    static void mikkt_get_tex_coord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert);
    static void mikkt_set_tspace_basic(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert);

    bool generate_tangents(sSurfaceData* vertices_data);

    uint32_t get_index_count() const;
    uint64_t get_indices_byte_size() const;

    virtual void render_gui();

    AABB get_aabb() const;
    void set_aabb(const AABB& aabb);
    void update_aabb(std::vector<glm::vec3>& vertices);
};
