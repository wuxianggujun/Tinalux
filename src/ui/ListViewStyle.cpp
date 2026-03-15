#include "tinalux/ui/ListViewStyle.h"

#include <algorithm>

namespace tinalux::ui {

ListViewStyle ListViewStyle::standard(
    const ColorScheme& colors,
    const Typography& typography,
    const Spacing& spacingScale)
{
    (void)colors;
    (void)typography;

    ListViewStyle style;
    style.padding = std::max(8.0f, spacingScale.sm);
    style.spacing = std::max(8.0f, spacingScale.sm);
    return style;
}

}  // namespace tinalux::ui
