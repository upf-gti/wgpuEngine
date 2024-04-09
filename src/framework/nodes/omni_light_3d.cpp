#include "omni_light_3d.h"

OmniLight3D::OmniLight3D() : Light3D()
{
    type = LIGHT_OMNI;
}

OmniLight3D::~OmniLight3D()
{

}

void OmniLight3D::set_range(float value)
{
    this->range = value;
}
