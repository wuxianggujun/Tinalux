#include "tinalux/ui/Toggle.h"

#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Theme.h"

#include "HoverAnimationUtils.h"
#include "../TextPrimitives.h"

namespace tinalux::ui {

Toggle::Toggle(std::string label, bool on)
    : label_(std::move(label))
    , on_(on)
{
}

Toggle::~Toggle()
{
    if (hoverAnimationAlive_ != nullptr) {
        *hoverAnimationAlive_ = false;
    }
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

void Toggle::setStyle(const ToggleStyle& style)
{
    customStyle_ = style;
    markLayoutDirty();
}

void Toggle::clearStyle()
{
    if (!customStyle_.has_value()) {
        return;
    }

    customStyle_.reset();
    markLayoutDirty();
}

const ToggleStyle* Toggle::style() const
{
    return customStyle_ ? &*customStyle_ : nullptr;
}

bool Toggle::focusable() const
{
    return true;
}

const ToggleStyle& Toggle::resolvedStyle() const
{
    return customStyle_ ? *customStyle_ : resolvedTheme().toggleStyle;
}

WidgetState Toggle::currentState() const
{
    if (pressed_) {
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

void Toggle::animateHoverProgress(float targetProgress)
{
    AnimationSink* currentAnimationSink = &animationSink();
    if (hoverAnimation_ != 0 && hoverAnimationSink_ == currentAnimationSink) {
        currentAnimationSink->cancelAnimation(hoverAnimation_);
        hoverAnimation_ = 0;
    }
    hoverAnimationSink_ = currentAnimationSink;

    const float currentProgress = animatedHoverProgress_ != nullptr ? *animatedHoverProgress_ : 0.0f;
    const float clampedTarget = detail::clampUnit(targetProgress);
    if (std::abs(currentProgress - clampedTarget) <= 0.001f) {
        if (animatedHoverProgress_ != nullptr) {
            *animatedHoverProgress_ = clampedTarget;
        }
        return;
    }

    const std::shared_ptr<float> progress = animatedHoverProgress_;
    const std::shared_ptr<bool> alive = hoverAnimationAlive_;
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

void Toggle::updateHovered(bool hovered)
{
    if (hovered_ == hovered) {
        return;
    }

    hovered_ = hovered;
    animateHoverProgress(hovered ? 1.0f : 0.0f);
    markPaintDirty();
}

core::Size Toggle::measure(const Constraints& constraints)
{
    const ToggleStyle& style = resolvedStyle();
    const TextMetrics metrics = measureTextMetrics(label_, style.textStyle.fontSize);
    return constraints.constrain(core::Size::Make(
        metrics.width + style.labelGap + style.trackWidth,
        std::max(style.minHeight, std::max(metrics.height, style.trackHeight))));
}

void Toggle::onDraw(rendering::Canvas& canvas)
{
    if (!canvas) {
        return;
    }

    const ToggleStyle& style = resolvedStyle();
    const WidgetState state = currentState();
    const WidgetState baseState = focused() ? WidgetState::Focused : WidgetState::Normal;
    const float hoverProgress = animatedHoverProgress_ != nullptr ? *animatedHoverProgress_ : 0.0f;
    const TextMetrics metrics = measureTextMetrics(label_, style.textStyle.fontSize);
    const float trackY = (bounds_.height() - style.trackHeight) * 0.5f;
    const float thumbCenterY = bounds_.height() * 0.5f;
    const float thumbCenterX = on_
        ? (style.trackWidth - style.thumbInset - style.thumbRadius)
        : (style.thumbInset + style.thumbRadius);
    const float trackBorderWidth = style.trackBorderWidth.resolve(state);
    const auto& trackColorStyle = on_ ? style.onTrackColor : style.offTrackColor;
    const auto& thumbColorStyle = on_ ? style.onThumbColor : style.offThumbColor;
    const core::Color trackColor = pressed_
        ? trackColorStyle.resolve(state)
        : detail::lerpColor(
            trackColorStyle.resolve(baseState),
            trackColorStyle.resolve(WidgetState::Hovered),
            hoverProgress);
    const core::Color thumbColor = pressed_
        ? thumbColorStyle.resolve(state)
        : detail::lerpColor(
            thumbColorStyle.resolve(baseState),
            thumbColorStyle.resolve(WidgetState::Hovered),
            hoverProgress);

    canvas.drawRoundRect(
        core::Rect::MakeXYWH(0.0f, trackY, style.trackWidth, style.trackHeight),
        style.trackHeight * 0.5f,
        style.trackHeight * 0.5f,
        trackColor);

    if (trackBorderWidth > 0.0f) {
        canvas.drawRoundRect(
            core::Rect::MakeXYWH(
                trackBorderWidth * 0.5f,
                trackY + trackBorderWidth * 0.5f,
                std::max(0.0f, style.trackWidth - trackBorderWidth),
                std::max(0.0f, style.trackHeight - trackBorderWidth)),
            style.trackHeight * 0.5f,
            style.trackHeight * 0.5f,
            style.trackBorderColor.resolve(state),
            rendering::PaintStyle::Stroke,
            trackBorderWidth);
    }
    canvas.drawCircle(
        thumbCenterX,
        thumbCenterY,
        style.thumbRadius,
        thumbColor);
    canvas.drawText(
        label_.c_str(),
        style.trackWidth + style.labelGap + metrics.drawX,
        (bounds_.height() - metrics.height) * 0.5f + metrics.baseline,
        style.textStyle.fontSize,
        style.labelColor.resolve(state));
}

bool Toggle::onEvent(core::Event& event)
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
        const core::Point localPoint = globalToLocal(core::Point::Make(
            static_cast<float>(moveEvent.x),
            static_cast<float>(moveEvent.y)));
        const bool inside = containsLocalPoint(localPoint.x(), localPoint.y());
        updateHovered(inside);
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
            updateHovered(true);
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
            updateHovered(inside);
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
