#include "tinalux/ui/ProgressBar.h"

#include <algorithm>
#include <cmath>

#include "tinalux/ui/Theme.h"
#include "tinalux/rendering/rendering.h"

namespace tinalux::ui {

namespace {

constexpr float kDefaultProgressBarWidth = 200.0f;

}  // namespace

ProgressBar::ProgressBar(float min, float max)
    : min_(min), max_(max)
{
}

void ProgressBar::setValue(float value)
{
    float newValue = std::clamp(value, min_, max_);
    if (value_ != newValue) {
        value_ = newValue;
        markPaintDirty();
    }
}

void ProgressBar::setRange(float min, float max)
{
    if (min_ != min || max_ != max) {
        min_ = min;
        max_ = max;
        value_ = std::clamp(value_, min_, max_);
        markPaintDirty();
    }
}

void ProgressBar::setIndeterminate(bool indeterminate)
{
    if (indeterminate_ != indeterminate) {
        indeterminate_ = indeterminate;
        markPaintDirty();
    }
}

void ProgressBar::setHeight(float height)
{
    if (height_ != height) {
        height_ = height;
        markLayoutDirty();
    }
}

void ProgressBar::setPreferredWidth(float width)
{
    if (preferredWidth_ != width) {
        preferredWidth_ = width;
        markLayoutDirty();
    }
}

void ProgressBar::setColor(core::Color color)
{
    if (color_ != color) {
        color_ = color;
        markPaintDirty();
    }
}

void ProgressBar::setBackgroundColor(core::Color bgColor)
{
    if (backgroundColor_ != bgColor) {
        backgroundColor_ = bgColor;
        markPaintDirty();
    }
}

core::Size ProgressBar::measure(const Constraints& constraints)
{
    float width = preferredWidth_;
    if (width < 0.0f && std::isfinite(constraints.maxWidth)) {
        width = constraints.maxWidth;
    }
    if (!std::isfinite(width) || width <= 0.0f) {
        width = kDefaultProgressBarWidth;
    }
    return constraints.constrain(core::Size::Make(width, height_));
}

void ProgressBar::onDraw(rendering::Canvas& canvas)
{
    const float w = bounds_.width();
    const float h = bounds_.height();
    const float radius = h / 2.0f;

    canvas.drawRoundRect(
        core::Rect::MakeWH(w, h),
        radius,
        radius,
        backgroundColor_
    );

    if (!indeterminate_) {
        const float range = max_ - min_;
        if (range > 0) {
            const float progress = (value_ - min_) / range;
            const float progressWidth = w * progress;
            if (progressWidth > 0) {
                canvas.drawRoundRect(
                    core::Rect::MakeWH(progressWidth, h),
                    radius,
                    radius,
                    color_
                );
            }
        }
    } else {
        const float barWidth = w * 0.3f;
        if (barWidth > 0) {
            canvas.drawRoundRect(
                core::Rect::MakeXYWH(0.0f, 0.0f, barWidth, h),
                radius,
                radius,
                color_
            );
        }
    }
}

}  // namespace tinalux::ui
