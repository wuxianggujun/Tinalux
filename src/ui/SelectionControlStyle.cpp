#include "tinalux/ui/SelectionControlStyle.h"

#include <algorithm>

namespace tinalux::ui {

CheckboxStyle CheckboxStyle::standard(
    const ColorScheme& colors,
    const Typography& typography,
    const Spacing& spacing)
{
    CheckboxStyle style;
    style.uncheckedBackgroundColor.normal = colors.surface;
    style.uncheckedBackgroundColor.hovered = colors.background;
    style.uncheckedBackgroundColor.pressed = colors.background;

    style.checkedBackgroundColor.normal = colors.primary;
    style.checkedBackgroundColor.hovered = colors.primary;
    style.checkedBackgroundColor.pressed = colors.primaryVariant;
    style.checkedBackgroundColor.focused = colors.primary;

    style.borderColor.normal = colors.border;
    style.borderColor.hovered = colors.primary;
    style.borderColor.pressed = colors.primary;
    style.borderColor.focused = colors.primary;

    style.borderWidth.normal = 1.5f;
    style.borderWidth.focused = 2.0f;

    style.labelColor.normal = colors.onBackground;
    style.textStyle = typography.body1;
    style.checkmarkColor = colors.onPrimary;
    style.indicatorSize = std::max(22.0f, spacing.md + spacing.radiusSm);
    style.indicatorRadius = spacing.radiusLg * 0.5f;
    style.labelGap = spacing.md - spacing.xs;
    style.minHeight = std::max(28.0f, std::max(style.indicatorSize, style.textStyle.fontSize + spacing.sm));
    return style;
}

RadioStyle RadioStyle::standard(
    const ColorScheme& colors,
    const Typography& typography,
    const Spacing& spacing)
{
    RadioStyle style;
    style.unselectedBorderColor.normal = colors.border;
    style.unselectedBorderColor.hovered = colors.primary;
    style.unselectedBorderColor.pressed = colors.primary;
    style.unselectedBorderColor.focused = colors.primary;

    style.selectedBorderColor.normal = colors.primary;
    style.selectedBorderColor.hovered = colors.primary;
    style.selectedBorderColor.pressed = colors.primaryVariant;
    style.selectedBorderColor.focused = colors.primary;

    style.borderWidth.normal = 1.5f;
    style.borderWidth.focused = 2.0f;

    style.labelColor.normal = colors.onBackground;
    style.textStyle = typography.body1;
    style.dotColor = colors.primary;
    style.indicatorSize = std::max(22.0f, spacing.md + spacing.radiusSm);
    style.innerDotSize = std::max(10.0f, spacing.sm + spacing.radiusSm * 0.5f);
    style.labelGap = spacing.md - spacing.xs;
    style.minHeight = std::max(28.0f, std::max(style.indicatorSize, style.textStyle.fontSize + spacing.sm));
    return style;
}

ToggleStyle ToggleStyle::standard(
    const ColorScheme& colors,
    const Typography& typography,
    const Spacing& spacing)
{
    ToggleStyle style;
    style.offTrackColor.normal = colors.surface;
    style.offTrackColor.hovered = colors.background;
    style.offTrackColor.pressed = colors.background;

    style.onTrackColor.normal = colors.primary;
    style.onTrackColor.hovered = colors.primary;
    style.onTrackColor.pressed = colors.primaryVariant;
    style.onTrackColor.focused = colors.primary;

    style.offThumbColor.normal = colors.onSurface;
    style.onThumbColor.normal = colors.onPrimary;

    style.trackBorderColor.normal = core::colorARGB(0, 0, 0, 0);
    style.trackBorderColor.focused = colors.primary;
    style.trackBorderWidth.normal = 0.0f;
    style.trackBorderWidth.focused = 2.0f;

    style.labelColor.normal = colors.onBackground;
    style.textStyle = typography.body1;
    style.trackWidth = std::max(44.0f, spacing.xl + spacing.md - spacing.radiusXs);
    style.trackHeight = std::max(24.0f, spacing.md + spacing.radiusMd);
    style.thumbRadius = std::max(9.0f, style.trackHeight * 0.5f - spacing.xs + 1.0f);
    style.thumbInset = std::max(3.0f, spacing.radiusXs + 1.0f);
    style.labelGap = spacing.md - spacing.xs;
    style.minHeight = std::max(30.0f, std::max(style.trackHeight, style.textStyle.fontSize + spacing.sm));
    return style;
}

}  // namespace tinalux::ui
