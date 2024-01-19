#pragma once

#include "includes.h"

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
