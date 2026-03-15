#include "tinalux/ui/SliderStyle.h"

#include <algorithm>

namespace tinalux::ui {

SliderStyle SliderStyle::standard(
    const ColorScheme& colors,
    const Typography& typography,
    const Spacing& spacing)
{
    (void)typography;

    SliderStyle style;
    style.trackColor.normal = colors.surface;

    style.fillColor.normal = colors.primary;
    style.fillColor.hovered = colors.primary;
    style.fillColor.pressed = colors.primaryVariant;
    style.fillColor.focused = colors.primary;

    style.thumbColor.normal = colors.border;
    style.thumbColor.hovered = colors.primary;
    style.thumbColor.pressed = colors.primary;
    style.thumbColor.focused = colors.primary;

    style.thumbInnerColor.normal = colors.onPrimary;
    style.focusHaloColor = core::colorARGB(
        72,
        core::colorRed(colors.primary),
        core::colorGreen(colors.primary),
        core::colorBlue(colors.primary));
    style.preferredWidth = std::max(240.0f, spacing.xxl * 5.0f);
    style.preferredHeight = std::max(32.0f, spacing.xl + spacing.xs);
    style.horizontalInset = std::max(12.0f, spacing.md - spacing.radiusXs);
    style.trackHeight = std::max(4.0f, spacing.radiusSm);
    style.thumbRadius = std::max(10.0f, spacing.md * 0.625f);
    style.activeThumbRadius = std::max(style.thumbRadius + 2.0f, spacing.md * 0.75f);
    style.focusHaloPadding = std::max(4.0f, spacing.xs);
    style.innerThumbInset = std::max(5.0f, spacing.sm - spacing.radiusXs);
    return style;
}

}  // namespace tinalux::ui
