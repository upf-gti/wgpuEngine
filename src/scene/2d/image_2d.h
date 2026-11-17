#pragma once

#include "panel_2d.h"

class Image2D : public Panel2D {
public:
    Image2D() {}
    Image2D(const std::string& image_path, const glm::vec2& s, uint32_t flags = 0);
    Image2D(const std::string& name, const std::string& image_path, const glm::vec2& s, uint8_t priority = IMAGE);
    Image2D(const std::string& name, const std::string& image_path, const glm::vec2& p, const glm::vec2& s, uint8_t priority = IMAGE, uint32_t flags = 0);

    void update(float delta_time) override;
};
