#include "tinalux/ui/Slider.h"

#include <algorithm>
#include <cmath>

#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Theme.h"

namespace tinalux::ui {

namespace {

constexpr float kPreferredWidth = 240.0f;
constexpr float kPreferredHeight = 32.0f;
constexpr float kHorizontalInset = 12.0f;
constexpr float kTrackHeight = 4.0f;
constexpr float kThumbRadius = 10.0f;
constexpr float kFocusedThumbRadius = 12.0f;
constexpr float kValueTolerance = 0.0001f;

bool sameValue(float lhs, float rhs)
{
    return std::abs(lhs - rhs) <= kValueTolerance;
}

}  // namespace

void Slider::setRange(float minimum, float maximum)
{
    const auto [nextMinimum, nextMaximum] = std::minmax(minimum, maximum);
    if (sameValue(minimum_, nextMinimum) && sameValue(maximum_, nextMaximum)) {
        return;
    }

    minimum_ = nextMinimum;
    maximum_ = nextMaximum;
    setValueInternal(value_, false);
    markPaintDirty();
}

float Slider::minimum() const
{
    return minimum_;
}

float Slider::maximum() const
{
    return maximum_;
}

float Slider::value() const
{
    return value_;
}

void Slider::setValue(float value)
{
    setValueInternal(value, false);
}

float Slider::step() const
{
    return step_;
}

void Slider::setStep(float step)
{
    const float clampedStep = std::max(0.0f, step);
    if (sameValue(step_, clampedStep)) {
        return;
    }

    step_ = clampedStep;
    setValueInternal(value_, false);
}

void Slider::onValueChanged(std::function<void(float)> handler)
{
    onValueChanged_ = std::move(handler);
}

bool Slider::focusable() const
{
    return true;
}

core::Size Slider::measure(const Constraints& constraints)
{
    return constraints.constrain(core::Size::Make(kPreferredWidth, kPreferredHeight));
}

void Slider::onDraw(rendering::Canvas& canvas)
{
    if (!canvas) {
        return;
    }

    const Theme& theme = resolvedTheme();
    const float trackLeft = kHorizontalInset;
    const float trackWidth = std::max(1.0f, bounds_.width() - kHorizontalInset * 2.0f);
    const float trackTop = (bounds_.height() - kTrackHeight) * 0.5f;
    const float fraction = maximum_ > minimum_
        ? (value_ - minimum_) / (maximum_ - minimum_)
        : 0.0f;
    const float clampedFraction = std::clamp(fraction, 0.0f, 1.0f);
    const float thumbX = trackLeft + trackWidth * clampedFraction;
    const float thumbRadius = (focused() || dragging_) ? kFocusedThumbRadius : kThumbRadius;

    canvas.drawRoundRect(
        core::Rect::MakeXYWH(trackLeft, trackTop, trackWidth, kTrackHeight),
        kTrackHeight * 0.5f,
        kTrackHeight * 0.5f,
        theme.surface);

    canvas.drawRoundRect(
        core::Rect::MakeXYWH(
            trackLeft,
            trackTop,
            std::max(0.0f, thumbX - trackLeft),
            kTrackHeight),
        kTrackHeight * 0.5f,
        kTrackHeight * 0.5f,
        theme.primary);

    if (focused()) {
        canvas.drawCircle(
            thumbX,
            bounds_.height() * 0.5f,
            thumbRadius + 4.0f,
            core::colorARGB(
                72,
                core::colorRed(theme.primary),
                core::colorGreen(theme.primary),
                core::colorBlue(theme.primary)));
    }

    canvas.drawCircle(
        thumbX,
        bounds_.height() * 0.5f,
        thumbRadius,
        dragging_ || hovered_ ? theme.primary : theme.border);
    canvas.drawCircle(
        thumbX,
        bounds_.height() * 0.5f,
        std::max(4.0f, thumbRadius - 5.0f),
        theme.onPrimary);
}

bool Slider::onEvent(core::Event& event)
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
        const float globalX = static_cast<float>(moveEvent.x);
        const float globalY = static_cast<float>(moveEvent.y);
        const core::Point localPoint = globalToLocal(core::Point::Make(globalX, globalY));
        const bool inside = containsLocalPoint(localPoint.x(), localPoint.y());
        if (hovered_ != inside) {
            hovered_ = inside;
            markPaintDirty();
        }

        if (!dragging_) {
            return false;
        }

        setValueInternal(valueFromLocalX(localPoint.x()), true);
        return true;
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

        dragging_ = true;
        hovered_ = true;
        setValueInternal(valueFromLocalX(localPoint.x()), true);
        markPaintDirty();
        return true;
    }
    case core::EventType::MouseButtonRelease: {
        const auto& mouseEvent = static_cast<const core::MouseButtonEvent&>(event);
        if (mouseEvent.button != core::mouse::kLeft || !dragging_) {
            return false;
        }

        const core::Point localPoint = globalToLocal(core::Point::Make(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y)));
        const bool inside = containsLocalPoint(localPoint.x(), localPoint.y());
        setValueInternal(valueFromLocalX(localPoint.x()), true);
        dragging_ = false;
        hovered_ = inside;
        markPaintDirty();
        return true;
    }
    case core::EventType::KeyPress:
    case core::EventType::KeyRepeat: {
        if (!focused()) {
            return false;
        }

        const auto& keyEvent = static_cast<const core::KeyEvent&>(event);
        const float delta = effectiveStep();
        switch (keyEvent.key) {
        case core::keys::kLeft:
        case core::keys::kDown:
            setValueInternal(value_ - delta, true);
            return true;
        case core::keys::kRight:
        case core::keys::kUp:
            setValueInternal(value_ + delta, true);
            return true;
        case core::keys::kHome:
            setValueInternal(minimum_, true);
            return true;
        case core::keys::kEnd:
            setValueInternal(maximum_, true);
            return true;
        case core::keys::kPageDown:
            setValueInternal(value_ - delta * 4.0f, true);
            return true;
        case core::keys::kPageUp:
            setValueInternal(value_ + delta * 4.0f, true);
            return true;
        default:
            return false;
        }
    }
    default:
        return false;
    }
}

float Slider::effectiveStep() const
{
    if (step_ > 0.0f) {
        return step_;
    }

    const float range = maximum_ - minimum_;
    return range > 0.0f ? std::max(range / 20.0f, 0.01f) : 0.0f;
}

float Slider::valueFromLocalX(float localX) const
{
    const float trackWidth = std::max(1.0f, bounds_.width() - kHorizontalInset * 2.0f);
    const float normalized = std::clamp((localX - kHorizontalInset) / trackWidth, 0.0f, 1.0f);
    return minimum_ + (maximum_ - minimum_) * normalized;
}

void Slider::setValueInternal(float value, bool emitCallback)
{
    float clamped = std::clamp(value, minimum_, maximum_);
    const float quantizeStep = effectiveStep();
    if (quantizeStep > 0.0f) {
        const float steps = std::round((clamped - minimum_) / quantizeStep);
        clamped = minimum_ + steps * quantizeStep;
        clamped = std::clamp(clamped, minimum_, maximum_);
    }

    if (sameValue(value_, clamped)) {
        return;
    }

    value_ = clamped;
    markPaintDirty();
    if (emitCallback && onValueChanged_) {
        onValueChanged_(value_);
    }
}

}  // namespace tinalux::ui
