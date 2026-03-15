#pragma once

#include "tinalux/core/Geometry.h"
#include "tinalux/ui/ColorScheme.h"
#include "tinalux/ui/Spacing.h"
#include "tinalux/ui/Typography.h"
#include "tinalux/ui/WidgetStyle.h"

namespace tinalux::ui {

struct ButtonStyle {
    StateStyle<core::Color> backgroundColor {
        .normal = core::colorRGB(32, 35, 47),
    };
    StateStyle<core::Color> textColor {
        .normal = core::colorRGB(235, 239, 248),
    };
    StateStyle<core::Color> borderColor {
        .normal = core::colorARGB(0, 0, 0, 0),
    };
    StateStyle<float> borderWidth {
        .normal = 0.0f,
    };
    TextStyle textStyle { .fontSize = 16.0f, .lineHeight = 1.25f, .letterSpacing = 0.15f, .bold = true };
    float borderRadius = 16.0f;
    float paddingHorizontal = 16.0f;
    float paddingVertical = 10.0f;
    float iconSpacing = 8.0f;
    float minWidth = 64.0f;
    float minHeight = 0.0f;
    core::Color focusRingColor = core::colorRGB(137, 180, 250);
    float focusRingWidth = 2.0f;

    static ButtonStyle primary(
        const ColorScheme& colors,
        const Typography& typography,
        const Spacing& spacing);
    static ButtonStyle secondary(
        const ColorScheme& colors,
        const Typography& typography,
        const Spacing& spacing);
    static ButtonStyle outlined(
        const ColorScheme& colors,
        const Typography& typography,
        const Spacing& spacing);
    static ButtonStyle text(
        const ColorScheme& colors,
        const Typography& typography,
        const Spacing& spacing);
};

}  // namespace tinalux::ui
