#pragma once

#include <algorithm>
#include <cmath>

#include "tinalux/core/Geometry.h"

namespace tinalux::ui::layout_utils {

/// 计算内部区域（减去 padding），并保证不会产生负尺寸。
inline core::Rect innerExtent(const core::Rect& bounds, float padding)
{
    const float left = std::min(bounds.left() + padding, bounds.right());
    const float top = std::min(bounds.top() + padding, bounds.bottom());
    const float right = std::max(left, bounds.right() - padding);
    const float bottom = std::max(top, bounds.bottom() - padding);
    return core::Rect::MakeLTRB(left, top, right, bottom);
}

/// 计算减去双侧 padding 后的可用尺寸。
inline float innerExtent(float extent, float padding)
{
    if (std::isinf(extent)) {
        return extent;
    }
    return std::max(0.0f, extent - padding * 2.0f);
}

}  // namespace tinalux::ui::layout_utils
