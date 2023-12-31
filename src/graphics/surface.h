#pragma once

#include "material.h"

class Mesh;
class EntityMesh;

struct Surface
{
    Mesh* mesh = nullptr;
    Material material;
    EntityMesh* entity_mesh_ref = nullptr;

    void set_material_color(const glm::vec4& color) { material.color = color; }
    void set_material_diffuse(Texture* diffuse) { material.diffuse_texture = diffuse; material.flags |= MATERIAL_DIFFUSE; }
    void set_material_shader(Shader* shader) { material.shader = shader; }
    void set_material_flag(eMaterialFlags flag) { material.flags |= flag; }
    void set_material_priority(uint8_t priority) { material.priority = priority; }
};
