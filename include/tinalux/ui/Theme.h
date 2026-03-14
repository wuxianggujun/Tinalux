#pragma once

#include "include/core/SkColor.h"

namespace tinalux::ui {

struct Theme {
    SkColor background = SkColorSetRGB(18, 20, 28);
    SkColor surface = SkColorSetRGB(32, 35, 47);
    SkColor primary = SkColorSetRGB(137, 180, 250);
    SkColor onPrimary = SkColorSetRGB(15, 18, 28);
    SkColor text = SkColorSetRGB(235, 239, 248);
    SkColor textSecondary = SkColorSetRGB(166, 173, 200);
    SkColor border = SkColorSetRGB(88, 126, 196);
    float cornerRadius = 16.0f;
    float fontSize = 16.0f;
    float fontSizeLarge = 28.0f;
    float padding = 16.0f;
    float spacing = 12.0f;

    static Theme dark();
    static Theme light();
};

const Theme& currentTheme();
void setTheme(Theme theme);

}  // namespace tinalux::ui
