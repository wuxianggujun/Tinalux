#pragma once

#include "tinalux/core/Geometry.h"
#include "tinalux/ui/ColorScheme.h"
#include "tinalux/ui/Spacing.h"
#include "tinalux/ui/Typography.h"

namespace tinalux::ui {

struct ScrollViewStyle {
    core::Color scrollbarThumbColor = core::colorARGB(120, 166, 173, 200);
    core::Color scrollbarTrackColor = core::colorARGB(32, 166, 173, 200);
    float scrollbarMargin = 6.0f;
    float scrollbarWidth = 6.0f;
    float minThumbHeight = 24.0f;
    float scrollStep = 48.0f;

    static ScrollViewStyle standard(
        const ColorScheme& colors,
        const Typography& typography,
        const Spacing& spacing);
};

}  // namespace tinalux::ui
