#include "tinalux/ui/Checkbox.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
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
    markDirty();
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

SkSize Checkbox::measure(const Constraints& constraints)
{
    const Theme& theme = currentTheme();
    const TextMetrics metrics = measureTextMetrics(label_, theme.fontSize);
    return constraints.constrain(SkSize::Make(
        kIndicatorSize + kLabelGap + metrics.width,
        std::max(kMinimumHeight, std::max(kIndicatorSize, metrics.height))));
}

void Checkbox::onDraw(SkCanvas* canvas)
{
    if (canvas == nullptr) {
        return;
    }

    const Theme& theme = currentTheme();
    const TextMetrics metrics = measureTextMetrics(label_, theme.fontSize);
    const float indicatorY = (bounds_.height() - kIndicatorSize) * 0.5f;
    const float labelY = (bounds_.height() - metrics.height) * 0.5f + metrics.baseline;

    SkPaint indicatorFill;
    indicatorFill.setAntiAlias(true);
    indicatorFill.setColor(
        checked_
            ? theme.primary
            : (pressed_ || hovered_ ? theme.background : theme.surface));
    canvas->drawRRect(
        SkRRect::MakeRectXY(
            SkRect::MakeXYWH(0.0f, indicatorY, kIndicatorSize, kIndicatorSize),
            kIndicatorRadius,
            kIndicatorRadius),
        indicatorFill);

    SkPaint indicatorStroke;
    indicatorStroke.setAntiAlias(true);
    indicatorStroke.setStyle(SkPaint::kStroke_Style);
    indicatorStroke.setStrokeWidth(focused() ? kIndicatorStroke : 1.5f);
    indicatorStroke.setColor((focused() || hovered_) ? theme.primary : theme.border);
    canvas->drawRRect(
        SkRRect::MakeRectXY(
            SkRect::MakeXYWH(
                focused() ? 1.0f : 0.5f,
                indicatorY + (focused() ? 1.0f : 0.5f),
                kIndicatorSize - (focused() ? 2.0f : 1.0f),
                kIndicatorSize - (focused() ? 2.0f : 1.0f)),
            kIndicatorRadius,
            kIndicatorRadius),
        indicatorStroke);

    if (checked_) {
        SkPaint checkPaint;
        checkPaint.setAntiAlias(true);
        checkPaint.setColor(theme.onPrimary);
        checkPaint.setStrokeWidth(2.5f);
        checkPaint.setStrokeCap(SkPaint::kRound_Cap);

        const float left = 5.0f;
        const float midX = 9.0f;
        const float midY = indicatorY + 15.0f;
        const float right = 17.0f;
        const float top = indicatorY + 7.0f;
        canvas->drawLine(left, indicatorY + 11.0f, midX, midY, checkPaint);
        canvas->drawLine(midX, midY, right, top, checkPaint);
    }

    SkFont font;
    font.setSize(theme.fontSize);

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(theme.text);
    canvas->drawString(
        label_.c_str(),
        kIndicatorSize + kLabelGap + metrics.drawX,
        labelY,
        font,
        textPaint);
}

bool Checkbox::onEvent(core::Event& event)
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
            markDirty();
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
        return pressed_;
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
        const bool wasPressed = pressed_;
        if (pressed_ || hovered_ != inside) {
            pressed_ = false;
            hovered_ = inside;
            markDirty();
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
    markDirty();
    if (emitCallback && onToggle_) {
        onToggle_(checked_);
    }
}

}  // namespace tinalux::ui
