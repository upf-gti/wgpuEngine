#pragma once

#include <cstdint>

constexpr uint8_t MAX_NODE_NAME_SIZE = 64;

struct sNodeBinaryHeader {
    size_t children_count = 0;
};
