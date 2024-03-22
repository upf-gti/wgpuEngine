#include "parse_obj.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "graphics/texture.h"
#include "graphics/shader.h"
#include "graphics/renderer_storage.h"

#include "framework/nodes/mesh_instance_3d.h"

#include <filesystem>

#include "spdlog/spdlog.h"

void parse_obj(const char* obj_path, MeshInstance3D* entity)
{
    if (!entity) {
        return;
    }
        
    tinyobj::ObjReaderConfig reader_config;
    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(obj_path, reader_config)) {
        if (!reader.Error().empty()) {
            spdlog::error("TinyObjReader: {}", reader.Error());
        }
        return;
    }

    if (!reader.Warning().empty()) {
        spdlog::warn("TinyObjReader: {}", reader.Warning());
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    std::filesystem::path obj_path_fs = std::filesystem::path(obj_path);

    entity->set_name(obj_path_fs.stem().string());

    Surface* new_surface = RendererStorage::get_surface(obj_path);

    if (!materials.empty()) {
        if (materials[0].diffuse_texname.empty()) {
            new_surface->set_material_color(glm::vec4(materials[0].diffuse[0], materials[0].diffuse[1], materials[0].diffuse[2], 1.0f));
            new_surface->set_material_shader(RendererStorage::get_shader("data/shaders/mesh_color.wgsl", new_surface->get_material()));
        }
        else {
            new_surface->set_material_diffuse((RendererStorage::get_texture(obj_path_fs.parent_path().string() + "/" + materials[0].diffuse_texname)));
            new_surface->set_material_flag(MATERIAL_DIFFUSE);
            new_surface->set_material_shader(RendererStorage::get_shader("data/shaders/mesh_texture.wgsl", new_surface->get_material()));
        }
    }

    entity->add_surface(new_surface);

    auto& vertices = new_surface->get_vertices();

    // Mesh already loaded
    if (!vertices.empty()) { return; }

    InterleavedData vertex_data;

    glm::vec3 min_pos = { FLT_MAX, FLT_MAX, FLT_MAX };
    glm::vec3 max_pos = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {

                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                vertex_data.position.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                vertex_data.position.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                vertex_data.position.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

                if (idx.normal_index >= 0) {
                    vertex_data.normal.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    vertex_data.normal.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    vertex_data.normal.z = attrib.normals[3 * size_t(idx.normal_index) + 2];
                }

                if (idx.texcoord_index >= 0) {
                    vertex_data.uv.x = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    vertex_data.uv.y = 1.0f - attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                }

                vertex_data.color.x = attrib.colors[3 * size_t(idx.vertex_index) + 0];
                vertex_data.color.y = attrib.colors[3 * size_t(idx.vertex_index) + 1];
                vertex_data.color.z = attrib.colors[3 * size_t(idx.vertex_index) + 2];

                vertices.push_back(vertex_data);

                glm::bvec3 less_than = glm::lessThan(vertex_data.position, min_pos);

                min_pos.x = less_than.x ? vertex_data.position.x : min_pos.x;
                min_pos.y = less_than.y ? vertex_data.position.y : min_pos.y;
                min_pos.z = less_than.z ? vertex_data.position.z : min_pos.z;

                glm::bvec3 greater_than = glm::greaterThan(vertex_data.position, max_pos);

                max_pos.x = greater_than.x ? vertex_data.position.x : max_pos.x;
                max_pos.y = greater_than.y ? vertex_data.position.y : max_pos.y;
                max_pos.z = greater_than.z ? vertex_data.position.z : max_pos.z;
            }

            index_offset += fv;
        }
    }

    AABB aabb;
    aabb.center = (max_pos - min_pos) * glm::vec3(0.5);
    aabb.half_size = max_pos - aabb.center;

    entity->set_aabb(aabb);

    new_surface->create_vertex_buffer();
}

MeshInstance3D* parse_obj(const char* obj_path)
{
    MeshInstance3D* new_entity = new MeshInstance3D();

    parse_obj(obj_path, new_entity);

    return new_entity;
}
