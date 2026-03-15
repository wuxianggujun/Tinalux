#pragma once

#include "tinalux/ui/ColorScheme.h"
#include "tinalux/ui/Spacing.h"
#include "tinalux/ui/Typography.h"

namespace tinalux::ui {

struct ListViewStyle {
    float padding = 8.0f;
    float spacing = 8.0f;
    float itemCornerRadius = 12.0f;
    core::Color selectionFillColor = core::colorARGB(28, 137, 180, 250);
    core::Color selectionBorderColor = core::colorARGB(168, 88, 126, 196);
    core::Color focusBorderColor = core::colorARGB(220, 137, 180, 250);

    static ListViewStyle standard(
        const ColorScheme& colors,
        const Typography& typography,
        const Spacing& spacingScale);
};

}  // namespace tinalux::ui
