#include "light_3d.h"

Light3D::Light3D()
{
}

Light3D::~Light3D()
{
    
}


void Light3D::set_color(glm::vec3 color)
{
    this->color = color;
}

void Light3D::set_intensity(float value)
{
    this->intensity = value;
}

void Light3D::set_fading_enabled(bool value)
{
    this->fading_enabled = value;
}

void Light3D::set_cast_shadows(bool value)
{
    this->cast_shadows = value;
}
