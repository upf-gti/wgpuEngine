#include "material.h"

#include "texture.h"
#include "pipeline.h"
#include "shader.h"

#include "imgui.h"

#define UNREF_TEXTURE(x) if(x) { x->unref(); }

Material::Material()
{
    properties["color"] = &color;
    properties["roughness"] = &roughness;
    properties["metallic"] = &metallic;
    properties["occlusion"] = &occlusion;
    properties["emissive"] = &emissive;
    properties["alpha_mask"] = &alpha_mask;
    properties["clear_coat_factor"] = &clearcoat_factor;
    properties["clear_coat_roughness"] = &clearcoat_roughness;

    uv_transforms.resize(MAX_UV_TRANSFORMS);
    for (auto& t : uv_transforms) {
        t = glm::mat4x4(1.0f);
    }
}

Material::~Material()
{
    UNREF_TEXTURE(diffuse_texture);
    UNREF_TEXTURE(metallic_roughness_texture);
    UNREF_TEXTURE(normal_texture);
    UNREF_TEXTURE(emissive_texture);
    UNREF_TEXTURE(occlusion_texture);
    UNREF_TEXTURE(clearcoat_texture);
    UNREF_TEXTURE(clearcoat_roughness_texture);
    UNREF_TEXTURE(clearcoat_normal_texture);
    UNREF_TEXTURE(iridescence_texture);
    UNREF_TEXTURE(iridescence_thickness_texture);
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

void Material::set_metallic(float metallic)
{
    this->metallic = metallic;
    dirty_flags |= eMaterialProperties::PROP_OCLUSSION_ROUGHNESS_METALLIC;
}

void Material::set_occlusion(float occlusion)
{
    this->occlusion = occlusion;
    dirty_flags |= eMaterialProperties::PROP_OCLUSSION_ROUGHNESS_METALLIC;
}

void Material::set_normal_scale(float normal_scale)
{
    this->normal_scale = normal_scale;
    dirty_flags |= eMaterialProperties::PROP_OCLUSSION_ROUGHNESS_METALLIC;
}

void Material::set_emissive(const glm::vec3& emissive)
{
    this->emissive = emissive;
    dirty_flags |= eMaterialProperties::PROP_EMISSIVE;
}

void Material::set_clearcoat_factor(float new_clearcoat_factor)
{
    clearcoat_factor = new_clearcoat_factor;
    dirty_flags |= eMaterialProperties::PROP_CLEARCOAT;
}

void Material::set_clearcoat_roughness(float new_clearcoat_roughness)
{
    this->clearcoat_roughness = new_clearcoat_roughness;
    dirty_flags |= eMaterialProperties::PROP_CLEARCOAT;
}

void Material::set_iridescence_factor(float new_iridescence_factor)
{
    iridescence_factor = new_iridescence_factor;
    dirty_flags |= eMaterialProperties::PROP_IRIDESCENCE;
}

void Material::set_iridescence_ior(float new_iridescence_ior)
{
    this->iridescence_ior = new_iridescence_ior;
    dirty_flags |= eMaterialProperties::PROP_IRIDESCENCE;
}

void Material::set_iridescence_thickness_min(float new_iridescence_thickness_min)
{
    this->iridescence_thickness_min = new_iridescence_thickness_min;
    dirty_flags |= eMaterialProperties::PROP_IRIDESCENCE;
}

void Material::set_iridescence_thickness_max(float new_iridescence_thickness_max)
{
    this->iridescence_thickness_max = new_iridescence_thickness_max;
    dirty_flags |= eMaterialProperties::PROP_IRIDESCENCE;
}

void Material::set_anisotropy_factor(float new_anisotropy_factor)
{
    anisotropy_factor = new_anisotropy_factor;
    dirty_flags |= eMaterialProperties::PROP_ANISOTROPY;
}

void Material::set_anisotropy_rotation(float new_anisotropy_rotation)
{
    this->anisotropy_rotation = new_anisotropy_rotation;
    dirty_flags |= eMaterialProperties::PROP_ANISOTROPY;
}

void Material::set_transmission_factor(float new_transmission_factor)
{
    transmission_factor = new_transmission_factor;
    dirty_flags |= eMaterialProperties::PROP_TRANSMISSION;
}

void Material::set_uv_transform(uint8_t index, const glm::mat3x3& matrix)
{
    use_uv_transforms = true;
    uv_transforms[index] = matrix;
}

void Material::set_diffuse_texture(Texture* diffuse_texture)
{
    if (this->diffuse_texture != diffuse_texture) {

        if (this->diffuse_texture) {
            this->diffuse_texture->unref();
        }

        if (diffuse_texture) {
            diffuse_texture->ref();
        }
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

void Material::set_clearcoat_texture(Texture* clearcoat_texture)
{
    if (this->clearcoat_texture != clearcoat_texture) {

        if (this->clearcoat_texture) {
            this->clearcoat_texture->unref();
        }

        clearcoat_texture->ref();
    }

    this->clearcoat_texture = clearcoat_texture;

    dirty_flags |= eMaterialProperties::PROP_CLEARCOAT_TEXTURE;
}

void Material::set_clearcoat_roughness_texture(Texture* clearcoat_roughness_texture)
{
    if (this->clearcoat_roughness_texture != clearcoat_roughness_texture) {

        if (this->clearcoat_roughness_texture) {
            this->clearcoat_roughness_texture->unref();
        }

        clearcoat_roughness_texture->ref();
    }

    this->clearcoat_roughness_texture = clearcoat_roughness_texture;

    dirty_flags |= eMaterialProperties::PROP_CLEARCOAT_ROUGHNESS_TEXTURE;
}

void Material::set_clearcoat_normal_texture(Texture* clearcoat_normal_texture)
{
    if (this->clearcoat_normal_texture != clearcoat_normal_texture) {

        if (this->clearcoat_normal_texture) {
            this->clearcoat_normal_texture->unref();
        }

        clearcoat_normal_texture->ref();
    }

    this->clearcoat_normal_texture = clearcoat_normal_texture;

    dirty_flags |= eMaterialProperties::PROP_CLEARCOAT_NORMAL_TEXTURE;
}

void Material::set_iridescence_texture(Texture* iridescence_texture)
{
    if (this->iridescence_texture != iridescence_texture) {

        if (this->iridescence_texture) {
            this->iridescence_texture->unref();
        }

        iridescence_texture->ref();
    }

    this->iridescence_texture = iridescence_texture;

    dirty_flags |= eMaterialProperties::PROP_IRIDESCENCE_TEXTURE;
}

void Material::set_iridescence_thickness_texture(Texture* iridescence_thickness_texture)
{
    if (this->iridescence_thickness_texture != iridescence_thickness_texture) {

        if (this->iridescence_thickness_texture) {
            this->iridescence_thickness_texture->unref();
        }

        iridescence_thickness_texture->ref();
    }

    this->iridescence_thickness_texture = iridescence_thickness_texture;

    dirty_flags |= eMaterialProperties::PROP_IRIDESCENCE_THICKNESS_TEXTURE;
}

void Material::set_anisotropy_texture(Texture* anisotropy_texture)
{
    if (this->anisotropy_texture != anisotropy_texture) {

        if (this->anisotropy_texture) {
            this->anisotropy_texture->unref();
        }

        anisotropy_texture->ref();
    }

    this->anisotropy_texture = anisotropy_texture;

    dirty_flags |= eMaterialProperties::PROP_ANISOTROPY_TEXTURE;
}

void Material::set_transmission_texture(Texture* transmission_texture)
{
    if (this->transmission_texture != transmission_texture) {

        if (this->transmission_texture) {
            this->transmission_texture->unref();
        }

        transmission_texture->ref();
    }

    this->transmission_texture = transmission_texture;

    dirty_flags |= eMaterialProperties::PROP_TRANSMISSION_TEXTURE;
}

void Material::set_alpha_mask(float alpha_mask)
{
    this->alpha_mask = alpha_mask;
    dirty_flags |= eMaterialProperties::PROP_ALPHA_MASK;
}

void Material::set_depth_read_write(bool value)
{
    set_depth_write(value);
    set_depth_read(value);
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

void Material::set_fragment_write(bool fragment_write)
{
    this->fragment_write = fragment_write;
    dirty_flags |= eMaterialProperties::PROP_FRAGMENT_WRITE;
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

void Material::set_use_tangents(bool value)
{
    use_tangents = value;
}

void Material::set_use_clearcoat(bool value)
{
    use_clearcoat = value;
    dirty_flags |= eMaterialProperties::PROP_CLEARCOAT_TOGGLE;
}

void Material::set_use_iridescence(bool value)
{
    use_iridescence = value;
    dirty_flags |= eMaterialProperties::PROP_IRIDESCENCE_TOGGLE;
}

void Material::set_use_anisotropy(bool value)
{
    use_anisotropy = value;
    dirty_flags |= eMaterialProperties::PROP_ANISOTROPY_TOGGLE;
}

void Material::set_use_transmission(bool value)
{
    use_transmission = value;
    dirty_flags |= eMaterialProperties::PROP_TRANSMISSION_TOGGLE;
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

float Material::get_metallic() const
{
    return metallic;
}

float Material::get_occlusion() const
{
    return occlusion;
}

float Material::get_normal_scale() const
{
    return normal_scale;
}

glm::vec3 Material::get_emissive() const
{
    return emissive;
}

float Material::get_clearcoat_factor() const
{
    return clearcoat_factor;
}

float Material::get_clearcoat_roughness() const
{
    return clearcoat_roughness;
}

float Material::get_iridescence_factor() const
{
    return iridescence_factor;
}

float Material::get_iridescence_ior() const
{
    return iridescence_ior;
}

float Material::get_iridescence_thickness_min() const
{
    return iridescence_thickness_min;
}

float Material::get_iridescence_thickness_max() const
{
    return iridescence_thickness_max;
}

float Material::get_anisotropy_factor() const
{
    return anisotropy_factor;
}

float Material::get_anisotropy_rotation() const
{
    return anisotropy_rotation;
}

float Material::get_transmission_factor() const
{
    return transmission_factor;
}

const std::vector<glm::mat4x4>& Material::get_uv_transforms() const
{
    return uv_transforms;
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

const Texture* Material::get_clearcoat_texture() const
{
    return clearcoat_texture;
}

const Texture* Material::get_clearcoat_roughness_texture() const
{
    return clearcoat_roughness_texture;
}

const Texture* Material::get_clearcoat_normal_texture() const
{
    return clearcoat_normal_texture;
}

const Texture* Material::get_iridescence_texture() const
{
    return iridescence_texture;
}

const Texture* Material::get_iridescence_thickness_texture() const
{
    return iridescence_thickness_texture;
}

const Texture* Material::get_anisotropy_texture() const
{
    return anisotropy_texture;
}

const Texture* Material::get_transmission_texture() const
{
    return transmission_texture;
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

bool Material::get_fragment_write() const
{
    return fragment_write;
}

bool Material::get_use_uv_transforms() const
{
    return use_uv_transforms;
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

void Material::set_dirty_flag(eMaterialProperties property_flag)
{
    dirty_flags |= property_flag;
}

void Material::reset_dirty_flags()
{
    dirty_flags = 0;
}

uint32_t Material::get_dirty_flags() const
{
    return dirty_flags;
}

void Material::render_gui()
{
    std::string material_name = name.empty() ? "" : (" (" + name + ")");
    if (ImGui::TreeNodeEx(("Material" + material_name).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

        ImGui::Text("Type");
        ImGui::SameLine(200);
        static const char* types[] = { "MATERIAL_PBR", "MATERIAL_UNLIT" };
        int type_int = static_cast<int>(type);
        if (ImGui::Combo("##Type", &type_int, types, ((int)(sizeof(types) / sizeof(*(types)))))) {
            set_type(static_cast<eMaterialType>(type_int));
        }

        ImGui::Text("Diffuse Color");
        ImGui::SameLine(200);
        if (ImGui::ColorEdit4("##Color", &color[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
            dirty_flags |= eMaterialProperties::PROP_COLOR;
        }

        ImGui::Text("Emissive Color");
        ImGui::SameLine(200);
        if (ImGui::ColorEdit4("##Emissive", &emissive[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
            dirty_flags |= eMaterialProperties::PROP_EMISSIVE;
        }

        ImGui::Text("Transparency Type");
        ImGui::SameLine(200);
        static const char* transparency_types[] = { "OPAQUE", "BLEND", "MASK", "HASH" };
        int transparency_type_int = static_cast<int>(transparency_type);
        if (ImGui::Combo("##Transparency", &transparency_type_int, transparency_types, ((int)(sizeof(transparency_types) / sizeof(*(transparency_types)))))) {
            set_transparency_type(static_cast<eTransparencyType>(transparency_type_int));
        }

        if (transparency_type == ALPHA_MASK) {
            ImGui::Text("Alpha Mask");
            ImGui::SameLine(200);
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
            if (ImGui::DragFloat("##Alpha Mask", &alpha_mask, 0.01f, 0.0f, 1.0f)) {
                dirty_flags |= eMaterialProperties::PROP_ALPHA_MASK;
            }
        }

        ImGui::Text("Roughness");
        ImGui::SameLine(200);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::DragFloat("##Roughness", &roughness, 0.01f, 0.0f, 1.0f)) {
            dirty_flags |= eMaterialProperties::PROP_OCLUSSION_ROUGHNESS_METALLIC;
        }

        ImGui::Text("Metallic");
        ImGui::SameLine(200);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::DragFloat("##Metallic", &metallic, 0.01f, 0.0f, 1.0f)) {
            dirty_flags |= eMaterialProperties::PROP_OCLUSSION_ROUGHNESS_METALLIC;
        }

        ImGui::Text("Oclussion");
        ImGui::SameLine(200);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::DragFloat("##Oclussion", &occlusion, 0.01f, 0.0f, 1.0f)) {
            dirty_flags |= eMaterialProperties::PROP_OCLUSSION_ROUGHNESS_METALLIC;
        }

        ImGui::Text("Normal Scale");
        ImGui::SameLine(200);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::DragFloat("##Normal Scale", &normal_scale, 0.01f, 0.0f, 1.0f)) {
            dirty_flags |= eMaterialProperties::PROP_NORMAL_SCALE;
        }

        // Clear coat
        bool uses_clearcoat = use_clearcoat;
        if (ImGui::Checkbox("Uses ClearCoat", &uses_clearcoat)) {
            set_use_clearcoat(uses_clearcoat);
        }

        if (use_clearcoat) {
            ImGui::Text("Factor");
            ImGui::SameLine(200);
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
            float factor = clearcoat_factor;
            if (ImGui::DragFloat("##CC_Factor", &factor, 0.01f, 0.0f, 1.0f)) {
                set_clearcoat_factor(factor);
            }

            ImGui::Text("Roughness");
            ImGui::SameLine(200);
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
            if (ImGui::DragFloat("##CC_Roughness", &clearcoat_roughness, 0.01f, 0.0f, 1.0f)) {
                dirty_flags |= eMaterialProperties::PROP_CLEARCOAT;
            }

            ImGui::Separator();
        }

        // Iridescence
        bool uses_iridescence = use_iridescence;
        if (ImGui::Checkbox("Uses Iridescence", &uses_iridescence)) {
            set_use_iridescence(uses_iridescence);
        }

        if (use_iridescence) {
            ImGui::Text("Factor");
            ImGui::SameLine(200);
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
            float factor = iridescence_factor;
            if (ImGui::DragFloat("##I_Factor", &factor, 0.01f, 0.0f, 1.0f)) {
                set_iridescence_factor(factor);
            }

            ImGui::Text("IOR");
            ImGui::SameLine(200);
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
            if (ImGui::DragFloat("##I_IOR", &iridescence_ior, 0.01f, 0.0f, 2.0f)) {
                dirty_flags |= eMaterialProperties::PROP_IRIDESCENCE;
            }

            ImGui::Text("Thickness");
            ImGui::SameLine(200);
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
            glm::vec2 iridescence_thickness = { get_iridescence_thickness_min(), get_iridescence_thickness_max() };
            if (ImGui::DragFloat2("##I_Thickness", &iridescence_thickness.x, 1.0f, 0.0f, 600.0f)) {
                set_iridescence_thickness_min(iridescence_thickness.x);
                set_iridescence_thickness_max(iridescence_thickness.y);
            }

            ImGui::Separator();
        }

        // Anisotropy
        bool uses_anisotropy = use_anisotropy;
        if (ImGui::Checkbox("Uses Anisotropy", &uses_anisotropy)) {
            set_use_anisotropy(uses_anisotropy);
        }

        if (use_anisotropy) {
            ImGui::Text("Factor");
            ImGui::SameLine(200);
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
            float factor = anisotropy_factor;
            if (ImGui::DragFloat("##A_Factor", &factor, 0.01f, 0.0f, 1.0f)) {
                set_anisotropy_factor(factor);
            }

            ImGui::Text("Rotation");
            ImGui::SameLine(200);
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
            if (ImGui::DragFloat("##A_Rotation", &anisotropy_rotation, 0.01f, 0.0f, 3.1415f)) {
                dirty_flags |= eMaterialProperties::PROP_ANISOTROPY;
            }

            ImGui::Separator();
        }

        ImGui::Text("Topology Type");
        ImGui::SameLine(200);
        static const char* topology_types[] = { "TOPOLOGY_POINT_LIST", "TOPOLOGY_LINE_LIST", "TOPOLOGY_LINE_STRIP", "TOPOLOGY_TRIANGLE_LIST", "TOPOLOGY_TRIANGLE_STRIP" };
        int topology_type_int = static_cast<int>(topology_type);
        if (ImGui::Combo("##Topology Type", &topology_type_int, topology_types, ((int)(sizeof(topology_types) / sizeof(*(topology_types)))))) {
            set_topology_type(static_cast<eTopologyType>(topology_type_int));
        }

        ImGui::Text("Cull Type");
        ImGui::SameLine(200);
        static const char* cull_types[] = { "NONE", "BACK", "FRONT" };
        int cull_type_int = static_cast<int>(cull_type);
        if (ImGui::Combo("##Cull Type", &cull_type_int, cull_types, ((int)(sizeof(cull_types) / sizeof(*(cull_types)))))) {
            set_cull_type(static_cast<eCullType>(cull_type_int));
        }

        ImGui::Text("Depth Read");
        ImGui::SameLine(200);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::Checkbox("##Depth Read", &depth_read)) {
            dirty_flags |= eMaterialProperties::PROP_DEPTH_READ;
        }

        ImGui::Text("Depth Write");
        ImGui::SameLine(200);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::Checkbox("##Depth Write", &depth_write)) {
            dirty_flags |= eMaterialProperties::PROP_DEPTH_WRITE;
        }

        ImGui::Text("Priority");
        ImGui::SameLine(200);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        int priority_int = static_cast<int>(priority);
        if (ImGui::DragInt("##Priority", &priority_int, 1, 0, 20)) {
            set_priority(static_cast<uint8_t>(priority_int));
        }

        ImGui::TreePop();
    }
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

Texture* Material::get_clearcoat_texture()
{
    return clearcoat_texture;
}

Texture* Material::get_clearcoat_roughness_texture()
{
    return clearcoat_roughness_texture;
}

Texture* Material::get_clearcoat_normal_texture()
{
    return clearcoat_normal_texture;
}

Texture* Material::get_iridescence_texture()
{
    return iridescence_texture;
}

Texture* Material::get_iridescence_thickness_texture()
{
    return iridescence_thickness_texture;
}

Texture* Material::get_anisotropy_texture()
{
    return anisotropy_texture;
}

Texture* Material::get_transmission_texture()
{
    return transmission_texture;
}

