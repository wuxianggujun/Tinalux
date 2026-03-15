#include "tinalux/ui/Theme.h"

namespace tinalux::ui {

Theme Theme::dark()
{
    return {};
}

Theme Theme::light()
{
    return {
        .background = core::colorRGB(242, 244, 248),
        .surface = core::colorRGB(255, 255, 255),
        .primary = core::colorRGB(32, 99, 245),
        .onPrimary = core::colorRGB(255, 255, 255),
        .text = core::colorRGB(25, 32, 44),
        .textSecondary = core::colorRGB(92, 104, 126),
        .border = core::colorRGB(173, 184, 204),
        .cornerRadius = 16.0f,
        .fontSize = 16.0f,
        .fontSizeLarge = 28.0f,
        .padding = 16.0f,
        .spacing = 12.0f,
    };
}

}  // namespace tinalux::ui
