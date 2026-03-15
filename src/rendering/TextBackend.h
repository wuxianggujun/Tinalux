#pragma once

#include <string_view>

namespace tinalux::rendering {

struct TextMetricsResult {
    float width = 0.0f;
    float height = 0.0f;
    float baseline = 0.0f;
    float drawX = 0.0f;
};

TextMetricsResult measureText(std::string_view text, float fontSize);

}  // namespace tinalux::rendering
