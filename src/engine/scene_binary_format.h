#pragma once

#include <cstdint>

constexpr uint8_t MAX_SCENE_NAME_SIZE = 64;

struct sSceneBinaryHeader {
    uint8_t version = 1;
    size_t node_count = 0;
};
