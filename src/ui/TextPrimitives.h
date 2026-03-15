#pragma once

#include <string_view>

namespace tinalux::ui {

struct TextMetrics {
    float width = 0.0f;
    float height = 0.0f;
    float baseline = 0.0f;
    float drawX = 0.0f;
};

TextMetrics measureTextMetrics(std::string_view text, float fontSize);

}  // namespace tinalux::ui
