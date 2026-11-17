#pragma once

#include "scene/3d/node_3d.h"

class VisualInstance3D : public Node3D {
protected:
    virtual void notification(int notification_id);

    uint32_t render_list_idx = 0;

public:
    VisualInstance3D();
    ~VisualInstance3D();
};
