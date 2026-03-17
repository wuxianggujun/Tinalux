#include "tinalux/ui/ButtonStyle.h"

#include <algorithm>
#include <cmath>

namespace tinalux::ui {

namespace {

float clampUnit(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

core::Color mixColor(core::Color lhs, core::Color rhs, float t)
{
    const float progress = clampUnit(t);
    const auto mixChannel = [progress](core::Color::Channel from, core::Color::Channel to) {
        return static_cast<core::Color::Channel>(std::lround(
            static_cast<float>(from)
            + (static_cast<float>(to) - static_cast<float>(from)) * progress));
    };

    return core::colorARGB(
        mixChannel(lhs.alpha(), rhs.alpha()),
        mixChannel(lhs.red(), rhs.red()),
        mixChannel(lhs.green(), rhs.green()),
        mixChannel(lhs.blue(), rhs.blue()));
}

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
    style.backgroundColor.normal = colors.primary;
    style.backgroundColor.hovered = mixColor(colors.primary, colors.primaryVariant, 0.18f);
    style.backgroundColor.pressed = colors.primaryVariant;
    style.backgroundColor.focused = style.backgroundColor.hovered;

    style.textColor.normal = colors.onPrimary;
    style.textColor.hovered = colors.onPrimary;
    style.textColor.pressed = colors.onPrimary;
    style.textColor.focused = colors.onPrimary;

    style.borderColor.normal = mixColor(colors.primaryVariant, colors.surfaceVariant, 0.18f);
    style.borderColor.hovered = colors.primaryVariant;
    style.borderColor.pressed = colors.primaryVariant;
    style.borderColor.focused = colors.primaryVariant;
    style.borderWidth.normal = 1.0f;
    style.borderWidth.hovered = 1.0f;
    style.borderWidth.pressed = 1.0f;
    style.borderWidth.focused = 1.0f;
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
