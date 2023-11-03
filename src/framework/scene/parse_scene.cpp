#include "framework/scene/parse_scene.h"

#include "parse_obj.h"
#include "parse_gltf.h"

#include <iostream>

EntityMesh* parse_scene(const std::string& scene_path)
{
    std::string extension = scene_path.substr(scene_path.find_last_of(".") + 1);

    spdlog::info("Parsing scene: {}", scene_path);

    if (extension == "obj") {
        return parse_obj(scene_path);
    }
    else if (extension == "gltf") {
        return parse_gltf(scene_path);
    }
    else {
        spdlog::error("Scene extension .{} not supported", extension);
        assert(0);
    }

    return nullptr;
}
