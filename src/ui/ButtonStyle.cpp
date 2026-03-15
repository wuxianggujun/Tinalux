#include "tinalux/ui/ButtonStyle.h"

#include <algorithm>

namespace tinalux::ui {

namespace {

float resolveVerticalPadding(const Spacing& spacing)
{
    return spacing.sm + spacing.radiusXs;
}

ButtonStyle makeBaseButtonStyle(
    const ColorScheme& colors,
    const Typography& typography,
    const Spacing& spacing)
{
    ButtonStyle style;
    style.textStyle = typography.button;
    style.borderRadius = spacing.radiusXl;
    style.paddingHorizontal = spacing.md;
    style.paddingVertical = resolveVerticalPadding(spacing);
    style.iconSpacing = spacing.sm;
    style.minWidth = std::max(64.0f, spacing.xl * 2.0f);
    style.minHeight = style.textStyle.fontSize + style.paddingVertical * 2.0f;
    style.focusRingColor = colors.primary;
    style.focusRingWidth = 2.0f;
    return style;
}

}  // namespace

ButtonStyle ButtonStyle::primary(
    const ColorScheme& colors,
    const Typography& typography,
    const Spacing& spacing)
{
    ButtonStyle style = makeBaseButtonStyle(colors, typography, spacing);
    style.backgroundColor.normal = colors.surface;
    style.backgroundColor.hovered = colors.border;
    style.backgroundColor.pressed = colors.primary;
    style.backgroundColor.focused = colors.primaryVariant;

    style.textColor.normal = colors.onSurface;
    style.textColor.hovered = colors.onSurface;
    style.textColor.pressed = colors.onPrimary;
    style.textColor.focused = colors.onPrimary;

    style.borderColor.normal = core::colorARGB(0, 0, 0, 0);
    style.borderWidth.normal = 0.0f;
    return style;
}

ButtonStyle ButtonStyle::secondary(
    const ColorScheme& colors,
    const Typography& typography,
    const Spacing& spacing)
{
    ButtonStyle style = makeBaseButtonStyle(colors, typography, spacing);
    style.backgroundColor.normal = colors.surfaceVariant;
    style.backgroundColor.hovered = colors.secondaryVariant;
    style.backgroundColor.pressed = colors.secondary;

    style.textColor.normal = colors.onSurface;
    style.textColor.hovered = colors.onSecondary;
    style.textColor.pressed = colors.onSecondary;
    return style;
}

ButtonStyle ButtonStyle::outlined(
    const ColorScheme& colors,
    const Typography& typography,
    const Spacing& spacing)
{
    ButtonStyle style = makeBaseButtonStyle(colors, typography, spacing);
    style.backgroundColor.normal = colors.surface;
    style.backgroundColor.hovered = colors.surfaceVariant;
    style.backgroundColor.pressed = colors.surfaceVariant;

    style.textColor.normal = colors.primary;
    style.textColor.hovered = colors.primary;
    style.textColor.pressed = colors.primary;

    style.borderColor.normal = colors.border;
    style.borderColor.hovered = colors.primary;
    style.borderColor.pressed = colors.primaryVariant;
    style.borderWidth.normal = 1.0f;
    style.borderWidth.hovered = 1.5f;
    style.borderWidth.pressed = 1.5f;
    return style;
}

ButtonStyle ButtonStyle::text(
    const ColorScheme& colors,
    const Typography& typography,
    const Spacing& spacing)
{
    ButtonStyle style = makeBaseButtonStyle(colors, typography, spacing);
    style.backgroundColor.normal = core::colorARGB(0, 0, 0, 0);
    style.backgroundColor.hovered = colors.surfaceVariant;
    style.backgroundColor.pressed = colors.border;

    style.textColor.normal = colors.primary;
    style.textColor.hovered = colors.primary;
    style.textColor.pressed = colors.onSurface;

    style.focusRingWidth = 1.5f;
    style.minWidth = 0.0f;
    return style;
}

}  // namespace tinalux::ui
