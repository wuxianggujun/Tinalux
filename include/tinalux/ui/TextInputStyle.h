#pragma once

#include "tinalux/core/Geometry.h"
#include "tinalux/ui/ColorScheme.h"
#include "tinalux/ui/Spacing.h"
#include "tinalux/ui/Typography.h"
#include "tinalux/ui/WidgetStyle.h"

namespace tinalux::ui {

struct TextInputStyle {
    StateStyle<core::Color> backgroundColor {
        .normal = core::colorRGB(32, 35, 47),
    };
    StateStyle<core::Color> borderColor {
        .normal = core::colorRGB(88, 126, 196),
    };
    StateStyle<float> borderWidth {
        .normal = 1.0f,
    };
    TextStyle textStyle { .fontSize = 16.0f };
    core::Color textColor = core::colorRGB(235, 239, 248);
    core::Color placeholderColor = core::colorRGB(166, 173, 200);
    core::Color selectionColor = core::colorARGB(96, 137, 180, 250);
    core::Color caretColor = core::colorRGB(137, 180, 250);
    float borderRadius = 16.0f;
    float paddingHorizontal = 16.0f;
    float paddingVertical = 12.0f;
    float selectionCornerRadius = 6.0f;
    float minWidth = 260.0f;
    float minHeight = 0.0f;

    static TextInputStyle standard(
        const ColorScheme& colors,
        const Typography& typography,
        const Spacing& spacing);
};

}  // namespace tinalux::ui
