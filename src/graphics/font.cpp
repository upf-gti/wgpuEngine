#include "font.h"

#include "renderer_storage.h"
#include "graphics/texture.h"

#include <filesystem>
namespace fs = std::filesystem;

#include "rapidjson/document.h"

using namespace std::string_literals;

std::map<std::string, Font*> Font::s_fonts;

Font::~Font()
{
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
    rapidjson::Document j = load_json("data/fonts/" + font_name + "/" + font_name + ".json");

    // Load all pages
    {   
        const rapidjson::Value& font_pages = j["pages"];
        for (rapidjson::SizeType i = 0; i < font_pages.Size(); i++)
        {
            fs::path page_path = fs::path(font_pages[i].GetString());
            page_path.replace_extension("png");
            std::string filename = "data/fonts/" + font_name + "/" + page_path.string();

            Texture* tex = RendererStorage::get_texture(filename, TEXTURE_STORAGE_KEEP_MEMORY);
            textures.push_back(tex);
        }
    }

    // Font info
    {
        const rapidjson::Value& info = j["info"];
        face = info["face"].GetString();
        size = info["size"].GetInt();
        bold = info["bold"].GetInt();
        italic = info["italic"].GetInt();

        const rapidjson::Value& font_charset = info["charset"];
        charset.resize(font_charset.Size());
        for (rapidjson::SizeType i = 0; i < font_charset.Size(); i++)
        {
            charset.push_back(font_charset[i].GetString()[0]);
        }

        unicode = info["unicode"].GetInt();
        stretchH = info["stretchH"].GetInt();
        smooth = info["smooth"].GetInt();
        aa = info["aa"].GetInt();

        const rapidjson::Value& j_padding = info["padding"].GetArray();
        const rapidjson::Value& j_spacing = info["spacing"].GetArray();

        padding = glm::vec4(j_padding[0].GetInt(), j_padding[1].GetInt(), j_padding[2].GetInt(), j_padding[3].GetInt());
        spacing = glm::vec2(j_spacing[0].GetInt(), j_spacing[1].GetInt());

        const rapidjson::Value& common = j["common"];

        lineHeight = common["lineHeight"].GetInt();
        base = common["base"].GetInt();
        scaleW = common["scaleW"].GetInt();
        scaleH = common["scaleH"].GetInt();
        pages = common["pages"].GetInt();
        packed = common["packed"].GetInt();
        alphaChnl = common["alphaChnl"].GetInt();
        redChnl = common["redChnl"].GetInt();
        greenChnl = common["greenChnl"].GetInt();
        blueChnl = common["blueChnl"].GetInt();

        const rapidjson::Value& distanceField = j["distanceField"];
        df_range = distanceField["distanceRange"].GetInt();
    }

    // Fill kernings multimap
    {
        const rapidjson::Value& _kernings = j["kernings"];
        for (rapidjson::SizeType i = 0; i < _kernings.Size(); i++)
        {
            const rapidjson::Value& kerning = _kernings[i];
            CKerning k;
            k.first = kerning["first"].GetInt();
            k.second = kerning["second"].GetInt();
            k.amount = kerning["amount"].GetInt();

            kernings.insert({ k.first,k });
        }
    }

    // Fill character map with all its properties
    {
        const rapidjson::Value& _characters = j["chars"];
        for (rapidjson::SizeType i = 0; i < _characters.Size(); i++)
        {
            const rapidjson::Value& character = _characters[i];

            Character new_character;
            new_character.id = character.HasMember("id") ? character["id"].GetInt() : -1;
            new_character.index = character.HasMember("index") ? character["index"].GetInt() : -1;
            std::string c = character["char"].GetString();
            new_character.character = c[0];
            glm::vec2 pos = new_character.pos = glm::vec2(character["x"].GetInt(), character["y"].GetInt()) + glm::vec2(0.5f);
            glm::vec2 size = new_character.size = glm::vec2(character["width"].GetInt(), character["height"].GetInt());
            glm::vec2 off = new_character.offset = glm::vec2(character["xoffset"].GetInt(), character["yoffset"].GetInt());
            int  adv = new_character.xadvance = character["xadvance"].GetInt();
            int  chnl = new_character.chnl = character["chnl"].GetInt();
            int  pg = new_character.page = character["page"].GetInt();

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
