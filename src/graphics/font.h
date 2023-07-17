#pragma once

#include "includes.h"

#include <map>
#include <string>
#include <vector>

struct CKerning {
    int first;
    int second;
    int amount;
};

struct Character {
    int id;
    int index;
    char character;
    glm::vec2 size;
    glm::vec2 offset;
    int xadvance;
    int chnl;
    glm::vec2 pos;
    int page;

    // Mesh properties
    glm::vec3 vertices[6];
    glm::vec2 uvs[6];
};

class Font {

    // json m_font_description;
    // std::vector<const CTexture*> m_pages;
    std::map<unsigned char, Character> m_characters;

    // Info
    std::string face;
    int size;
    int bold;
    int italic;
    std::vector<char> charset;
    int unicode;
    int stretchH;
    int smooth;
    int aa;
    glm::vec4 padding;
    glm::vec2 spacing;

    // Common
    int lineHeight;
    int base;
    int scaleW;
    int scaleH;
    int pages;
    int packed;
    int alphaChnl;
    int redChnl;
    int greenChnl;
    int blueChnl;

    int df_range;

    std::multimap<int, CKerning> m_kernings;

    float adjust_kerning_pairs(int first, int second);

    /*struct vertex {
        vec3 pos;    vec2 uv;    vec4 color;
        vertex(vec3 _pos, vec2 _uv, vec4 _color) :pos(_pos), uv(_uv), color(_color) {};
    };
    std::vector<vertex> m_buffer;*/

    static std::map<std::string, Font*> s_fonts;

public:


};