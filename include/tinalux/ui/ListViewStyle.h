#pragma once

#include "tinalux/ui/ColorScheme.h"
#include "tinalux/ui/Spacing.h"
#include "tinalux/ui/Typography.h"

namespace tinalux::ui {

struct ListViewStyle {
    float padding = 8.0f;
    float spacing = 8.0f;

    static ListViewStyle standard(
        const ColorScheme& colors,
        const Typography& typography,
        const Spacing& spacingScale);
};

}  // namespace tinalux::ui
