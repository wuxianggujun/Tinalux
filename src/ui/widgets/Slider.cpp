#include "tinalux/ui/Slider.h"

#include <algorithm>
#include <cmath>

#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Theme.h"

#include "HoverAnimationUtils.h"

namespace tinalux::ui {

namespace {

constexpr float kValueTolerance = 0.0001f;

bool sameValue(float lhs, float rhs)
{
    return std::abs(lhs - rhs) <= kValueTolerance;
}

}  // namespace

Slider::~Slider()
{
    if (animationAlive_ != nullptr) {
        *animationAlive_ = false;
    }
}

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

void Slider::setStyle(const SliderStyle& style)
{
    customStyle_ = style;
    markLayoutDirty();
}

void Slider::clearStyle()
{
    if (!customStyle_.has_value()) {
        return;
    }

    customStyle_.reset();
    markLayoutDirty();
}

const SliderStyle* Slider::style() const
{
    return customStyle_ ? &*customStyle_ : nullptr;
}

bool Slider::focusable() const
{
    return true;
}

void Slider::setFocused(bool focused)
{
    if (Widget::focused() == focused) {
        return;
    }

    Widget::setFocused(focused);
    animateFocusProgress(focused ? 1.0f : 0.0f);
}

const SliderStyle& Slider::resolvedStyle() const
{
    return customStyle_ ? *customStyle_ : resolvedTheme().sliderStyle;
}

WidgetState Slider::currentState() const
{
    if (dragging_) {
        return WidgetState::Pressed;
    }
    if (hovered_) {
        return WidgetState::Hovered;
    }
    if (focused()) {
        return WidgetState::Focused;
    }
    return WidgetState::Normal;
}

void Slider::animateHoverProgress(float targetProgress)
{
    AnimationSink* currentAnimationSink = &animationSink();
    if (hoverAnimation_ != 0 && hoverAnimationSink_ == currentAnimationSink) {
        currentAnimationSink->cancelAnimation(hoverAnimation_);
        hoverAnimation_ = 0;
    }
    hoverAnimationSink_ = currentAnimationSink;

    const float currentProgress = animatedHoverProgress_ != nullptr ? *animatedHoverProgress_ : 0.0f;
    const float clampedTarget = detail::clampUnit(targetProgress);
    if (std::abs(currentProgress - clampedTarget) <= kValueTolerance) {
        if (animatedHoverProgress_ != nullptr) {
            *animatedHoverProgress_ = clampedTarget;
        }
        return;
    }

    const std::shared_ptr<float> progress = animatedHoverProgress_;
    const std::shared_ptr<bool> alive = animationAlive_;
    hoverAnimation_ = currentAnimationSink->animate(
        {
            .from = currentProgress,
            .to = clampedTarget,
            .durationSeconds = detail::kHoverTransitionDurationSeconds,
            .loop = false,
            .alternate = false,
            .easing = Easing::EaseInOut,
        },
        [progress, alive, this](float value) {
            if (progress != nullptr) {
                *progress = value;
            }
            if (alive != nullptr && *alive) {
                markPaintDirty();
            }
        });
}

void Slider::animateFocusProgress(float targetProgress)
{
    AnimationSink* currentAnimationSink = &animationSink();
    if (focusAnimation_ != 0 && focusAnimationSink_ == currentAnimationSink) {
        currentAnimationSink->cancelAnimation(focusAnimation_);
        focusAnimation_ = 0;
    }
    focusAnimationSink_ = currentAnimationSink;

    const float currentProgress = animatedFocusProgress_ != nullptr ? *animatedFocusProgress_ : 0.0f;
    const float clampedTarget = detail::clampUnit(targetProgress);
    if (std::abs(currentProgress - clampedTarget) <= kValueTolerance) {
        if (animatedFocusProgress_ != nullptr) {
            *animatedFocusProgress_ = clampedTarget;
        }
        return;
    }

    const std::shared_ptr<float> progress = animatedFocusProgress_;
    const std::shared_ptr<bool> alive = animationAlive_;
    focusAnimation_ = currentAnimationSink->animate(
        {
            .from = currentProgress,
            .to = clampedTarget,
            .durationSeconds = detail::kHoverTransitionDurationSeconds,
            .loop = false,
            .alternate = false,
            .easing = Easing::EaseInOut,
        },
        [progress, alive, this](float value) {
            if (progress != nullptr) {
                *progress = value;
            }
            if (alive != nullptr && *alive) {
                markPaintDirty();
            }
        });
}

void Slider::updateHovered(bool hovered)
{
    if (hovered_ == hovered) {
        return;
    }

    hovered_ = hovered;
    animateHoverProgress(hovered ? 1.0f : 0.0f);
    markPaintDirty();
}

core::Size Slider::measure(const Constraints& constraints)
{
    const SliderStyle& style = resolvedStyle();
    const float preferredWidth = style.preferredWidth < 0.0f && std::isfinite(constraints.maxWidth)
        ? constraints.maxWidth
        : style.preferredWidth;
    return constraints.constrain(core::Size::Make(preferredWidth, style.preferredHeight));
}

void Slider::onDraw(rendering::Canvas& canvas)
{
    if (!canvas) {
        return;
    }

    const SliderStyle& style = resolvedStyle();
    const WidgetState state = currentState();
    const float hoverProgress = animatedHoverProgress_ != nullptr ? *animatedHoverProgress_ : 0.0f;
    const float focusProgress = animatedFocusProgress_ != nullptr ? *animatedFocusProgress_ : 0.0f;
    const float trackLeft = style.horizontalInset;
    const float trackWidth = std::max(1.0f, bounds_.width() - style.horizontalInset * 2.0f);
    const float trackTop = (bounds_.height() - style.trackHeight) * 0.5f;
    const float fraction = maximum_ > minimum_
        ? (value_ - minimum_) / (maximum_ - minimum_)
        : 0.0f;
    const float clampedFraction = std::clamp(fraction, 0.0f, 1.0f);
    const float thumbX = trackLeft + trackWidth * clampedFraction;
    const auto blendStateColor = [&](const StateStyle<core::Color>& stateStyle) {
        if (dragging_) {
            return stateStyle.resolve(WidgetState::Pressed);
        }

        const core::Color hoverColor = detail::lerpColor(
            stateStyle.resolve(WidgetState::Normal),
            stateStyle.resolve(WidgetState::Hovered),
            hoverProgress);
        return detail::lerpColor(
            hoverColor,
            stateStyle.resolve(WidgetState::Focused),
            focusProgress);
    };
    const float thumbRadius = dragging_
        ? style.activeThumbRadius
        : detail::lerpFloat(style.thumbRadius, style.activeThumbRadius, focusProgress);
    const core::Color focusHaloColor = detail::lerpColor(
        core::colorARGB(
            0,
            style.focusHaloColor.red(),
            style.focusHaloColor.green(),
            style.focusHaloColor.blue()),
        style.focusHaloColor,
        focusProgress);

    canvas.drawRoundRect(
        core::Rect::MakeXYWH(trackLeft, trackTop, trackWidth, style.trackHeight),
        style.trackHeight * 0.5f,
        style.trackHeight * 0.5f,
        dragging_
            ? style.trackColor.resolve(state)
            : detail::lerpColor(
                style.trackColor.resolve(WidgetState::Normal),
                style.trackColor.resolve(WidgetState::Focused),
                focusProgress));

    canvas.drawRoundRect(
        core::Rect::MakeXYWH(
            trackLeft,
            trackTop,
            std::max(0.0f, thumbX - trackLeft),
            style.trackHeight),
        style.trackHeight * 0.5f,
        style.trackHeight * 0.5f,
        blendStateColor(style.fillColor));

    if (focusProgress > 0.001f) {
        canvas.drawCircle(
            thumbX,
            bounds_.height() * 0.5f,
            thumbRadius + detail::lerpFloat(0.0f, style.focusHaloPadding, focusProgress),
            focusHaloColor);
    }

    canvas.drawCircle(
        thumbX,
        bounds_.height() * 0.5f,
        thumbRadius,
        blendStateColor(style.thumbColor));
    canvas.drawCircle(
        thumbX,
        bounds_.height() * 0.5f,
        std::max(4.0f, thumbRadius - style.innerThumbInset),
        blendStateColor(style.thumbInnerColor));
}

bool Slider::onEvent(core::Event& event)
{
    switch (event.type()) {
    case core::EventType::MouseEnter:
        updateHovered(true);
        return false;
    case core::EventType::MouseLeave:
        updateHovered(false);
        return false;
    case core::EventType::MouseMove: {
        const auto& moveEvent = static_cast<const core::MouseMoveEvent&>(event);
        const float globalX = static_cast<float>(moveEvent.x);
        const float globalY = static_cast<float>(moveEvent.y);
        const core::Point localPoint = globalToLocal(core::Point::Make(globalX, globalY));
        const bool inside = containsLocalPoint(localPoint.x(), localPoint.y());
        updateHovered(inside);

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
        updateHovered(true);
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
        updateHovered(inside);
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
    const SliderStyle& style = resolvedStyle();
    const float trackWidth = std::max(1.0f, bounds_.width() - style.horizontalInset * 2.0f);
    const float normalized =
        std::clamp((localX - style.horizontalInset) / trackWidth, 0.0f, 1.0f);
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
