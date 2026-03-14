#include "tinalux/ui/Button.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/ui/Theme.h"

#include "../TextPrimitives.h"

namespace tinalux::ui {

Button::Button(std::string label)
    : label_(std::move(label))
{
}

void Button::setLabel(const std::string& label)
{
    if (label_ == label) {
        return;
    }

    label_ = label;
    markDirty();
}

void Button::onClick(std::function<void()> handler)
{
    onClick_ = std::move(handler);
}

bool Button::focusable() const
{
    return true;
}

SkSize Button::measure(const Constraints& constraints)
{
    const Theme& theme = currentTheme();
    constexpr float kVerticalPadding = 10.0f;

    const TextMetrics metrics = measureTextMetrics(label_, theme.fontSize);
    return constraints.constrain(SkSize::Make(
        metrics.width + theme.padding * 2.0f,
        metrics.height + kVerticalPadding * 2.0f));
}

void Button::onDraw(SkCanvas* canvas)
{
    if (canvas == nullptr) {
        return;
    }

    const Theme& theme = currentTheme();
    const SkColor backgroundColor = pressed_
        ? theme.primary
        : hovered_
            ? theme.border
            : theme.surface;
    const SkColor textColor = pressed_
        ? theme.onPrimary
        : theme.text;

    SkPaint fillPaint;
    fillPaint.setAntiAlias(true);
    fillPaint.setColor(backgroundColor);
    canvas->drawRRect(
        SkRRect::MakeRectXY(
            SkRect::MakeWH(bounds_.width(), bounds_.height()),
            theme.cornerRadius,
            theme.cornerRadius),
        fillPaint);

    if (focused()) {
        SkPaint strokePaint;
        strokePaint.setAntiAlias(true);
        strokePaint.setStyle(SkPaint::kStroke_Style);
        strokePaint.setStrokeWidth(2.0f);
        strokePaint.setColor(theme.primary);
        canvas->drawRRect(
            SkRRect::MakeRectXY(
                SkRect::MakeXYWH(1.0f, 1.0f, bounds_.width() - 2.0f, bounds_.height() - 2.0f),
                theme.cornerRadius - 1.0f,
                theme.cornerRadius - 1.0f),
            strokePaint);
    }

    const TextMetrics metrics = measureTextMetrics(label_, theme.fontSize);
    const float textX = (bounds_.width() - metrics.width) * 0.5f + metrics.drawX;
    const float textY = (bounds_.height() - metrics.height) * 0.5f + metrics.baseline;

    SkFont font;
    font.setSize(theme.fontSize);

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(textColor);
    canvas->drawString(label_.c_str(), textX, textY, font, textPaint);
}

bool Button::onEvent(core::Event& event)
{
    switch (event.type()) {
    case core::EventType::MouseEnter:
        if (!hovered_) {
            hovered_ = true;
            markDirty();
        }
        return false;
    case core::EventType::MouseLeave:
        if (hovered_) {
            hovered_ = false;
            if (!pressed_) {
                markDirty();
            }
        }
        return false;
    case core::EventType::MouseMove: {
        const auto& moveEvent = static_cast<const core::MouseMoveEvent&>(event);
        const bool inside = containsGlobalPoint(
            static_cast<float>(moveEvent.x),
            static_cast<float>(moveEvent.y));
        if (hovered_ != inside) {
            hovered_ = inside;
            markDirty();
        }
        return false;
    }
    case core::EventType::MouseButtonPress: {
        const auto& mouseEvent = static_cast<const core::MouseButtonEvent&>(event);
        if (mouseEvent.button != core::mouse::kLeft
            || !containsGlobalPoint(static_cast<float>(mouseEvent.x), static_cast<float>(mouseEvent.y))) {
            return false;
        }

        if (!pressed_) {
            pressed_ = true;
            hovered_ = true;
            markDirty();
        }
        return true;
    }
    case core::EventType::MouseButtonRelease: {
        const auto& mouseEvent = static_cast<const core::MouseButtonEvent&>(event);
        if (mouseEvent.button != core::mouse::kLeft) {
            return false;
        }

        const bool inside = containsGlobalPoint(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y));
        const bool shouldClick = pressed_ && inside;
        if (pressed_ || hovered_ != inside) {
            pressed_ = false;
            hovered_ = inside;
            markDirty();
        }
        if (shouldClick && onClick_) {
            onClick_();
        }
        return shouldClick || inside;
    }
    case core::EventType::KeyPress:
    case core::EventType::KeyRepeat: {
        const auto& keyEvent = static_cast<const core::KeyEvent&>(event);
        if (!focused()) {
            return false;
        }

        if (keyEvent.key == core::keys::kEnter
            || keyEvent.key == core::keys::kKpEnter
            || keyEvent.key == core::keys::kSpace) {
            if (onClick_) {
                onClick_();
            }
            pressed_ = false;
            hovered_ = false;
            markDirty();
            return true;
        }
        return false;
    }
    default:
        return false;
    }
}

}  // namespace tinalux::ui
