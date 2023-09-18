#include "framework/scene/parse_scene.h"

#include "parse_obj.h"
#include "parse_gltf.h"

#include <iostream>

EntityMesh* parse_scene(const std::string& scene_path)
{
    std::string extension = scene_path.substr(scene_path.find_last_of(".") + 1);

    std::cout << "Parsing scene: " << scene_path << std::endl;

    if (extension == "obj") {
        return parse_obj(scene_path);
    }
    else if (extension == "gltf") {
        return parse_gltf(scene_path);
    }
    else {
        std::cerr << "Scene extension ." << extension << " not supported" << std::endl;
        assert(0);
    }

    return nullptr;
}
