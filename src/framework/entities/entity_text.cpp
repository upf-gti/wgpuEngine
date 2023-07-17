#include "entity_text.h"
#include "graphics/renderer.h"
#include <assert.h>

void TextEntity::write_char(glm::vec3 pos, Character& ch)
{
	/*float size = (float)m_scale / m_active->size;
	for (int k = 0; k < 6; ++k) {
		vertex b = vertex((pos + ch.vertices[k]) * size, ch.uvs[k], COLOR::White);
		m_buffer.push_back(b);
	}*/
}

void TextEntity::write(glm::vec2 pos, glm::vec2 size, std::wstring text, bool wrap)
{
	return write(glm::vec3(pos, 0), size, text, wrap);
}

void TextEntity::write(glm::vec3 pos, glm::vec2 size, std::wstring text, bool wrap)
{
	pos.y *= -1.f;
	glm::mat4x4 model = glm::translate(glm::mat4x4(1.f), pos);
	return write(model, size, text, wrap);
}

void TextEntity::write(glm::mat4x4 model, glm::vec2 box, std::wstring text, bool wrap)
{
    assert(font || "No font set prior to draw!");

    if (text.empty()) 
        return;

    /*

    float size = (float)scale / font->size;
    float space = (float)font->m_characters[' '].xadvance;
    glm::vec3 pos = model[3];
    glm::vec3 initial_pos = pos;
    std::string word = "";
    int word_count = 0;
    int last_char = text[0];

    for (int i = 0; i <= text.size(); ++i) {
        int c = text[i];
        if (c == ' ' || i == text.size()) { // End of word or text. Push the word to our buffer.

            //We must decide prior to draw the word if it fits on this line or has to go on the next one.
            float word_size = get_text_width(word) * size;
            if (wrap
                && word_count > 0
                //So the ammount of space already traversed + the width of the word fits on the line?
                && (pos.x - initial_pos.x + word_size > box.x)
                )
            {
                //Move to the start of the next line
                pos.x = initial_pos.x;
                pos.y += (float)font->lineHeight;
                word_count = 0;
            }
            else
            {
                //We dont need to move, lets write our word
                ++word_count;
            }

            //Write the word
            for (int j = 0; j < word.size(); ++j)
            {
                //Check we dont overflow the box from the bottom
                if (wrap && pos.y + (float)font->lineHeight > box.y) continue;

                c = word[j];
                Character& ch = font->m_characters[c];
                write_char(pos, ch);
                pos.x += (float)ch.xadvance;

                //Adjust next leter position depending on the kerning
                if (j + 1 < word.size()) {
                    float amount = font->adjust_kerning_pairs(c, text[j + 1]);
                    pos.x += (float)amount;
                }
            }

            //Add the space we found in first place :D
            pos.x += space;
            word = "";
        }
        else
            word = word + (char)c;


    }

    */

    /*

    //update mesh
    m_mesh->Reserve(text.size() * 6);
    m_mesh->UpdateVertsFromCPU(m_buffer.data(), m_buffer.size() * sizeof(vertex));
    m_mesh->Activate();

    //activate mat
    m_material->Activate();
    m_active->m_pages[0]->Activate(TS_COLOR);

    //upload constants
    auto vp = ERender.GetViewport();

    //mat4 proj = mat4::CreateOrthographic(vp.Width, vp.Height, vp.MinDepth, vp.MaxDepth);
    mat4 proj = mat4::CreateOrthographicOffCenter(0, vp.Width, vp.Height, 0, vp.MinDepth, vp.MaxDepth);

    ctes_font.u_font_scale = size;
    ctes_font.u_font_ortho = proj;
    ctes_font.updateGPU();
    ctes_object.u_model = model;
    ctes_object.u_color = COLOR::White;
    ctes_object.updateGPU();

    //draw
    m_mesh->RenderRange((uint32_t)text.size() * 6, 0u);

    */
}

int TextEntity::get_text_width(const std::string text)
{
	int size = 0;
	int textsize = (int)text.size();

	/*
    * 
    for (int i = 0; i < textsize; ++i) {
		unsigned char c = text[i];
		Character& ch = font->m_characters[c];
		int kern = (i + 1 < textsize) ? (int)font->adjust_kerning_pairs(c, text[i + 1]) : 0;
		size += ch.xadvance + kern;
	}

    */

	return size;
}