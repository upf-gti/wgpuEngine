#include "material.h"

#include "texture.h"
#include "pipeline.h"
#include "shader.h"

Material::Material()
{
    properties["color"] = &color;
    properties["roughness"] = &roughness;
    properties["metalness"] = &metalness;
    properties["occlusion"] = &occlusion;
    properties["emissive"] = &emissive;
    properties["alpha_mask"] = &alpha_mask;
}

Material::~Material()
{
    if (diffuse_texture) {
        diffuse_texture->unref();
    }

    if (metallic_roughness_texture) {
        metallic_roughness_texture->unref();
    }

    if (normal_texture) {
        normal_texture->unref();
    }

    if (emissive_texture) {
        emissive_texture->unref();
    }

    if (occlusion_texture) {
        occlusion_texture->unref();
    }
}

void Material::set_color(const glm::vec4& color)
{
    this->color = color;
    dirty_flags |= eMaterialProperties::PROP_COLOR;
}

void Material::set_roughness(float roughness)
{
    this->roughness = roughness;
    dirty_flags |= eMaterialProperties::PROP_OCLUSSION_ROUGHNESS_METALLIC;
}

void Material::set_metalness(float metalness)
{
    this->metalness = metalness;
    dirty_flags |= eMaterialProperties::PROP_OCLUSSION_ROUGHNESS_METALLIC;
}

void Material::set_occlusion(float occlusion)
{
    this->occlusion = occlusion;
    dirty_flags |= eMaterialProperties::PROP_OCLUSSION_ROUGHNESS_METALLIC;
}

void Material::set_emissive(const glm::vec3& emissive)
{
    this->emissive = emissive;
    dirty_flags |= eMaterialProperties::PROP_EMISSIVE;
}

void Material::set_diffuse_texture(Texture* diffuse_texture)
{
    if (this->diffuse_texture != diffuse_texture) {

        if (this->diffuse_texture) {
            this->diffuse_texture->unref();
        }

        diffuse_texture->ref();
    }

    this->diffuse_texture = diffuse_texture;

    dirty_flags |= eMaterialProperties::PROP_DIFFUSE_TEXTURE;
}

void Material::set_metallic_roughness_texture(Texture* metallic_roughness_texture)
{
    if (this->metallic_roughness_texture != metallic_roughness_texture) {

        if (this->metallic_roughness_texture) {
            this->metallic_roughness_texture->unref();
        }

        metallic_roughness_texture->ref();
    }

    this->metallic_roughness_texture = metallic_roughness_texture;

    dirty_flags |= eMaterialProperties::PROP_METALLIC_ROUGHNESS_TEXTURE;
}

void Material::set_normal_texture(Texture* normal_texture)
{
    if (this->normal_texture != normal_texture) {

        if (this->normal_texture) {
            this->normal_texture->unref();
        }

        normal_texture->ref();
    }

    this->normal_texture = normal_texture;

    dirty_flags |= eMaterialProperties::PROP_NORMAL_TEXTURE;
}

void Material::set_emissive_texture(Texture* emissive_texture)
{
    if (this->emissive_texture != emissive_texture) {

        if (this->emissive_texture) {
            this->emissive_texture->unref();
        }

        emissive_texture->ref();
    }

    this->emissive_texture = emissive_texture;

    dirty_flags |= eMaterialProperties::PROP_EMISSIVE_TEXTURE;
}

void Material::set_occlusion_texture(Texture* occlusion_texture)
{
    if (this->occlusion_texture != occlusion_texture) {

        if (this->occlusion_texture) {
            this->occlusion_texture->unref();
        }

        occlusion_texture->ref();
    }

    this->occlusion_texture = occlusion_texture;

    dirty_flags |= eMaterialProperties::PROP_OCLUSSION_TEXTURE;
}

void Material::set_alpha_mask(float alpha_mask)
{
    this->alpha_mask = alpha_mask;
    dirty_flags |= eMaterialProperties::PROP_ALPHA_MASK;
}

void Material::set_depth_read(bool depth_read)
{
    this->depth_read = depth_read;
    dirty_flags |= eMaterialProperties::PROP_DEPTH_READ;
}

void Material::set_depth_write(bool depth_write)
{
    this->depth_write = depth_write;
    dirty_flags |= eMaterialProperties::PROP_DEPTH_WRITE;
}

void Material::set_use_skinning(bool use_skinning)
{
    this->use_skinning = use_skinning;
}

void Material::set_is_2D(bool is_2D)
{
    this->is_2D = is_2D;
}

void Material::set_transparency_type(eTransparencyType transparency_type)
{
    this->transparency_type = transparency_type;
    dirty_flags |= eMaterialProperties::PROP_TRANSPARENCY_TYPE;
}

void Material::set_topology_type(eTopologyType topology_type)
{
    this->topology_type = topology_type;
    dirty_flags |= eMaterialProperties::PROP_TOPOLOGY_TYPE;
}

void Material::set_cull_type(eCullType cull_type)
{
    this->cull_type = cull_type;
    dirty_flags |= eMaterialProperties::PROP_CULL_TYPE;
}

void Material::set_type(eMaterialType type)
{
    this->type = type;
    dirty_flags |= eMaterialProperties::PROP_TYPE;
}

void Material::set_priority(uint8_t priority)
{
    this->priority = priority;
    dirty_flags |= eMaterialProperties::PROP_PRIORITY;
}

void Material::set_shader(Shader* shader)
{
    this->shader = shader;
    dirty_flags |= eMaterialProperties::PROP_SHADER;
}

void Material::set_shader_pipeline(Pipeline* pipeline)
{
    this->shader->set_pipeline(pipeline);
}

glm::vec4 Material::get_color() const
{
    return color;
}

float Material::get_roughness() const
{
    return roughness;
}

float Material::get_metalness() const
{
    return metalness;
}

float Material::get_occlusion() const
{
    return occlusion;
}

glm::vec3 Material::get_emissive() const
{
    return emissive;
}

const Texture* Material::get_diffuse_texture() const
{
    return diffuse_texture;
}

const Texture* Material::get_metallic_roughness_texture() const
{
    return metallic_roughness_texture;
}

const Texture* Material::get_normal_texture() const
{
    return normal_texture;
}

const Texture* Material::get_emissive_texture() const
{
    return emissive_texture;
}

const Texture* Material::get_occlusion_texture() const
{
    return occlusion_texture;
}

float Material::get_alpha_mask() const
{
    return alpha_mask;
}

bool Material::get_depth_read() const
{
    return depth_read;
}

bool Material::get_depth_write() const
{
    return depth_write;
}

bool Material::get_use_skinning() const
{
    return use_skinning;
}

bool Material::get_is_2D() const
{
    return is_2D;
}

eTransparencyType Material::get_transparency_type() const
{
    return transparency_type;
}

eTopologyType Material::get_topology_type() const
{
    return topology_type;
}

eCullType Material::get_cull_type() const
{
    return cull_type;
}

eMaterialType Material::get_type() const
{
    return type;
}

uint8_t Material::get_priority() const
{
    return priority;
}

const Shader* Material::get_shader() const
{
    return shader;
}

Shader* Material::get_shader_ref()
{
    return shader;
}

void Material::reset_dirty_flags()
{
    dirty_flags = 0;
}

uint32_t Material::get_dirty_flags() const
{
    return dirty_flags;
}

Texture* Material::get_diffuse_texture()
{
    return diffuse_texture;
}

Texture* Material::get_metallic_roughness_texture()
{
    return metallic_roughness_texture;
}

Texture* Material::get_normal_texture()
{
    return normal_texture;
}

Texture* Material::get_emissive_texture()
{
    return emissive_texture;
}

Texture* Material::get_occlusion_texture()
{
    return occlusion_texture;
}
