#include "framework/parsers/parse_scene.h"

#include "parse_obj.h"
#include "parse_gltf.h"
#include "parse_vdb.h"
#include "parse_ply.h"

#include "framework/nodes/mesh_instance_3d.h"

#include "spdlog/spdlog.h"

bool parse_scene(const char* scene_path, std::vector<Node*>& entities, bool fill_surface_data, Node3D* root)
{
    std::string scene_path_str = std::string(scene_path);
    std::string extension = scene_path_str.substr(scene_path_str.find_last_of(".") + 1);

    spdlog::info("Parsing scene: {}", scene_path);

    if (extension == "obj") {
        entities.push_back(parse_obj(scene_path));
        return true;
    }

    Parser* parser = nullptr;
    uint32_t flags = 0u;

    if (extension == "gltf" || extension == "glb") {
        spdlog::info("Parsing a GLTF/GLB file");
        parser = new GltfParser();
        static_cast<GltfParser*>(parser)->push_scene_root(root);
        if (fill_surface_data) {
            flags |= PARSE_GLTF_FILL_SURFACE_DATA;
        }
    }
    else if (extension == "vdb") {
        spdlog::info("Parsing a VDB file (WIP)");
        parser = new VdbParser();
    }
    else if (extension == "ply") {
        spdlog::info("Parsing a PLY file");
        parser = new PlyParser();
    }
    else {
        spdlog::error("Scene extension .{} not supported", extension);
        assert(0);
        return false;
    }

    bool res = parser->parse(scene_path, entities, flags);

    delete parser;

    return res;
}

MeshInstance3D* parse_mesh(const char* mesh_path, bool create_aabb, bool fill_surface_data)
{
    std::string mesh_path_str = std::string(mesh_path);
    std::string extension = mesh_path_str.substr(mesh_path_str.find_last_of(".") + 1);

    spdlog::trace("Parsing mesh: {}", mesh_path);

    if (extension == "obj") {
        return parse_obj(mesh_path, create_aabb);
    }
    else {
        spdlog::error("Mesh extension .{} not supported", extension);
        assert(0);
    }

    return {};
}
