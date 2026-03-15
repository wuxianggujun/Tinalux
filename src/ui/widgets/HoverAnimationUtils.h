#pragma once

#include <algorithm>
#include <cmath>

#include "tinalux/core/Geometry.h"

namespace tinalux::ui::detail {

inline constexpr double kHoverTransitionDurationSeconds = 0.15;

inline float clampUnit(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

inline float lerpFloat(float from, float to, float progress)
{
    const float t = clampUnit(progress);
    return from + (to - from) * t;
}

inline core::Color lerpColor(core::Color from, core::Color to, float progress)
{
    const float t = clampUnit(progress);
    const auto lerpChannel = [t](core::Color::Channel lhs, core::Color::Channel rhs) {
        return static_cast<core::Color::Channel>(std::lround(
            static_cast<float>(lhs)
            + (static_cast<float>(rhs) - static_cast<float>(lhs)) * t));
    };

    return core::colorARGB(
        lerpChannel(from.alpha(), to.alpha()),
        lerpChannel(from.red(), to.red()),
        lerpChannel(from.green(), to.green()),
        lerpChannel(from.blue(), to.blue()));
}

}  // namespace tinalux::ui::detail
