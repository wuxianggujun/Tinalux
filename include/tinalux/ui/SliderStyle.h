#pragma once

#include "tinalux/core/Geometry.h"
#include "tinalux/ui/ColorScheme.h"
#include "tinalux/ui/Spacing.h"
#include "tinalux/ui/Typography.h"
#include "tinalux/ui/WidgetStyle.h"

namespace tinalux::ui {

struct SliderStyle {
    StateStyle<core::Color> trackColor {
        .normal = core::colorRGB(32, 35, 47),
    };
    StateStyle<core::Color> fillColor {
        .normal = core::colorRGB(137, 180, 250),
    };
    StateStyle<core::Color> thumbColor {
        .normal = core::colorRGB(88, 126, 196),
    };
    StateStyle<core::Color> thumbInnerColor {
        .normal = core::colorRGB(15, 18, 28),
    };
    core::Color focusHaloColor = core::colorARGB(72, 137, 180, 250);
    float preferredWidth = 240.0f;
    float preferredHeight = 32.0f;
    float horizontalInset = 12.0f;
    float trackHeight = 4.0f;
    float thumbRadius = 10.0f;
    float activeThumbRadius = 12.0f;
    float focusHaloPadding = 4.0f;
    float innerThumbInset = 5.0f;

    static SliderStyle standard(
        const ColorScheme& colors,
        const Typography& typography,
        const Spacing& spacing);
};

}  // namespace tinalux::ui
