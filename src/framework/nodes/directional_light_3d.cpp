#include "directional_light_3d.h"
#include "framework/utils/utils.h"

DirectionalLight3D::DirectionalLight3D() : Light3D()
{
    type = LIGHT_DIRECTIONAL;
}

DirectionalLight3D::~DirectionalLight3D()
{

}

sLightUniformData DirectionalLight3D::get_uniform_data()
{
    return {
        .position = get_translation(),
        .type = type,
        .color = color,
        .intensity = intensity,
        .direction = get_front(get_model()),
    };
}
