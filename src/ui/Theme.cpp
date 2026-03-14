#include "tinalux/ui/Theme.h"

namespace tinalux::ui {

namespace {

Theme gTheme = Theme::dark();

}  // namespace

Theme Theme::dark()
{
    return {};
}

Theme Theme::light()
{
    return {
        .background = SkColorSetRGB(242, 244, 248),
        .surface = SkColorSetRGB(255, 255, 255),
        .primary = SkColorSetRGB(32, 99, 245),
        .onPrimary = SkColorSetRGB(255, 255, 255),
        .text = SkColorSetRGB(25, 32, 44),
        .textSecondary = SkColorSetRGB(92, 104, 126),
        .border = SkColorSetRGB(173, 184, 204),
        .cornerRadius = 16.0f,
        .fontSize = 16.0f,
        .fontSizeLarge = 28.0f,
        .padding = 16.0f,
        .spacing = 12.0f,
    };
}

const Theme& currentTheme()
{
    return gTheme;
}

void setTheme(Theme theme)
{
    gTheme = theme;
}

}  // namespace tinalux::ui
