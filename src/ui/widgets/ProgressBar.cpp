#include "tinalux/ui/ProgressBar.h"

#include <algorithm>
#include <cmath>

#include "tinalux/ui/Theme.h"
#include "tinalux/rendering/rendering.h"

namespace tinalux::ui {

ProgressBar::ProgressBar(float min, float max)
    : min_(min), max_(max)
{
}

void ProgressBar::setValue(float value)
{
    float newValue = std::clamp(value, min_, max_);
    if (value_ != newValue) {
        value_ = newValue;
        markDirty();
    }
}

void ProgressBar::setRange(float min, float max)
{
    if (min_ != min || max_ != max) {
        min_ = min;
        max_ = max;
        value_ = std::clamp(value_, min_, max_);
        markDirty();
    }
}

void ProgressBar::setIndeterminate(bool indeterminate)
{
    if (indeterminate_ != indeterminate) {
        indeterminate_ = indeterminate;
        markDirty();
    }
}

void ProgressBar::setHeight(float height)
{
    if (height_ != height) {
        height_ = height;
        markLayoutDirty();
    }
}

void ProgressBar::setColor(core::Color color)
{
    if (color_ != color) {
        color_ = color;
        markDirty();
    }
}

void ProgressBar::setBackgroundColor(core::Color bgColor)
{
    if (backgroundColor_ != bgColor) {
        backgroundColor_ = bgColor;
        markDirty();
    }
}

core::Size ProgressBar::measure(const Constraints& constraints)
{
    float width = constraints.maxWidth();
    if (std::isinf(width)) {
        width = 200.0f;  // 默认宽度
    }
    return constraints.constrain(core::Size::Make(width, height_));
}

void ProgressBar::onDraw(rendering::Canvas& canvas)
{
    const float w = bounds().width();
    const float h = bounds().height();
    const float radius = h / 2.0f;
    
    // 绘制背景
    canvas.drawRoundRect(
        core::Rect::MakeWH(w, h),
        radius,
        backgroundColor_
    );
    
    // 绘制进度
    if (!indeterminate_) {
        // 确定进度模式
        float range = max_ - min_;
        if (range > 0) {
            float progress = (value_ - min_) / range;
            float progressWidth = w * progress;
            if (progressWidth > 0) {
                canvas.drawRoundRect(
                    core::Rect::MakeWH(progressWidth, h),
                    radius,
                    color_
                );
            }
        }
    } else {
        // 不确定进度模式：绘制简单的部分填充
        // TODO: 未来可以添加动画效果
        float barWidth = w * 0.3f;
        if (barWidth > 0) {
            canvas.drawRoundRect(
                core::Rect::MakeXYWH(0, 0, barWidth, h),
                radius,
                color_
            );
        }
    }
}

}  // namespace tinalux::ui
