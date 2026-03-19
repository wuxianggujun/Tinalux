#pragma once

#include <algorithm>
#include <cmath>

namespace tinalux::platform::detail {

inline float sanitizeWindowScale(float value)
{
    return std::isfinite(value) && value > 0.0f ? value : 1.0f;
}

inline float resolveWindowDpiScale(
    int windowWidth,
    int windowHeight,
    int framebufferWidth,
    int framebufferHeight,
    float contentScaleX,
    float contentScaleY)
{
    const float framebufferScaleX = windowWidth > 0 && framebufferWidth > 0
        ? sanitizeWindowScale(static_cast<float>(framebufferWidth) / static_cast<float>(windowWidth))
        : 1.0f;
    const float framebufferScaleY = windowHeight > 0 && framebufferHeight > 0
        ? sanitizeWindowScale(static_cast<float>(framebufferHeight) / static_cast<float>(windowHeight))
        : 1.0f;
    const float reportedContentScale =
        (std::max)(sanitizeWindowScale(contentScaleX), sanitizeWindowScale(contentScaleY));

    return (std::max)({ framebufferScaleX, framebufferScaleY, reportedContentScale });
}

}  // namespace tinalux::platform::detail
