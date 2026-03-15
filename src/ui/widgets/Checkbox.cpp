#include "tinalux/ui/Checkbox.h"

#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Theme.h"

#include "../TextPrimitives.h"

namespace tinalux::ui {

namespace {

constexpr float kIndicatorSize = 22.0f;
constexpr float kIndicatorRadius = 6.0f;
constexpr float kLabelGap = 12.0f;
constexpr float kIndicatorStroke = 2.0f;
constexpr float kMinimumHeight = 28.0f;

}  // namespace

Checkbox::Checkbox(std::string label, bool checked)
    : label_(std::move(label))
    , checked_(checked)
{
}

const std::string& Checkbox::label() const
{
    return label_;
}

void Checkbox::setLabel(const std::string& label)
{
    if (label_ == label) {
        return;
    }

    label_ = label;
    markLayoutDirty();
}

bool Checkbox::checked() const
{
    return checked_;
}

void Checkbox::setChecked(bool checked)
{
    updateChecked(checked, false);
}

void Checkbox::onToggle(std::function<void(bool)> handler)
{
    onToggle_ = std::move(handler);
}

bool Checkbox::focusable() const
{
    return true;
}

core::Size Checkbox::measure(const Constraints& constraints)
{
    const Theme& theme = resolvedTheme();
    const TextMetrics metrics = measureTextMetrics(label_, theme.fontSize);
    return constraints.constrain(core::Size::Make(
        kIndicatorSize + kLabelGap + metrics.width,
        std::max(kMinimumHeight, std::max(kIndicatorSize, metrics.height))));
}

void Checkbox::onDraw(rendering::Canvas& canvas)
{
    if (!canvas) {
        return;
    }

    const Theme& theme = resolvedTheme();
    const TextMetrics metrics = measureTextMetrics(label_, theme.fontSize);
    const float indicatorY = (bounds_.height() - kIndicatorSize) * 0.5f;
    const float labelY = (bounds_.height() - metrics.height) * 0.5f + metrics.baseline;

    canvas.drawRoundRect(
        core::Rect::MakeXYWH(0.0f, indicatorY, kIndicatorSize, kIndicatorSize),
        kIndicatorRadius,
        kIndicatorRadius,
        checked_ ? theme.primary : (pressed_ || hovered_ ? theme.background : theme.surface));
    canvas.drawRoundRect(
        core::Rect::MakeXYWH(
            focused() ? 1.0f : 0.5f,
            indicatorY + (focused() ? 1.0f : 0.5f),
            kIndicatorSize - (focused() ? 2.0f : 1.0f),
            kIndicatorSize - (focused() ? 2.0f : 1.0f)),
        kIndicatorRadius,
        kIndicatorRadius,
        (focused() || hovered_) ? theme.primary : theme.border,
        rendering::PaintStyle::Stroke,
        focused() ? kIndicatorStroke : 1.5f);

    if (checked_) {
        const float left = 5.0f;
        const float midX = 9.0f;
        const float midY = indicatorY + 15.0f;
        const float right = 17.0f;
        const float top = indicatorY + 7.0f;
        canvas.drawLine(left, indicatorY + 11.0f, midX, midY, theme.onPrimary, 2.5f, true);
        canvas.drawLine(midX, midY, right, top, theme.onPrimary, 2.5f, true);
    }

    canvas.drawText(
        label_.c_str(),
        kIndicatorSize + kLabelGap + metrics.drawX,
        labelY,
        theme.fontSize,
        theme.text);
}

bool Checkbox::onEvent(core::Event& event)
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
            updateChecked(!checked_, true);
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
            updateChecked(!checked_, true);
            return true;
        }
        return false;
    }
    default:
        return false;
    }
}

void Checkbox::updateChecked(bool checked, bool emitCallback)
{
    if (checked_ == checked) {
        return;
    }

    checked_ = checked;
    markPaintDirty();
    if (emitCallback && onToggle_) {
        onToggle_(checked_);
    }
}

}  // namespace tinalux::ui
