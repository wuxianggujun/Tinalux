#pragma once

#include <string>

namespace tinalux::ui {

struct TextStyle {
    std::string fontFamily = "sans-serif";
    float fontSize = 14.0f;
    float lineHeight = 1.5f;
    float letterSpacing = 0.0f;
    bool bold = false;
    bool italic = false;
};

struct Typography {
    TextStyle h1 { .fontSize = 42.0f, .lineHeight = 1.15f, .letterSpacing = -0.5f, .bold = true };
    TextStyle h2 { .fontSize = 36.0f, .lineHeight = 1.2f, .letterSpacing = -0.25f, .bold = true };
    TextStyle h3 { .fontSize = 30.0f, .lineHeight = 1.25f, .bold = true };
    TextStyle h4 { .fontSize = 26.0f, .lineHeight = 1.25f, .bold = true };
    TextStyle h5 { .fontSize = 22.0f, .lineHeight = 1.3f, .bold = true };
    TextStyle h6 { .fontSize = 19.0f, .lineHeight = 1.35f, .bold = true };
    TextStyle body1 { .fontSize = 17.0f, .lineHeight = 1.5f };
    TextStyle body2 { .fontSize = 15.0f, .lineHeight = 1.5f };
    TextStyle caption { .fontSize = 13.0f, .lineHeight = 1.4f };
    TextStyle button { .fontSize = 17.0f, .lineHeight = 1.25f, .letterSpacing = 0.15f, .bold = true };

    static Typography defaultTypography();
};

}  // namespace tinalux::ui
