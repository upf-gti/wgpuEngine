#include "framework/parsers/parse_scene.h"

#include "parse_obj.h"
#include "parse_gltf.h"
#include "parse_vdb.h"
#include "parse_ply.h"

#include "framework/nodes/mesh_instance_3d.h"

#include "spdlog/spdlog.h"

bool parse_scene(const char* scene_path, std::vector<Node*>& entities, bool fill_surface_data)
{
    std::string scene_path_str = std::string(scene_path);
    std::string extension = scene_path_str.substr(scene_path_str.find_last_of(".") + 1);

    spdlog::info("Parsing scene: {}", scene_path);

    if (extension == "obj") {
        entities.push_back(parse_obj(scene_path));
        return true;
    }
    else if (extension == "gltf" || extension == "glb") {
        return parse_gltf(scene_path, entities, fill_surface_data);
    }
    else if (extension == "vdb") {
        spdlog::info("Parsing a VDB file (WIP)");
        parse_vdb(scene_path, entities);
    }
    else if (extension == "ply") {
        spdlog::info("Parsing a PLY file");
        parse_ply(scene_path, entities);
    }
    else {
        spdlog::error("Scene extension .{} not supported", extension);
        assert(0);
    }

    return false;
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
