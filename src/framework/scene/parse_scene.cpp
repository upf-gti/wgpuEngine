#include "framework/scene/parse_scene.h"

#include "parse_obj.h"
#include "parse_gltf.h"

#include "framework/nodes/mesh_instance_3d.h"

#include "spdlog/spdlog.h"

bool parse_scene(const char* scene_path, std::vector<Node3D*>& entities)
{
    std::string scene_path_str = std::string(scene_path);
    std::string extension = scene_path_str.substr(scene_path_str.find_last_of(".") + 1);

    spdlog::info("Parsing scene: {}", scene_path);

    if (extension == "obj") {
        entities.push_back(parse_obj(scene_path));
        return true;
    }
    else if (extension == "gltf" || extension == "glb") {
        return parse_gltf(scene_path, entities);
    }
    else {
        spdlog::error("Scene extension .{} not supported", extension);
        assert(0);
    }

    return false;
}

MeshInstance3D* parse_mesh(const char* mesh_path)
{
    std::string mesh_path_str = std::string(mesh_path);
    std::string extension = mesh_path_str.substr(mesh_path_str.find_last_of(".") + 1);

    spdlog::info("Parsing mesh: {}", mesh_path);

    if (extension == "obj") {
        return parse_obj(mesh_path);
    }
    else {
        spdlog::error("Mesh extension .{} not supported", extension);
        assert(0);
    }

    return {};
}
