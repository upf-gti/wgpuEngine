#pragma once

#include <stdint.h>

namespace ui {

    const float PICKER_SIZE         = 154.f;
    const float BUTTON_SIZE         = 64.f;
    const float GROUP_MARGIN        = 12.f;
    const float TEXT_SHADOW_MARGIN  = 1.f;

    const float HOVER_HAPTIC_AMPLITUDE  = 0.05f;
    const float HOVER_HAPTIC_DURATION   = 0.1f;

    enum ComboIndex {
        UNIQUE,
        FIRST,
        MIDDLE,
        LAST
    };

    enum Node2DFlags : uint32_t {
        SELECTED = 1 << 0,
        DISABLED = 1 << 1,
        UNIQUE_SELECTION = 1 << 2,
        ALLOW_TOGGLE = 1 << 3,
        SKIP_NAME = 1 << 4,
        SKIP_VALUE = 1 << 5,
        USER_RANGE = 1 << 6,
        CURVE_INV_POW = 1 << 7,
        TEXT_CENTERED = 1 << 8,
        TEXT_EVENTS = 1 << 9,
        TEXT_SELECTABLE = 1 << 10,
        SKIP_TEXT_RECT = 1 << 11,
        SCROLLABLE = 1 << 12,
        DBL_CLICK = 1 << 13,
        LONG_CLICK = 1 << 14,
        HIDDEN = 1 << 15,
        CONFIRM_BUTTON = 1 << 16,
        CREATE_3D = 1 << 17
    };
}
