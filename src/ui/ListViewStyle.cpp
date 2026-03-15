#include "tinalux/ui/ListViewStyle.h"

#include <algorithm>

namespace tinalux::ui {

ListViewStyle ListViewStyle::standard(
    const ColorScheme& colors,
    const Typography& typography,
    const Spacing& spacingScale)
{
    (void)typography;

    ListViewStyle style;
    style.padding = std::max(8.0f, spacingScale.sm);
    style.spacing = std::max(8.0f, spacingScale.sm);
    style.itemCornerRadius = std::max(10.0f, spacingScale.md);
    style.selectionFillColor = core::colorARGB(
        28,
        colors.primary.red(),
        colors.primary.green(),
        colors.primary.blue());
    style.selectionBorderColor = core::colorARGB(
        168,
        colors.border.red(),
        colors.border.green(),
        colors.border.blue());
    style.focusBorderColor = core::colorARGB(
        220,
        colors.primary.red(),
        colors.primary.green(),
        colors.primary.blue());
    return style;
}

}  // namespace tinalux::ui
