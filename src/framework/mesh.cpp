#include "mesh.h"

void Mesh::create_quad()
{
    vertices = {
      // position   uv
        -1.0, 1.0,  0.0, 1.0,
        -1.0,-1.0,  0.0, 0.0,
         1.0,-1.0,  1.0, 0.0,

        -1.0, 1.0,  0.0, 1.0,
         1.0,-1.0,  1.0, 0.0,
         1.0, 1.0,  1.0, 1.0
    };
}

void* Mesh::data()
{
    return vertices.data();
}

size_t Mesh::get_size()
{
    return vertices.size();
}

void Mesh::destroy()
{
    vertices.clear();
}