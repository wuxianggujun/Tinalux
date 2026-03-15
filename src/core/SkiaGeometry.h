#pragma once

#include "include/core/SkColor.h"
#include "include/core/SkPoint.h"
#include "include/core/SkRect.h"
#include "include/core/SkSize.h"
#include "tinalux/core/Geometry.h"

namespace tinalux::core {

inline constexpr SkColor toSkColor(Color color)
{
    return static_cast<SkColor>(color.value());
}

inline constexpr Color fromSkColor(SkColor color)
{
    return Color(static_cast<std::uint32_t>(color));
}

inline SkPoint toSkPoint(Point point)
{
    return SkPoint::Make(point.x(), point.y());
}

inline Point fromSkPoint(SkPoint point)
{
    return Point::Make(point.x(), point.y());
}

inline SkSize toSkSize(Size size)
{
    return SkSize::Make(size.width(), size.height());
}

inline Size fromSkSize(SkSize size)
{
    return Size::Make(size.width(), size.height());
}

inline SkRect toSkRect(Rect rect)
{
    return SkRect::MakeLTRB(rect.left(), rect.top(), rect.right(), rect.bottom());
}

inline Rect fromSkRect(SkRect rect)
{
    return Rect::MakeLTRB(rect.left(), rect.top(), rect.right(), rect.bottom());
}

}  // namespace tinalux::core
