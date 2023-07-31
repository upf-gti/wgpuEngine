#include "entity_text.h"
#include "graphics/renderer.h"
#include "graphics/mesh.h"
#include <assert.h>

TextEntity::TextEntity(const std::string& _text, glm::vec2 _box_size, bool _wrap) : EntityMesh()
{
    font = Font::get("Lato");
    text = _text;
    box_size = _box_size;
    wrap = _wrap;
}

void TextEntity::update(float delta_time)
{
    
}

void TextEntity::append_char(glm::vec3 pos, Character& ch)
{
	float size = (float)font_scale / font->size;
	for (int k = 0; k < 6; ++k) {

        vertices.push_back({
            .position = (pos + ch.vertices[k]) * size,
            .uv = ch.uvs[k] / glm::vec2(font->scaleW, font->scaleH),
            .normal = glm::vec3(0.f, 1.f, 0.f),
            .color = color
        });
	}
}

void TextEntity::generate_mesh()
{
    assert(font || "No font set prior to draw!");

    if (text.empty() || !font) 
        return;

    float size = (float)font_scale / font->size;
    float space = (float)font->characters[' '].xadvance;
    glm::vec3 pos(0.f);
    glm::vec3 initial_pos = pos;
    std::string word = "";
    int word_count = 0;
    int last_char = text[0];

    for (int i = 0; i <= text.size(); ++i) {
        int c = text[i];
        if (c == ' ' || i == text.size()) { // End of word or text. Push the word to our buffer.

            // We must decide prior to draw the word if it fits on this line or has to go on the next one.
            float word_size = get_text_width(word) * size;
            if (wrap && word_count > 0
                // So the ammount of space already traversed + the width of the word fits on the line?
                && (pos.x - initial_pos.x + word_size > box_size.x) )
            {
                // Move to the start of the next line
                pos.x = initial_pos.x;
                pos.y += (float)font->lineHeight;
                word_count = 0;
            }
            else
            {
                // We dont need to move, lets write our word
                ++word_count;
            }

            // Write the word
            for (int j = 0; j < word.size(); ++j)
            {
                // Check we dont overflow the box from the bottom
                if (wrap && pos.y + (float)font->lineHeight > box_size.y) continue;

                c = word[j];
                Character& ch = font->characters[c];
                append_char(pos, ch);
                pos.x += (float)ch.xadvance;

                //Adjust next leter position depending on the kerning
                if (j + 1 < word.size()) {
                    float amount = font->adjust_kerning_pairs(c, text[j + 1]);
                    pos.x += (float)amount;
                }
            }

            // Add the space we found in first place
            pos.x += space;
            word = "";
        }
        else
            word = word + (char)c;

    }

    mesh = new Mesh();
    mesh->create_from_vertices(vertices);
    mesh->set_texture(font->textures[0]);
    mesh->add_instance();
    mesh->create_bind_group_texture(Shader::get("data/shaders/sdf_fonts.wgsl"), 0);
}

int TextEntity::get_text_width(const std::string text)
{
	int size = 0;
	int textsize = (int)text.size();

    for (int i = 0; i < textsize; ++i) {
		unsigned char c = text[i];
		Character& ch = font->characters[c];
		int kern = (i + 1 < textsize) ? (int)font->adjust_kerning_pairs(c, text[i + 1]) : 0;
		size += ch.xadvance + kern;
	}

	return size;
}