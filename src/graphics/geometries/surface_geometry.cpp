#include "surface_geometry.h"

SurfaceGeometry::SurfaceGeometry(const glm::vec3& new_color)
    : color(new_color)
{

}

void SurfaceGeometry::set_color(const glm::vec3& new_color)
{
    color = new_color;

    build_mesh();
}
