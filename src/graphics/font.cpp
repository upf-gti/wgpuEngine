#include "font.h"

#include "renderer_storage.h"

#include <filesystem>
namespace fs = std::filesystem;

#include "json.hpp"

using namespace std::string_literals;

std::map<std::string, Font*> Font::s_fonts;

Font::~Font()
{
    delete font_description;
}

Font* Font::get(const std::string& font_name)
{
    // check if already loaded
    std::map<std::string, Font*>::iterator it = s_fonts.find(font_name);
    if (it != s_fonts.end())
        return it->second;

    Font* font = new Font();
    font->load(font_name);
    s_fonts[font_name] = font;

    return font;
}

void Font::load(const std::string& font_name)
{
    font_description = new json();

    json j = *font_description = load_json("data/fonts/" + font_name + "/" + font_name + ".json"); 

    // Load all pages
    {   
        auto font_pages = j["pages"];
        for (std::string page : font_pages)
        {
            fs::path page_path = fs::path(page);
            page_path.replace_extension("png");
            std::string filename = "data/fonts/" + font_name + "/" + page_path.string();

            Texture* tex = RendererStorage::get_texture(filename);
            textures.push_back(tex);
        }
    }

    // Font info
    {
        auto& info = j["info"];
        face = info["face"];
        size = info["size"];
        bold = info["bold"];
        italic = info["italic"];

        std::vector<std::string> font_charset = info["charset"];
        charset.resize(font_charset.size());
        for (auto& character : font_charset) {
            charset.push_back(character[0]);
        }

        unicode = info["unicode"];
        stretchH = info["stretchH"];
        smooth = info["smooth"];
        aa = info["aa"];
        padding = glm::vec4(info["padding"][0], info["padding"][1], info["padding"][2], info["padding"][3]);
        spacing = glm::vec2(info["spacing"][0], info["spacing"][1]);

        auto& common = j["common"];

        lineHeight = common["lineHeight"];
        base = common["base"];
        scaleW = common["scaleW"];
        scaleH = common["scaleH"];
        pages = common["pages"];
        packed = common["packed"];
        alphaChnl = common["alphaChnl"];
        redChnl = common["redChnl"];
        greenChnl = common["greenChnl"];
        blueChnl = common["blueChnl"];

        auto& distanceField = j["distanceField"];
        df_range = distanceField["distanceRange"];
    }

    // Fill kernings multimap
    {
        auto& _kernings = j["kernings"];
        for (auto& kerning : _kernings) {
            CKerning k;
            k.first = kerning["first"];
            k.second = kerning["second"];
            k.amount = kerning["amount"];

            kernings.insert({ k.first,k });
        }
    }

    // Fill character map with all its properties
    {
        auto _characters = j["chars"];
        for (auto& character : _characters)
        {
            Character new_character;
            new_character.id = character.value("id", -1);
            new_character.index = character.value("index", -1);
            std::string c = character["char"];
            new_character.character = c[0];
            glm::vec2 pos = new_character.pos = glm::vec2(character["x"], character["y"]) + glm::vec2(0.5f);
            glm::vec2 size = new_character.size = glm::vec2(character["width"], character["height"]);
            glm::vec2 off = new_character.offset = glm::vec2(character["xoffset"], character["yoffset"]);
            int  adv = new_character.xadvance = character["xadvance"];
            int  chnl = new_character.chnl = character["chnl"];
            int  pg = new_character.page = character["page"];

            //A:00 C:10
            //B:01 D:11
            //A B C
            //C B D
            //ABCCBD

            //ACBD
            glm::vec3 A = glm::vec3(off, 0.f);
            glm::vec3 B = glm::vec3(off + glm::vec2(0.f, size.y), 0.f);
            glm::vec3 C = glm::vec3(off + glm::vec2(size.x, 0.f), 0.f);
            glm::vec3 D = glm::vec3(off + size, 0.f)             ;

            glm::vec2 uvA = pos;
            glm::vec2 uvB = pos + glm::vec2(0.f, size.y);
            glm::vec2 uvC = pos + glm::vec2(size.x, 0.f);
            glm::vec2 uvD = pos + size;

            new_character.vertices[0] = A;
            new_character.vertices[1] = B;
            new_character.vertices[2] = C;
            new_character.vertices[3] = C;
            new_character.vertices[4] = B;
            new_character.vertices[5] = D;

            new_character.uvs[0] = uvA;
            new_character.uvs[1] = uvB;
            new_character.uvs[2] = uvC;
            new_character.uvs[3] = uvC;
            new_character.uvs[4] = uvB;
            new_character.uvs[5] = uvD;

            characters[new_character.id] = new_character;
        }
    }
}

float Font::adjust_kerning_pairs(int first, int second) {

    typedef std::multimap<int, CKerning>::iterator kiterator;
    if (!kernings.count(first))
        return 0.f;

    std::pair<kiterator, kiterator> result = kernings.equal_range(first);

    for (kiterator it = result.first; it != result.second; ++it)
    {
        CKerning& k = it->second;
        if (k.second == second) {
            return (float)k.amount;
        }
    }

    return 0.f;
}
