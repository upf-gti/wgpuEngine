#include "mesh.h"

void Mesh::createQuad()
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
    return vertices.data();;
}

size_t Mesh::getSize()
{
    return vertices.size();
}
