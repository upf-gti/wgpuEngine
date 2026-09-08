#pragma once

#include "core/error/error.h"

class RenderMethod {
public:
    virtual Error initialize() = 0;
    virtual Error finalize() = 0;

    virtual void render_scene() = 0;

private:
};
