#include "text_2d.h"

#include "core/managers/input/input_manager.h"
#include "core/managers/xr/xr_manager.h"

#include "scene/3d/text_3d.h"

#include "framework/ui/io.h"

#include "shaders/ui/ui_text_shadow.wgsl.gen.h"

Text2D* Text2D::selected = nullptr;

Text2D::Text2D(const std::string& _text, float scale, uint32_t flags) :
        Text2D(_text, { 0.0f, 0.0f }, scale, flags, colors::BLACK)
{
}

Text2D::Text2D(const std::string& _text, const glm::vec2& pos, float scale, uint32_t flags, const Color& color) :
        Panel2D("no_signal", pos, { 1.0f, 1.0f }, flags)
{
    text_string = _text;
    text_scale = scale;

    class_type = Node2DClassType::TEXT_SHADOW;

    text_entity = new Text3D(text_string, colors::WHITE, true);
    text_entity->generate_mesh();
    text_entity->set_scale(text_scale);
    text_entity->get_surface_material(0)->set_priority(Node2DClassType::TEXT);

    float text_width = (float)text_entity->get_text_width(text_string);
    size.x = std::max(text_width, 8.0f) + TEXT_SHADOW_MARGIN * text_scale;
    size.y = text_scale + TEXT_SHADOW_MARGIN * text_scale * 0.5f;

    render_background = !(parameter_flags & SKIP_TEXT_RECT);

    if ((parameter_flags & TEXT_SELECTABLE) && (parameter_flags & SELECTED)) {
        selected = this;
    }

    Material* material = new Material();
    material->set_color(color);
    material->set_type(MATERIAL_UI);
    material->set_is_2D(true);
    material->set_cull_type(CULL_BACK);
    material->set_transparency_type(ALPHA_BLEND);
    material->set_priority(class_type);
    material->set_depth_read_write(false);
    material->set_shader(RenderStorage::get_singleton()->get_singleton()->get_shader_from_source(shaders::ui_text_shadow::source, shaders::ui_text_shadow::path, shaders::ui_text_shadow::libraries, material));

    Surface* quad_surface = quad_mesh->get_surface(0);
    quad_surface->create_quad(size.x, size.y, true);
    quad_mesh->set_surface_material_override(quad_mesh->get_surface(0), material);

    Node::bind(name + "@selected", [&](const std::string& signal, void* data) {
        if (parameter_flags & TEXT_SELECTABLE) {
            selected = this;
        }
    });

    RenderStorage::get_singleton()->register_ui_widget(material->get_shader_ref(), quad_mesh, ui_data, 3, true);
}

void Text2D::update(float delta_time)
{
    update_scroll_view();

    Node2D* parent = get_parent<Node2D*>();

    if ((parameter_flags & TEXT_CENTERED) && parent) {
        const glm::vec2& par_size = parent->get_size();
        const glm::vec2& centered_position = { -size.x * 0.5f + par_size.x * 0.5f, get_local_translation().y };
        set_position(centered_position);
    }

    // Set Text entity in place
    {
        uint8_t priority = class_type;
        glm::vec2 scale = get_scale();
        glm::vec2 position = get_translation() + (render_background ? glm::vec2(TEXT_SHADOW_MARGIN * text_scale, TEXT_SHADOW_MARGIN * text_scale * 0.5f) * 0.5f * scale : glm::vec2(0.0f));

        auto t = Transform::identity();
        t.translate(glm::vec3(position, -priority * 3e-5));
        t.scale(glm::vec3(scale.x, -scale.y, 1.0f)); // Hack to invert text mesh in 2d

        text_entity->set_transform(Transform::combine(Transform::mat4_to_transform(get_global_viewport_model()), t));
    }

    if (visibility && (parameter_flags & TEXT_EVENTS)) {
        sInputData data = get_input_data();

        if (data.is_hovered) {
            IO::push_input(this, data);
        }

        // Update uniforms
        ui_data.flags &= ~(UI_DATA_HOVERED | UI_DATA_PRESSED | UI_DATA_SELECTED);
        ui_data.hover_time = 0.0f;

        on_hover = false;

        if (selected == this) {
            ui_data.flags |= UI_DATA_SELECTED;
        }
    }

    ui_data.aspect_ratio = size.x / size.y;

    update_ui_data();

    Node2D::update(delta_time);
}

bool Text2D::on_input(sInputData data)
{
    IO::set_hover(this, data);

    Panel2D::on_input(data);

    // Internally, use on release mouse, not on press..
    if (data.was_released) {
        if (!on_pressed()) {
            Node::emit_signal(name, (void*)this);

            if (parameter_flags & Node2DFlags::TEXT_SELECTABLE) {
                selected = this;
            }
        }
    }

    // Send signal on move thumbstick or using wheel hovering the text
    {
        float dt = XR_MANAGER->get_thumbstick_value(HAND_RIGHT).y;

        if (glm::abs(dt) > 0.01f) {
            Node::emit_signal(name + "@stick_moved", dt * 0.1f);
        }

        float wheel_dt = INPUT_MANAGER->get_mouse_wheel_delta();

        if (glm::abs(wheel_dt) > 0.01f) {
            Node::emit_signal(name + "@stick_moved", wheel_dt);
        }
    }

    // Update uniforms
    ui_data.flags |= UI_DATA_HOVERED;
    ui_data.hover_time = 1.0f;

    on_hover = true;

    update_ui_data();

    return true;
}

void Text2D::release()
{
    delete text_entity;

    Node2D::release();
}

void Text2D::render()
{
    if (!visibility) {
        return;
    }

    Panel2D::render();

    text_entity->render();
}

void Text2D::set_text(const std::string& text)
{
    text_entity->set_text(text);

    float text_width = (float)text_entity->get_text_width(text);
    size.x = std::max(text_width, 8.0f) + TEXT_SHADOW_MARGIN * text_scale;
    size.y = text_scale + TEXT_SHADOW_MARGIN * text_scale * 0.5f;

    Surface* quad_surface = quad_mesh->get_surface(0);
    quad_surface->create_quad(size.x, size.y, true);

    Node2D::on_children_changed();
}

void Text2D::disable_2d()
{
    Material* material = text_entity->get_surface_material(0);

    if (material->get_is_2D()) {
        material->set_is_2D(false);
        material->set_depth_read(true);

        text_entity->set_is_2d(false);
    }

    Panel2D::disable_2d();
}

void Text2D::set_priority(uint8_t priority)
{
    text_entity->get_surface_material(0)->set_priority(priority);

    Panel2D::set_priority(priority);
}

void Text2D::set_signal(const std::string& new_signal)
{
    // Remove old events..
    Node::unbind(name + "@selected");

    Panel2D::set_signal(new_signal);

    // Bind new ones
    Node::bind(name + "@selected", [&](const std::string& signal, void* data) {
        if (parameter_flags & TEXT_SELECTABLE) {
            selected = this;
        }
    });
}
