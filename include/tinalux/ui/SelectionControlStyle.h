#pragma once

#include "tinalux/core/Geometry.h"
#include "tinalux/ui/ColorScheme.h"
#include "tinalux/ui/Spacing.h"
#include "tinalux/ui/Typography.h"
#include "tinalux/ui/WidgetStyle.h"

namespace tinalux::ui {

struct CheckboxStyle {
    StateStyle<core::Color> uncheckedBackgroundColor {
        .normal = core::colorRGB(32, 35, 47),
    };
    StateStyle<core::Color> checkedBackgroundColor {
        .normal = core::colorRGB(137, 180, 250),
    };
    StateStyle<core::Color> borderColor {
        .normal = core::colorRGB(88, 126, 196),
    };
    StateStyle<float> borderWidth {
        .normal = 1.5f,
    };
    StateStyle<core::Color> labelColor {
        .normal = core::colorRGB(235, 239, 248),
    };
    TextStyle textStyle { .fontSize = 16.0f };
    core::Color checkmarkColor = core::colorRGB(15, 18, 28);
    float indicatorSize = 22.0f;
    float indicatorRadius = 6.0f;
    float labelGap = 12.0f;
    float checkmarkStrokeWidth = 2.5f;
    float minHeight = 28.0f;

    static CheckboxStyle standard(
        const ColorScheme& colors,
        const Typography& typography,
        const Spacing& spacing);
};

struct RadioStyle {
    StateStyle<core::Color> unselectedBorderColor {
        .normal = core::colorRGB(88, 126, 196),
    };
    StateStyle<core::Color> selectedBorderColor {
        .normal = core::colorRGB(137, 180, 250),
    };
    StateStyle<float> borderWidth {
        .normal = 1.5f,
    };
    StateStyle<core::Color> labelColor {
        .normal = core::colorRGB(235, 239, 248),
    };
    core::Color dotColor = core::colorRGB(137, 180, 250);
    TextStyle textStyle { .fontSize = 16.0f };
    float indicatorSize = 22.0f;
    float innerDotSize = 10.0f;
    float labelGap = 12.0f;
    float minHeight = 28.0f;

    static RadioStyle standard(
        const ColorScheme& colors,
        const Typography& typography,
        const Spacing& spacing);
};

struct ToggleStyle {
    StateStyle<core::Color> offTrackColor {
        .normal = core::colorRGB(32, 35, 47),
    };
    StateStyle<core::Color> onTrackColor {
        .normal = core::colorRGB(137, 180, 250),
    };
    StateStyle<core::Color> offThumbColor {
        .normal = core::colorRGB(235, 239, 248),
    };
    StateStyle<core::Color> onThumbColor {
        .normal = core::colorRGB(15, 18, 28),
    };
    StateStyle<core::Color> trackBorderColor {
        .normal = core::colorARGB(0, 0, 0, 0),
    };
    StateStyle<float> trackBorderWidth {
        .normal = 0.0f,
    };
    StateStyle<core::Color> labelColor {
        .normal = core::colorRGB(235, 239, 248),
    };
    TextStyle textStyle { .fontSize = 16.0f };
    float trackWidth = 44.0f;
    float trackHeight = 24.0f;
    float thumbRadius = 9.0f;
    float thumbInset = 3.0f;
    float labelGap = 12.0f;
    float minHeight = 30.0f;

    static ToggleStyle standard(
        const ColorScheme& colors,
        const Typography& typography,
        const Spacing& spacing);
};

}  // namespace tinalux::ui
