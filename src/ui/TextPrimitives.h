#pragma once

#include <cstddef>
#include <string_view>

namespace tinalux::ui {

struct TextMetrics {
    float width = 0.0f;
    float height = 0.0f;
    float baseline = 0.0f;
    float drawX = 0.0f;
};

struct TextMetricsCacheStats {
    std::size_t entryCount = 0;
    std::size_t hitCount = 0;
    std::size_t missCount = 0;
    std::size_t evictionCount = 0;
};

TextMetrics measureTextMetrics(std::string_view text, float fontSize);
void clearTextMetricsCache();
TextMetricsCacheStats textMetricsCacheStats();

}  // namespace tinalux::ui
