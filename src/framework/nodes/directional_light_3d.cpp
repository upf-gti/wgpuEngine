#include "directional_light_3d.h"

DirectionalLight3D::DirectionalLight3D() : Light3D()
{
    type = LIGHT_DIRECTIONAL;
    node_type = "DirectionalLight3D";
    name = node_type + "_" + std::to_string(last_node_id++);
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
        .direction = -get_global_model()[2]
    };
}
