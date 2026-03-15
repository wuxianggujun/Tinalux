#include "tinalux/ui/ScrollViewStyle.h"

#include <algorithm>

namespace tinalux::ui {

ScrollViewStyle ScrollViewStyle::standard(
    const ColorScheme& colors,
    const Typography& typography,
    const Spacing& spacing)
{
    (void)typography;

    ScrollViewStyle style;
    style.scrollbarThumbColor = core::colorARGB(
        120,
        core::colorRed(colors.onSurface),
        core::colorGreen(colors.onSurface),
        core::colorBlue(colors.onSurface));
    style.scrollbarTrackColor = core::colorARGB(
        32,
        core::colorRed(colors.onSurface),
        core::colorGreen(colors.onSurface),
        core::colorBlue(colors.onSurface));
    style.scrollbarMargin = std::max(6.0f, spacing.radiusSm + spacing.radiusXs);
    style.scrollbarWidth = std::max(6.0f, spacing.radiusMd);
    style.minThumbHeight = std::max(24.0f, spacing.xl);
    style.scrollStep = std::max(48.0f, spacing.xxl);
    return style;
}

}  // namespace tinalux::ui
