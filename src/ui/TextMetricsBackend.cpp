#include "TextMetricsBackend.h"

#include "../rendering/TextBackend.h"

namespace tinalux::ui {

TextMetrics measureTextMetricsBackend(std::string_view text, float fontSize)
{
    const rendering::TextMetricsResult metrics = rendering::measureText(text, fontSize);
    return TextMetrics {
        .width = metrics.width,
        .height = metrics.height,
        .baseline = metrics.baseline,
        .drawX = metrics.drawX,
    };
}

}  // namespace tinalux::ui
