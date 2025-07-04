#include "text_3d.h"

#include "graphics/renderer.h"
#include "graphics/font.h"
#include "graphics/renderer_storage.h"

#include "shaders/sdf_fonts.wgsl.gen.h"

#include <assert.h>

Text3D::Text3D(const std::string& text, const Color& color, bool is_2d, const glm::vec2& box_size, bool wrap)
    : MeshInstance3D(), text(text), color(color), is_2d(is_2d), box_size(box_size), wrap(wrap)
{
    font = Font::get("Mangaka");
    font->ref();

    mesh = new Mesh();
    mesh->set_mesh_type("TextMesh");
    mesh->set_node_ref(this);
}

Text3D::~Text3D()
{
    if (font) {
        font->unref();
    }
}

void Text3D::update(float delta_time)
{

}

void Text3D::append_char(const glm::vec3& pos, const Character& ch)
{
    float size = (float)font_scale / font->size;
    for (int k = 0; k < 6; ++k) {
        vertices.vertices.push_back((pos + glm::vec3(ch.vertices[k].x, -ch.vertices[k].y, ch.vertices[k].z)) * size);
        vertices.uvs.push_back(ch.uvs[k] / glm::vec2(font->scaleW, font->scaleH));
        vertices.normals.push_back(glm::vec3(0.f, 1.f, 0.f));
        vertices.colors.push_back(color);
    }
}

void Text3D::generate_mesh()
{
    assert(font || "No font set prior to draw!");

    if (text.empty() || !font) {
        return;
    }

    auto& surfaces = mesh->get_surfaces();

    // Clear previous mesh
    if (!surfaces.empty()) {
        vertices.clear();
        surfaces.clear();
    }

    float scale = (float)font_scale / font->size;
    float space = (float)font->characters[' '].xadvance;
    float line_height = (float)font->lineHeight;
    glm::vec3 pos(0.f);
    glm::vec3 initial_pos = pos;
    std::string word = "";
    int word_count = 0;
    int last_char = text[0];

    for (int i = 0; i <= text.size(); ++i) {
        char c = i < text.size() ? text[i] : '\0';

        // process the current word on "space" or end of the word
        if (c == ' ' || c == '\0') {
            if (!word.empty()) {
                float word_size = get_text_width(word);

                // We must decide prior to draw the word if it fits on this line or has to go on the next one.
                if (wrap && word_count > 0 && ((pos.x - initial_pos.x) * scale + word_size > box_size.x)) {
                    pos.x = initial_pos.x;
                    pos.y += line_height;
                    word_count = 0;
                }

                for (int j = 0; j < word.size(); ++j) {
                    if (wrap && (pos.y + line_height) * scale > box_size.y) {
                        continue;
                    }

                    char wc = word[j];
                    const Character& ch = font->characters[wc];
                    append_char(pos, ch);
                    pos.x += ch.xadvance;

                    // Adjust next leter position depending on the kerning
                    if (j + 1 < word.size()) {
                        float amount = font->adjust_kerning_pairs(wc, word[j + 1]);
                        pos.x += amount;
                    }
                }

                pos.x += space; // space after word
                word_count++;
                word.clear();
            }
        }
        else {
            word += c;
        }
    }

    if (vertices.vertices.empty()) {
        return;
    }

    Surface* surface = new Surface();
    surface->create_surface_data(vertices);

    Material* material = new Material();
    material->set_color(color);
    material->set_is_2D(is_2d);
    material->set_type(MATERIAL_UNLIT);
    material->set_transparency_type(ALPHA_BLEND);
    material->set_cull_type(CULL_BACK);
    material->set_depth_read_write(false);
    material->set_diffuse_texture(font->textures[0]);
    material->set_shader(RendererStorage::get_shader_from_source(shaders::sdf_fonts::source, shaders::sdf_fonts::path, shaders::sdf_fonts::libraries, material));
    surface->set_material(material);

    mesh->add_surface(surface);
}

void Text3D::set_text(const std::string& p_text)
{
    text = p_text;
    generate_mesh();
}

void Text3D::set_scale(float new_scale)
{
    font_scale = new_scale;
    generate_mesh();
}

void Text3D::set_wrap(bool new_wrap)
{
    wrap = new_wrap;
    generate_mesh();
}

void Text3D::set_is_2d(bool new_is_2d)
{
    is_2d = new_is_2d;
    generate_mesh();
}

void Text3D::set_box_size(const glm::vec2& new_box_size)
{
    box_size = new_box_size;
    generate_mesh();
}

int Text3D::get_text_width(const std::string& text)
{
    int size = 0;
    int textsize = (int)text.size();
    float scale = (float)font_scale / font->size;

    for (int i = 0; i < textsize; ++i) {
        unsigned char c = text[i];
        Character& ch = font->characters[c];
        int kern = (i + 1 < textsize) ? (int)font->adjust_kerning_pairs(c, text[i + 1]) : 0;
        size += c == ' ' ? 16 : (ch.xadvance + kern);
    }

    return size * scale;
}

int Text3D::get_text_height(const std::string& text)
{
    float size = 0;
    float scale = (float)font_scale / font->size;

    for (int i = 0; i < (int)text.size(); ++i) {
        unsigned char c = text[i];
        Character& ch = font->characters[c];
        size = std::max(ch.size.y, size);
    }

    return size * scale;
}

void Text3D::render_gui()
{
    bool dirty = false;

    ImGui::Text("Wrap");
    ImGui::SameLine(200);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    if (ImGui::Checkbox("##Wrap", &wrap)) {
        dirty = true;
    }

    ImGui::Text("Box Size");
    ImGui::SameLine(200);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    if (ImGui::DragFloat2("##Box Size", &box_size.x, 0.01f, 0.0f, 16.0f)) {
        dirty = true;
    }

    if (dirty) {
        generate_mesh();
    }

    MeshInstance3D::render_gui();
}
