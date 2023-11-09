#include "framework/scene/parse_scene.h"

#include "parse_obj.h"
#include "parse_gltf.h"

#include <iostream>

void parse_scene(const std::string& scene_path, std::vector<Entity*>& entities)
{
    std::string extension = scene_path.substr(scene_path.find_last_of(".") + 1);

    spdlog::info("Parsing scene: {}", scene_path);

    if (extension == "obj") {
        entities.push_back(parse_obj(scene_path));
    }
    else if (extension == "gltf") {
        parse_gltf(scene_path, entities);
    }
    else {
        spdlog::error("Scene extension .{} not supported", extension);
        assert(0);
    }
}

EntityMesh* parse_mesh(const std::string& mesh_path)
{
    std::string extension = mesh_path.substr(mesh_path.find_last_of(".") + 1);

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
