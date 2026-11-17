#include "image_2d.h"

#include "shaders/ui/ui_texture.wgsl.gen.h"

Image2D::Image2D(const std::string& image_path, const glm::vec2& s, uint32_t flags) :
        Image2D("image_no_name", image_path, { 0.0f, 0.0f }, s, IMAGE, flags) {}

Image2D::Image2D(const std::string& name, const std::string& image_path, const glm::vec2& s, uint8_t priority) :
        Image2D(name, image_path, { 0.0f, 0.0f }, s, priority) {}

Image2D::Image2D(const std::string& name, const std::string& image_path, const glm::vec2& p, const glm::vec2& s, uint8_t priority, uint32_t flags) :
        Panel2D(name, p, s, flags)
{
    class_type = priority;

    Material* material = new Material();
    material->set_color(color);
    material->set_type(MATERIAL_UI);
    material->set_is_2D(true);
    material->set_cull_type(CULL_BACK);
    material->set_transparency_type(ALPHA_BLEND);
    material->set_priority(class_type);
    material->set_diffuse_texture(RenderStorage::get_singleton()->get_texture(image_path, TEXTURE_STORAGE_UI));
    material->set_depth_read_write(false);
    material->set_shader(RenderStorage::get_singleton()->get_singleton()->get_shader_from_source(shaders::ui_texture::source, shaders::ui_texture::path, shaders::ui_texture::libraries, material));

    quad_mesh->set_surface_material_override(quad_mesh->get_surface(0), material);

    RenderStorage::get_singleton()->register_ui_widget(material->get_shader_ref(), quad_mesh, ui_data, 3, true);
}

void Image2D::update(float delta_time)
{
    update_scroll_view();

    if (!visibility) {
        return;
    }

    update_ui_data();

    Node2D::update(delta_time);
}
