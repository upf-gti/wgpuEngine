#include "spot_light_3d.h"

SpotLight3D::SpotLight3D() : Light3D()
{
    type = LIGHT_SPOT;
}

SpotLight3D::~SpotLight3D()
{
    
}

void SpotLight3D::set_range(float value)
{
    this->range = value;
}

void SpotLight3D::set_angle(float value)
{
    this->angle = value;
}
