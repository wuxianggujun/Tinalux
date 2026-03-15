#include "tinalux/ui/Checkbox.h"

#include <algorithm>
#include <cstdint>

#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Theme.h"

#include "HoverAnimationUtils.h"
#include "../TextPrimitives.h"

namespace tinalux::ui {

namespace {

core::Rect resolveCheckmarkIconBounds(float indicatorY, float indicatorSize)
{
    const float inset = std::max(2.0f, indicatorSize * 0.14f);
    return core::Rect::MakeXYWH(
        inset,
        indicatorY + inset,
        std::max(0.0f, indicatorSize - inset * 2.0f),
        std::max(0.0f, indicatorSize - inset * 2.0f));
}

void drawFallbackCheckmark(
    rendering::Canvas& canvas,
    float indicatorY,
    const CheckboxStyle& style)
{
    const float scale = style.indicatorSize / 22.0f;
    const float left = 5.0f * scale;
    const float midX = 9.0f * scale;
    const float midY = indicatorY + 15.0f * scale;
    const float right = 17.0f * scale;
    const float top = indicatorY + 7.0f * scale;
    canvas.drawLine(
        left,
        indicatorY + 11.0f * scale,
        midX,
        midY,
        style.checkmarkColor,
        style.checkmarkStrokeWidth,
        true);
    canvas.drawLine(
        midX,
        midY,
        right,
        top,
        style.checkmarkColor,
        style.checkmarkStrokeWidth,
        true);
}

}  // namespace

Checkbox::Checkbox(std::string label, bool checked)
    : label_(std::move(label))
    , checked_(checked)
{
}

Checkbox::~Checkbox()
{
    if (hoverAnimationAlive_ != nullptr) {
        *hoverAnimationAlive_ = false;
    }
    if (checkmarkIconLoadAlive_ != nullptr) {
        *checkmarkIconLoadAlive_ = false;
    }
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

void Checkbox::setCheckmarkIcon(rendering::Image icon)
{
    ++(*checkmarkIconLoadGeneration_);
    pendingCheckmarkIcon_ = {};
    checkmarkIconPath_.clear();
    checkmarkIconLoading_ = false;
    checkmarkIcon_ = std::move(icon);
    checkmarkIconLoadState_ = checkmarkIcon_ ? CheckboxIconLoadState::Ready : CheckboxIconLoadState::Idle;
    markPaintDirty();
}

void Checkbox::setCheckmarkIcon(IconType type, float sizeHint)
{
    setCheckmarkIcon(IconRegistry::instance().getIcon(type, sizeHint));
}

const rendering::Image& Checkbox::checkmarkIcon() const
{
    return checkmarkIcon_;
}

void Checkbox::loadCheckmarkIconAsync(const std::string& path)
{
    ++(*checkmarkIconLoadGeneration_);
    checkmarkIconPath_ = path;
    checkmarkIcon_ = {};
    checkmarkIconLoading_ = !path.empty();
    checkmarkIconLoadState_ = checkmarkIconLoading_ ? CheckboxIconLoadState::Loading : CheckboxIconLoadState::Idle;
    pendingCheckmarkIcon_ = {};
    markPaintDirty();
    if (path.empty()) {
        return;
    }

    const std::uint64_t generation = *checkmarkIconLoadGeneration_;
    pendingCheckmarkIcon_ = ResourceLoader::instance().loadImageAsync(path);
    const std::shared_ptr<bool> alive = checkmarkIconLoadAlive_;
    const std::shared_ptr<std::uint64_t> loadGeneration = checkmarkIconLoadGeneration_;
    pendingCheckmarkIcon_.onReady([this, alive, loadGeneration, generation](const rendering::Image& image) {
        if (alive == nullptr || !*alive || loadGeneration == nullptr || *loadGeneration != generation) {
            return;
        }

        checkmarkIconLoading_ = false;
        checkmarkIcon_ = image;
        checkmarkIconLoadState_ = checkmarkIcon_ ? CheckboxIconLoadState::Ready : CheckboxIconLoadState::Failed;
        markPaintDirty();
    });
}

const std::string& Checkbox::checkmarkIconPath() const
{
    return checkmarkIconPath_;
}

bool Checkbox::checkmarkIconLoading() const
{
    return checkmarkIconLoading_;
}

CheckboxIconLoadState Checkbox::checkmarkIconLoadState() const
{
    return checkmarkIconLoadState_;
}

void Checkbox::onToggle(std::function<void(bool)> handler)
{
    onToggle_ = std::move(handler);
}

void Checkbox::setStyle(const CheckboxStyle& style)
{
    customStyle_ = style;
    markLayoutDirty();
}

void Checkbox::clearStyle()
{
    if (!customStyle_.has_value()) {
        return;
    }

    customStyle_.reset();
    markLayoutDirty();
}

const CheckboxStyle* Checkbox::style() const
{
    return customStyle_ ? &*customStyle_ : nullptr;
}

bool Checkbox::focusable() const
{
    return true;
}

const CheckboxStyle& Checkbox::resolvedStyle() const
{
    return customStyle_ ? *customStyle_ : resolvedTheme().checkboxStyle;
}

WidgetState Checkbox::currentState() const
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

void Checkbox::animateHoverProgress(float targetProgress)
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

void Checkbox::updateHovered(bool hovered)
{
    if (hovered_ == hovered) {
        return;
    }

    hovered_ = hovered;
    animateHoverProgress(hovered ? 1.0f : 0.0f);
    markPaintDirty();
}

core::Size Checkbox::measure(const Constraints& constraints)
{
    const CheckboxStyle& style = resolvedStyle();
    const TextMetrics metrics = measureTextMetrics(label_, style.textStyle.fontSize);
    return constraints.constrain(core::Size::Make(
        style.indicatorSize + style.labelGap + metrics.width,
        std::max(style.minHeight, std::max(style.indicatorSize, metrics.height))));
}

void Checkbox::onDraw(rendering::Canvas& canvas)
{
    if (!canvas) {
        return;
    }

    const CheckboxStyle& style = resolvedStyle();
    const WidgetState state = currentState();
    const WidgetState baseState = focused() ? WidgetState::Focused : WidgetState::Normal;
    const float hoverProgress = animatedHoverProgress_ != nullptr ? *animatedHoverProgress_ : 0.0f;
    const TextMetrics metrics = measureTextMetrics(label_, style.textStyle.fontSize);
    const float indicatorY = (bounds_.height() - style.indicatorSize) * 0.5f;
    const float labelY = (bounds_.height() - metrics.height) * 0.5f + metrics.baseline;
    const float borderWidth = style.borderWidth.resolve(state);
    const auto& backgroundStyle = checked_ ? style.checkedBackgroundColor : style.uncheckedBackgroundColor;
    const core::Color backgroundColor = pressed_
        ? backgroundStyle.resolve(state)
        : detail::lerpColor(
            backgroundStyle.resolve(baseState),
            backgroundStyle.resolve(WidgetState::Hovered),
            hoverProgress);
    const core::Color borderColor = pressed_
        ? style.borderColor.resolve(state)
        : detail::lerpColor(
            style.borderColor.resolve(baseState),
            style.borderColor.resolve(WidgetState::Hovered),
            hoverProgress);

    canvas.drawRoundRect(
        core::Rect::MakeXYWH(0.0f, indicatorY, style.indicatorSize, style.indicatorSize),
        style.indicatorRadius,
        style.indicatorRadius,
        backgroundColor);
    canvas.drawRoundRect(
        core::Rect::MakeXYWH(
            borderWidth * 0.5f,
            indicatorY + borderWidth * 0.5f,
        std::max(0.0f, style.indicatorSize - borderWidth),
        std::max(0.0f, style.indicatorSize - borderWidth)),
        style.indicatorRadius,
        style.indicatorRadius,
        borderColor,
        rendering::PaintStyle::Stroke,
        borderWidth);

    if (checked_) {
        if (checkmarkIcon_) {
            canvas.drawImage(
                checkmarkIcon_,
                resolveCheckmarkIconBounds(indicatorY, style.indicatorSize));
        } else {
            drawFallbackCheckmark(canvas, indicatorY, style);
        }
    }

    canvas.drawText(
        label_.c_str(),
        style.indicatorSize + style.labelGap + metrics.drawX,
        labelY,
        style.textStyle.fontSize,
        style.labelColor.resolve(state));
}

bool Checkbox::onEvent(core::Event& event)
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
