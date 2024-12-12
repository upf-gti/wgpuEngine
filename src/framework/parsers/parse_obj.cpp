#include "parse_obj.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "graphics/texture.h"
#include "graphics/shader.h"
#include "graphics/renderer_storage.h"

#include "framework/nodes/mesh_instance_3d.h"

#include "shaders/mesh_forward.wgsl.gen.h"

#include <filesystem>

#include "spdlog/spdlog.h"

void parse_obj(const char* obj_path, MeshInstance3D* entity_mesh, bool create_aabb)
{
    if (!entity_mesh) {
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

    entity_mesh->set_name(obj_path_fs.stem().string());

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {

        Surface* new_surface = new Surface();

        new_surface->set_name(shapes[s].name);

        Material* material = new_surface->get_material();

        if (!material) {

            material = new Material();

            if (!materials.empty()) {
                uint32_t material_id = shapes[s].mesh.material_ids[0] == -1 ? 0 : shapes[s].mesh.material_ids[0];
                if (materials[material_id].diffuse_texname.empty()) {
                    material->set_color(glm::vec4(materials[material_id].diffuse[0], materials[material_id].diffuse[1], materials[material_id].diffuse[2], 1.0f));
                }
                else {
                    material->set_diffuse_texture(RendererStorage::get_texture(obj_path_fs.parent_path().string() + "/" + materials[material_id].diffuse_texname, TEXTURE_STORAGE_SRGB));
                }
            }

            material->set_shader(RendererStorage::get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, material));

            new_surface->set_material(material);
        }

        entity_mesh->add_surface(new_surface);

        sSurfaceData vertices;

        glm::vec3 min_pos = { FLT_MAX, FLT_MAX, FLT_MAX };
        glm::vec3 max_pos = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {

                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                vertices.vertices.push_back({
                    attrib.vertices[3 * size_t(idx.vertex_index) + 0],
                    attrib.vertices[3 * size_t(idx.vertex_index) + 1],
                    attrib.vertices[3 * size_t(idx.vertex_index) + 2]
                });

                if (idx.normal_index >= 0) {
                    vertices.normals.push_back({
                        attrib.normals[3 * size_t(idx.normal_index) + 0],
                        attrib.normals[3 * size_t(idx.normal_index) + 1],
                        attrib.normals[3 * size_t(idx.normal_index) + 2]
                    });
                }

                if (idx.texcoord_index >= 0) {
                    vertices.uvs.push_back({
                        attrib.texcoords[2 * size_t(idx.texcoord_index) + 0],
                        1.0f - attrib.texcoords[2 * size_t(idx.texcoord_index) + 1],
                    });
                }

                vertices.colors.push_back({
                    attrib.colors[3 * size_t(idx.vertex_index) + 0],
                    attrib.colors[3 * size_t(idx.vertex_index) + 1],
                    attrib.colors[3 * size_t(idx.vertex_index) + 2]
                });

                const glm::vec3& last_position = vertices.vertices.back();

                min_pos = glm::min(last_position, min_pos);
                max_pos = glm::max(last_position, max_pos);
            }

            index_offset += fv;
        }

        if (create_aabb) {
            AABB aabb;
            aabb.half_size = (max_pos - min_pos) * glm::vec3(0.5);
            aabb.center = max_pos - aabb.half_size;

            entity_mesh->set_aabb(aabb);
            new_surface->set_aabb(aabb);
        }

        new_surface->create_surface_data(vertices);
    }

    AABB entity_aabb;
    for (const Surface* surface : entity_mesh->get_surfaces()) {
        const AABB& surface_aabb = surface->get_aabb();

        entity_aabb = merge_aabbs(entity_aabb, surface_aabb);
    }

    entity_mesh->set_aabb(entity_aabb);
}

MeshInstance3D* parse_obj(const char* obj_path, bool create_aabb)
{
    MeshInstance3D* new_entity = new MeshInstance3D();

    parse_obj(obj_path, new_entity, create_aabb);

    return new_entity;
}
