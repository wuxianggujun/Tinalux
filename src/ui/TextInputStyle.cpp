#include "tinalux/ui/TextInputStyle.h"

#include <algorithm>

namespace tinalux::ui {

TextInputStyle TextInputStyle::standard(
    const ColorScheme& colors,
    const Typography& typography,
    const Spacing& spacing)
{
    TextInputStyle style;
    style.backgroundColor.normal = colors.surface;
    style.backgroundColor.focused = colors.surface;

    style.borderColor.normal = colors.border;
    style.borderColor.hovered = colors.primaryVariant;
    style.borderColor.focused = colors.primary;

    style.borderWidth.normal = 1.0f;
    style.borderWidth.focused = 2.0f;

    style.textStyle = typography.body1;
    style.textColor = colors.onBackground;
    style.placeholderColor = colors.onSurface;
    style.selectionColor = core::colorARGB(
        96,
        core::colorRed(colors.primary),
        core::colorGreen(colors.primary),
        core::colorBlue(colors.primary));
    style.caretColor = colors.primary;
    style.borderRadius = spacing.radiusXl;
    style.paddingHorizontal = spacing.md;
    style.paddingVertical = spacing.md - spacing.xs;
    style.selectionCornerRadius = spacing.radiusLg * 0.5f;
    style.minWidth = std::max(260.0f, spacing.xxl * 5.0f + spacing.sm);
    style.minHeight = style.textStyle.fontSize + style.paddingVertical * 2.0f;
    return style;
}

}  // namespace tinalux::ui
