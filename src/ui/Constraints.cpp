#include "tinalux/ui/Constraints.h"

#include <algorithm>
#include <cmath>

namespace tinalux::ui {

namespace {

float sanitizeDimension(float value)
{
    if (std::isnan(value)) {
        return 0.0f;
    }
    if (std::isinf(value)) {
        return value;
    }
    return std::max(0.0f, value);
}

float clampDimension(float value, float minValue, float maxValue)
{
    const float safeMin = sanitizeDimension(minValue);
    const float safeMax = std::max(safeMin, sanitizeDimension(maxValue));
    if (std::isinf(value)) {
        return safeMax;
    }
    return std::clamp(sanitizeDimension(value), safeMin, safeMax);
}

}  // namespace

Constraints Constraints::tight(float width, float height)
{
    const float safeWidth = sanitizeDimension(width);
    const float safeHeight = sanitizeDimension(height);
    return {
        .minWidth = safeWidth,
        .maxWidth = safeWidth,
        .minHeight = safeHeight,
        .maxHeight = safeHeight,
    };
}

Constraints Constraints::loose(float maxWidth, float maxHeight)
{
    return {
        .minWidth = 0.0f,
        .maxWidth = sanitizeDimension(maxWidth),
        .minHeight = 0.0f,
        .maxHeight = sanitizeDimension(maxHeight),
    };
}

Constraints Constraints::unbounded()
{
    return {};
}

Constraints Constraints::withMaxWidth(float width) const
{
    Constraints updated = *this;
    updated.maxWidth = sanitizeDimension(width);
    updated.minWidth = std::min(updated.minWidth, updated.maxWidth);
    return updated;
}

Constraints Constraints::withMaxHeight(float height) const
{
    Constraints updated = *this;
    updated.maxHeight = sanitizeDimension(height);
    updated.minHeight = std::min(updated.minHeight, updated.maxHeight);
    return updated;
}

SkSize Constraints::constrain(SkSize size) const
{
    return SkSize::Make(
        clampDimension(size.width(), minWidth, maxWidth),
        clampDimension(size.height(), minHeight, maxHeight));
}

bool Constraints::isTight() const
{
    constexpr float kTolerance = 0.001f;
    return std::abs(minWidth - maxWidth) <= kTolerance
        && std::abs(minHeight - maxHeight) <= kTolerance;
}

}  // namespace tinalux::ui
