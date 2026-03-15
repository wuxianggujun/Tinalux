#pragma once

#include "tinalux/core/Geometry.h"
#include "tinalux/ui/ColorScheme.h"
#include "tinalux/ui/Spacing.h"
#include "tinalux/ui/Typography.h"

namespace tinalux::ui {

struct PanelStyle {
    core::Color backgroundColor = core::colorRGB(32, 35, 47);
    float cornerRadius = 18.0f;

    static PanelStyle standard(
        const ColorScheme& colors,
        const Typography& typography,
        const Spacing& spacing);
};

}  // namespace tinalux::ui
