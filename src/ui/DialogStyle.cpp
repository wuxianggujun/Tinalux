#include "tinalux/ui/DialogStyle.h"

#include <algorithm>

namespace tinalux::ui {

DialogStyle DialogStyle::standard(
    const ColorScheme& colors,
    const Typography& typography,
    const Spacing& spacing)
{
    DialogStyle style;
    style.backdropColor = core::colorARGB(160, 0, 0, 0);
    style.backgroundColor = colors.surface;
    style.titleColor = colors.onBackground;
    style.titleTextStyle = typography.h3;
    style.cornerRadius = std::max(20.0f, spacing.radiusXl + spacing.radiusSm);
    style.padding = std::max(20.0f, spacing.md + spacing.radiusSm);
    style.titleGap = std::max(12.0f, spacing.sm + spacing.radiusXs);
    style.maxSize = core::Size::Make(
        std::max(520.0f, spacing.xxl * 10.0f + spacing.md),
        std::max(420.0f, spacing.xxl * 8.0f + spacing.sm));
    return style;
}

}  // namespace tinalux::ui
