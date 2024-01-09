#include "parse_obj.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "graphics/texture.h"
#include "graphics/shader.h"
#include "graphics/renderer_storage.h"

#include "spdlog/spdlog.h"

EntityMesh* parse_obj(const std::string& obj_path)
{
    tinyobj::ObjReaderConfig reader_config;
    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(obj_path, reader_config)) {
        if (!reader.Error().empty()) {
            spdlog::error("TinyObjReader: {}", reader.Error());
        }
        return nullptr;
    }

    if (!reader.Warning().empty()) {
        spdlog::warn("TinyObjReader: {}", reader.Warning());
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    EntityMesh* new_entity = new EntityMesh();

    Surface* new_surface = RendererStorage::get_surface(obj_path);

    if (!materials.empty()) {
        if (materials[0].diffuse_texname.empty()) {
            new_surface->set_material_color(glm::vec4(materials[0].diffuse[0], materials[0].diffuse[1], materials[0].diffuse[2], 1.0f));
            new_surface->set_material_shader(RendererStorage::get_shader("data/shaders/mesh_color.wgsl"));
            new_surface->set_material_flag(MATERIAL_COLOR);
        }
        else {
            new_surface->set_material_diffuse(RendererStorage::get_texture("data/textures/" + materials[0].diffuse_texname));
            new_surface->set_material_shader(RendererStorage::get_shader("data/shaders/mesh_texture.wgsl"));
            new_surface->set_material_flag(MATERIAL_DIFFUSE);
        }
    }

    new_entity->add_surface(new_surface);

    auto& vertices = new_surface->get_vertices();

    // Mesh already loaded
    if (!vertices.empty()) { return new_entity; }

    InterleavedData vertex_data;

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
            }

            index_offset += fv;
        }
    }

    new_surface->create_vertex_buffer();

    return new_entity;
}
