#pragma once

#include <string_view>

#include "TextPrimitives.h"

namespace tinalux::ui {

TextMetrics measureTextMetricsBackend(std::string_view text, float fontSize);

}  // namespace tinalux::ui
