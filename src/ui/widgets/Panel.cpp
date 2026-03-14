#include "tinalux/ui/Panel.h"

#include <algorithm>

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"

namespace tinalux::ui {

void Panel::setBackgroundColor(SkColor color)
{
    if (backgroundColor_ == color) {
        return;
    }

    backgroundColor_ = color;
    markDirty();
}

void Panel::setCornerRadius(float radius)
{
    const float clampedRadius = std::max(radius, 0.0f);
    if (cornerRadius_ == clampedRadius) {
        return;
    }

    cornerRadius_ = clampedRadius;
    markDirty();
}

void Panel::onDraw(SkCanvas* canvas)
{
    if (canvas == nullptr) {
        return;
    }

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(backgroundColor_);

    const SkRRect panelShape = SkRRect::MakeRectXY(
        SkRect::MakeWH(bounds_.width(), bounds_.height()),
        cornerRadius_,
        cornerRadius_);
    canvas->drawRRect(panelShape, paint);

    drawChildren(canvas);
}

}  // namespace tinalux::ui
