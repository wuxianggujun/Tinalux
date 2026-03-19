#include "tinalux/ui/Label.h"

#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Theme.h"
#include "../TextPrimitives.h"

namespace tinalux::ui {

Label::Label(std::string text)
    : text_(std::move(text))
{
}

void Label::setText(const std::string& text)
{
    if (text_ == text) {
        return;
    }

    text_ = text;
    markLayoutDirty();
}

void Label::setFontSize(float size)
{
    const float clampedSize = std::max(size, 1.0f);
    if (fontSize_ == clampedSize) {
        return;
    }

    fontSize_ = clampedSize;
    markLayoutDirty();
}

void Label::setColor(core::Color color)
{
    if (color_ && *color_ == color) {
        return;
    }

    color_ = color;
    markPaintDirty();
}

void Label::clearColor()
{
    if (!color_.has_value()) {
        return;
    }

    color_.reset();
    markPaintDirty();
}

core::Size Label::measure(const Constraints& constraints)
{
    const TextMetrics metrics = measureTextMetrics(text_, fontSize_);
    return constraints.constrain(core::Size::Make(metrics.width, metrics.height));
}

void Label::onDraw(rendering::Canvas& canvas)
{
    if (!canvas || text_.empty()) {
        return;
    }

    const TextMetrics metrics = measureTextMetrics(text_, fontSize_);
    canvas.drawText(text_, metrics.drawX, metrics.baseline, fontSize_, resolvedColor());
}

core::Color Label::resolvedColor() const
{
    return color_.value_or(resolvedTheme().textColor());
}

}  // namespace tinalux::ui
