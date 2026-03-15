#pragma once

#include "tinalux/core/Geometry.h"

namespace tinalux::ui {

struct Theme {
    core::Color background = core::colorRGB(18, 20, 28);
    core::Color surface = core::colorRGB(32, 35, 47);
    core::Color primary = core::colorRGB(137, 180, 250);
    core::Color onPrimary = core::colorRGB(15, 18, 28);
    core::Color text = core::colorRGB(235, 239, 248);
    core::Color textSecondary = core::colorRGB(166, 173, 200);
    core::Color border = core::colorRGB(88, 126, 196);
    float cornerRadius = 16.0f;
    float fontSize = 16.0f;
    float fontSizeLarge = 28.0f;
    float padding = 16.0f;
    float spacing = 12.0f;

    static Theme dark();
    static Theme light();
};

}  // namespace tinalux::ui
