#pragma once

#include "includes.h"
#include "framework/utils/json_utils.h"
#include "framework/math.h"
#include "font_common.h"
#include "graphics/texture.h"

#include <map>
#include <string>
#include <vector>

class Font {

public:

    ~Font();

    nlohmann::json* font_description = nullptr;
    std::vector<Texture*> textures;
    std::map<uint32_t, Character> characters;
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
