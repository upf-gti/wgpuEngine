#pragma once

#include "includes.h"
#include "utils.h"
#include <map>
#include <string>
#include <vector>

#include <filesystem>
namespace fs = std::filesystem;

#include "graphics/texture.h"

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

public:

    nlohmann::json font_description;
    std::vector<Texture*> textures;
    std::map<unsigned char, Character> characters;
    std::multimap<int, CKerning> kernings;

    static std::map<std::string, Font*> s_fonts;

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

    static Font* get(const std::string& font_name);
    void load(const std::string& font_name);
    float adjust_kerning_pairs(int first, int second);
};