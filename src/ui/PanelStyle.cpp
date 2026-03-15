#include "tinalux/ui/PanelStyle.h"

#include <algorithm>

namespace tinalux::ui {

PanelStyle PanelStyle::standard(
    const ColorScheme& colors,
    const Typography& typography,
    const Spacing& spacing)
{
    (void)typography;

    PanelStyle style;
    style.backgroundColor = colors.surface;
    style.cornerRadius = std::max(18.0f, spacing.radiusXl + spacing.radiusSm * 0.5f);
    return style;
}

}  // namespace tinalux::ui
