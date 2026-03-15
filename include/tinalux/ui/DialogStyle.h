#pragma once

#include "tinalux/core/Geometry.h"
#include "tinalux/ui/ColorScheme.h"
#include "tinalux/ui/Spacing.h"
#include "tinalux/ui/Typography.h"

namespace tinalux::ui {

struct DialogStyle {
    core::Color backdropColor = core::colorARGB(160, 0, 0, 0);
    core::Color backgroundColor = core::colorRGB(32, 35, 47);
    core::Color titleColor = core::colorRGB(235, 239, 248);
    TextStyle titleTextStyle { .fontSize = 28.0f, .lineHeight = 1.25f, .bold = true };
    float cornerRadius = 20.0f;
    float padding = 20.0f;
    float titleGap = 12.0f;
    core::Size maxSize = core::Size::Make(520.0f, 420.0f);

    static DialogStyle standard(
        const ColorScheme& colors,
        const Typography& typography,
        const Spacing& spacing);
};

}  // namespace tinalux::ui
