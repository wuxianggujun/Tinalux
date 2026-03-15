#pragma once

#include "tinalux/core/Geometry.h"

namespace tinalux::ui {

struct ColorScheme {
    core::Color primary = core::colorRGB(137, 180, 250);
    core::Color primaryVariant = core::colorRGB(88, 126, 196);
    core::Color secondary = core::colorRGB(203, 166, 247);
    core::Color secondaryVariant = core::colorRGB(148, 91, 214);

    core::Color background = core::colorRGB(18, 20, 28);
    core::Color surface = core::colorRGB(32, 35, 47);
    core::Color surfaceVariant = core::colorRGB(44, 48, 62);

    core::Color onPrimary = core::colorRGB(15, 18, 28);
    core::Color onSecondary = core::colorRGB(18, 20, 28);
    core::Color onBackground = core::colorRGB(235, 239, 248);
    core::Color onSurface = core::colorRGB(235, 239, 248);

    core::Color error = core::colorRGB(255, 107, 107);
    core::Color warning = core::colorRGB(255, 184, 108);
    core::Color success = core::colorRGB(127, 219, 168);
    core::Color info = core::colorRGB(137, 180, 250);

    core::Color border = core::colorRGB(88, 126, 196);
    core::Color divider = core::colorRGB(60, 72, 96);
    core::Color shadow = core::colorARGB(96, 0, 0, 0);

    static ColorScheme dark();
    static ColorScheme light();
    static ColorScheme custom(core::Color primary);
};

}  // namespace tinalux::ui
