#pragma once

#include "parser.h"

#include "framework/utils/hash.h"

class Node3D;
class Texture;
class Surface;

struct GltfPrimitive {
    std::map<std::string, int> attributes;
    int material = -1;
    int indices = -1;
    int mode = -1;
};

template <>
struct std::hash<GltfPrimitive>
{
    std::size_t operator()(const GltfPrimitive& k) const
    {
        using std::size_t;
        using std::hash;
        using std::string;

        std::vector<std::size_t> hs;

        hs.push_back(hash<int>()(k.material));
        hs.push_back(hash<int>()(k.indices));
        hs.push_back(hash<int>()(k.mode));

        for (auto& attrib : k.attributes) {
            hs.push_back(hash<int>()(attrib.second));
        }

        std::size_t seed = 0;
        hash_combine(seed, hs);
        return seed;
    }
};

namespace tinygltf {
    class Model;
}

class GltfParser : public Parser {

    Node3D* root = nullptr;

    std::map<uint32_t, Texture*> texture_cache;

    std::map<size_t, Surface*> mesh_cache;

    bool parse_model(tinygltf::Model* model, std::vector<Node*>& entities, uint32_t flags = PARSE_DEFAULT);

public:

    bool parse(const char* file_path, std::vector<Node*>& entities, uint32_t flags = PARSE_DEFAULT) override;

    bool read_data(int8_t* byte_array, uint32_t array_size, std::vector<Node*>& entities, uint32_t flags = PARSE_DEFAULT);

    void push_scene_root(Node3D* new_root) { root = new_root; };

    void clear_cache();
};
