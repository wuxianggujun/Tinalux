#include "tinalux/ui/Toggle.h"

#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Theme.h"

#include "../TextPrimitives.h"

namespace tinalux::ui {

namespace {

constexpr float kTrackWidth = 44.0f;
constexpr float kTrackHeight = 24.0f;
constexpr float kThumbRadius = 9.0f;
constexpr float kThumbInset = 3.0f;
constexpr float kLabelGap = 12.0f;
constexpr float kMinimumHeight = 30.0f;

}  // namespace

Toggle::Toggle(std::string label, bool on)
    : label_(std::move(label))
    , on_(on)
{
}

const std::string& Toggle::label() const
{
    return label_;
}

void Toggle::setLabel(const std::string& label)
{
    if (label_ == label) {
        return;
    }

    label_ = label;
    markLayoutDirty();
}

bool Toggle::on() const
{
    return on_;
}

void Toggle::setOn(bool on)
{
    updateOn(on, false);
}

void Toggle::onToggle(std::function<void(bool)> handler)
{
    onToggle_ = std::move(handler);
}

bool Toggle::focusable() const
{
    return true;
}

core::Size Toggle::measure(const Constraints& constraints)
{
    const Theme& theme = resolvedTheme();
    const TextMetrics metrics = measureTextMetrics(label_, theme.fontSize);
    return constraints.constrain(core::Size::Make(
        metrics.width + kLabelGap + kTrackWidth,
        std::max(kMinimumHeight, std::max(metrics.height, kTrackHeight))));
}

void Toggle::onDraw(rendering::Canvas& canvas)
{
    if (!canvas) {
        return;
    }

    const Theme& theme = resolvedTheme();
    const TextMetrics metrics = measureTextMetrics(label_, theme.fontSize);
    const float trackY = (bounds_.height() - kTrackHeight) * 0.5f;
    const float thumbCenterY = bounds_.height() * 0.5f;
    const float thumbCenterX = on_
        ? (kTrackWidth - kThumbInset - kThumbRadius)
        : (kThumbInset + kThumbRadius);

    canvas.drawRoundRect(
        core::Rect::MakeXYWH(0.0f, trackY, kTrackWidth, kTrackHeight),
        kTrackHeight * 0.5f,
        kTrackHeight * 0.5f,
        on_ ? theme.primary : (pressed_ || hovered_ ? theme.background : theme.surface));

    if (focused()) {
        canvas.drawRoundRect(
            core::Rect::MakeXYWH(1.0f, trackY + 1.0f, kTrackWidth - 2.0f, kTrackHeight - 2.0f),
            kTrackHeight * 0.5f,
            kTrackHeight * 0.5f,
            theme.primary,
            rendering::PaintStyle::Stroke,
            2.0f);
    }
    canvas.drawCircle(thumbCenterX, thumbCenterY, kThumbRadius, on_ ? theme.onPrimary : theme.text);
    canvas.drawText(
        label_.c_str(),
        kTrackWidth + kLabelGap + metrics.drawX,
        (bounds_.height() - metrics.height) * 0.5f + metrics.baseline,
        theme.fontSize,
        theme.text);
}

bool Toggle::onEvent(core::Event& event)
{
    switch (event.type()) {
    case core::EventType::MouseEnter:
        if (!hovered_) {
            hovered_ = true;
            markPaintDirty();
        }
        return false;
    case core::EventType::MouseLeave:
        if (hovered_) {
            hovered_ = false;
            markPaintDirty();
        }
        return false;
    case core::EventType::MouseMove: {
        const auto& moveEvent = static_cast<const core::MouseMoveEvent&>(event);
        const core::Point localPoint = globalToLocal(core::Point::Make(
            static_cast<float>(moveEvent.x),
            static_cast<float>(moveEvent.y)));
        const bool inside = containsLocalPoint(localPoint.x(), localPoint.y());
        if (hovered_ != inside) {
            hovered_ = inside;
            markPaintDirty();
        }
        return pressed_;
    }
    case core::EventType::MouseButtonPress: {
        const auto& mouseEvent = static_cast<const core::MouseButtonEvent&>(event);
        const core::Point localPoint = globalToLocal(core::Point::Make(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y)));
        if (mouseEvent.button != core::mouse::kLeft
            || !containsLocalPoint(localPoint.x(), localPoint.y())) {
            return false;
        }

        if (!pressed_) {
            pressed_ = true;
            hovered_ = true;
            markPaintDirty();
        }
        return true;
    }
    case core::EventType::MouseButtonRelease: {
        const auto& mouseEvent = static_cast<const core::MouseButtonEvent&>(event);
        if (mouseEvent.button != core::mouse::kLeft) {
            return false;
        }

        const core::Point localPoint = globalToLocal(core::Point::Make(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y)));
        const bool inside = containsLocalPoint(localPoint.x(), localPoint.y());
        const bool wasPressed = pressed_;
        if (pressed_ || hovered_ != inside) {
            pressed_ = false;
            hovered_ = inside;
            markPaintDirty();
        }
        if (wasPressed && inside) {
            updateOn(!on_, true);
        }
        return wasPressed;
    }
    case core::EventType::KeyPress:
    case core::EventType::KeyRepeat: {
        const auto& keyEvent = static_cast<const core::KeyEvent&>(event);
        if (!focused()) {
            return false;
        }

        if (keyEvent.key == core::keys::kSpace
            || keyEvent.key == core::keys::kEnter
            || keyEvent.key == core::keys::kKpEnter) {
            updateOn(!on_, true);
            return true;
        }
        return false;
    }
    default:
        return false;
    }
}

void Toggle::updateOn(bool on, bool emitCallback)
{
    if (on_ == on) {
        return;
    }

    on_ = on;
    markPaintDirty();
    if (emitCallback && onToggle_) {
        onToggle_(on_);
    }
}

}  // namespace tinalux::ui
