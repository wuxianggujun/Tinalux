#include "tinalux/ui/Label.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "tinalux/ui/Theme.h"

#include "../TextPrimitives.h"

namespace tinalux::ui {

Label::Label(std::string text)
    : text_(std::move(text))
    , fontSize_(currentTheme().fontSize)
    , color_(currentTheme().text)
{
}

void Label::setText(const std::string& text)
{
    if (text_ == text) {
        return;
    }

    text_ = text;
    markDirty();
}

void Label::setFontSize(float size)
{
    const float clampedSize = std::max(size, 1.0f);
    if (fontSize_ == clampedSize) {
        return;
    }

    fontSize_ = clampedSize;
    markDirty();
}

void Label::setColor(SkColor color)
{
    if (color_ == color) {
        return;
    }

    color_ = color;
    markDirty();
}

SkSize Label::measure(const Constraints& constraints)
{
    const TextMetrics metrics = measureTextMetrics(text_, fontSize_);
    return constraints.constrain(SkSize::Make(metrics.width, metrics.height));
}

void Label::onDraw(SkCanvas* canvas)
{
    if (canvas == nullptr || text_.empty()) {
        return;
    }

    const TextMetrics metrics = measureTextMetrics(text_, fontSize_);

    SkFont font;
    font.setSize(fontSize_);

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(color_);

    canvas->drawString(text_.c_str(), metrics.drawX, metrics.baseline, font, paint);
}

}  // namespace tinalux::ui
