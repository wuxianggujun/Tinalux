#include "tinalux/ui/Radio.h"

#include <algorithm>
#include <cstdint>

#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Container.h"
#include "tinalux/ui/Theme.h"

#include "HoverAnimationUtils.h"
#include "../TextPrimitives.h"

namespace tinalux::ui {

namespace {

core::Rect resolveRadioSelectionIconBounds(float indicatorY, const RadioStyle& style)
{
    const float side = std::max(0.0f, style.innerDotSize);
    const float offset = (style.indicatorSize - side) * 0.5f;
    return core::Rect::MakeXYWH(offset, indicatorY + offset, side, side);
}

}  // namespace

Radio::Radio(std::string label, std::string group, bool selected)
    : label_(std::move(label))
    , group_(std::move(group))
    , selected_(selected)
{
}

Radio::~Radio()
{
    if (hoverAnimationAlive_ != nullptr) {
        *hoverAnimationAlive_ = false;
    }
    if (selectionIconLoadAlive_ != nullptr) {
        *selectionIconLoadAlive_ = false;
    }
}

const std::string& Radio::label() const
{
    return label_;
}

void Radio::setLabel(const std::string& label)
{
    if (label_ == label) {
        return;
    }

    label_ = label;
    markLayoutDirty();
}

const std::string& Radio::group() const
{
    return group_;
}

void Radio::setGroup(const std::string& group)
{
    if (group_ == group) {
        return;
    }

    group_ = group;
    if (selected_) {
        deselectGroupSiblings();
    }
    markPaintDirty();
}

bool Radio::selected() const
{
    return selected_;
}

void Radio::setSelected(bool selected)
{
    updateSelected(selected, false, true);
}

void Radio::setSelectionIcon(rendering::Image icon)
{
    ++(*selectionIconLoadGeneration_);
    pendingSelectionIcon_ = {};
    selectionIconPath_.clear();
    selectionIconLoading_ = false;
    selectionIcon_ = std::move(icon);
    selectionIconLoadState_ = selectionIcon_ ? RadioIndicatorLoadState::Ready : RadioIndicatorLoadState::Idle;
    markPaintDirty();
}

void Radio::setSelectionIcon(IconType type, float sizeHint)
{
    setSelectionIcon(IconRegistry::instance().getIcon(type, sizeHint));
}

const rendering::Image& Radio::selectionIcon() const
{
    return selectionIcon_;
}

void Radio::loadSelectionIconAsync(const std::string& path)
{
    ++(*selectionIconLoadGeneration_);
    selectionIconPath_ = path;
    selectionIcon_ = {};
    selectionIconLoading_ = !path.empty();
    selectionIconLoadState_ = selectionIconLoading_ ? RadioIndicatorLoadState::Loading : RadioIndicatorLoadState::Idle;
    pendingSelectionIcon_ = {};
    markPaintDirty();
    if (path.empty()) {
        return;
    }

    const std::uint64_t generation = *selectionIconLoadGeneration_;
    pendingSelectionIcon_ = ResourceLoader::instance().loadImageAsync(path);
    const std::shared_ptr<bool> alive = selectionIconLoadAlive_;
    const std::shared_ptr<std::uint64_t> loadGeneration = selectionIconLoadGeneration_;
    pendingSelectionIcon_.onReady([this, alive, loadGeneration, generation](const rendering::Image& image) {
        if (alive == nullptr || !*alive || loadGeneration == nullptr || *loadGeneration != generation) {
            return;
        }

        selectionIconLoading_ = false;
        selectionIcon_ = image;
        selectionIconLoadState_ = selectionIcon_ ? RadioIndicatorLoadState::Ready : RadioIndicatorLoadState::Failed;
        markPaintDirty();
    });
}

const std::string& Radio::selectionIconPath() const
{
    return selectionIconPath_;
}

bool Radio::selectionIconLoading() const
{
    return selectionIconLoading_;
}

RadioIndicatorLoadState Radio::selectionIconLoadState() const
{
    return selectionIconLoadState_;
}

void Radio::onChanged(std::function<void(bool)> handler)
{
    onChanged_ = std::move(handler);
}

void Radio::setStyle(const RadioStyle& style)
{
    customStyle_ = style;
    markLayoutDirty();
}

void Radio::clearStyle()
{
    if (!customStyle_.has_value()) {
        return;
    }

    customStyle_.reset();
    markLayoutDirty();
}

const RadioStyle* Radio::style() const
{
    return customStyle_ ? &*customStyle_ : nullptr;
}

bool Radio::focusable() const
{
    return true;
}

const RadioStyle& Radio::resolvedStyle() const
{
    return customStyle_ ? *customStyle_ : resolvedTheme().radioStyle;
}

WidgetState Radio::currentState() const
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

void Radio::animateHoverProgress(float targetProgress)
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

void Radio::updateHovered(bool hovered)
{
    if (hovered_ == hovered) {
        return;
    }

    hovered_ = hovered;
    animateHoverProgress(hovered ? 1.0f : 0.0f);
    markPaintDirty();
}

core::Size Radio::measure(const Constraints& constraints)
{
    const RadioStyle& style = resolvedStyle();
    const TextMetrics metrics = measureTextMetrics(label_, style.textStyle.fontSize);
    return constraints.constrain(core::Size::Make(
        style.indicatorSize + style.labelGap + metrics.width,
        std::max(style.minHeight, std::max(style.indicatorSize, metrics.height))));
}

void Radio::onDraw(rendering::Canvas& canvas)
{
    if (!canvas) {
        return;
    }

    const RadioStyle& style = resolvedStyle();
    const WidgetState state = currentState();
    const WidgetState baseState = focused() ? WidgetState::Focused : WidgetState::Normal;
    const float hoverProgress = animatedHoverProgress_ != nullptr ? *animatedHoverProgress_ : 0.0f;
    const TextMetrics metrics = measureTextMetrics(label_, style.textStyle.fontSize);
    const float indicatorY = (bounds_.height() - style.indicatorSize) * 0.5f;
    const float center = indicatorY + style.indicatorSize * 0.5f;
    const float borderWidth = style.borderWidth.resolve(state);
    const auto& borderColorStyle = selected_ ? style.selectedBorderColor : style.unselectedBorderColor;
    const core::Color borderColor = pressed_
        ? borderColorStyle.resolve(state)
        : detail::lerpColor(
            borderColorStyle.resolve(baseState),
            borderColorStyle.resolve(WidgetState::Hovered),
            hoverProgress);

    canvas.drawCircle(
        style.indicatorSize * 0.5f,
        center,
        style.indicatorSize * 0.5f - borderWidth * 0.5f,
        borderColor,
        rendering::PaintStyle::Stroke,
        borderWidth);

    if (selected_) {
        if (selectionIcon_) {
            canvas.drawImage(
                selectionIcon_,
                resolveRadioSelectionIconBounds(indicatorY, style));
        } else {
            canvas.drawCircle(
                style.indicatorSize * 0.5f,
                center,
                style.innerDotSize * 0.5f,
                style.dotColor);
        }
    }
    canvas.drawText(
        label_.c_str(),
        style.indicatorSize + style.labelGap + metrics.drawX,
        (bounds_.height() - metrics.height) * 0.5f + metrics.baseline,
        style.textStyle.fontSize,
        style.labelColor.resolve(state));
}

bool Radio::onEvent(core::Event& event)
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
            updateSelected(true, true, true);
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
            updateSelected(true, true, true);
            return true;
        }
        return false;
    }
    default:
        return false;
    }
}

void Radio::updateSelected(bool selected, bool emitCallback, bool syncGroup)
{
    if (selected_ == selected) {
        return;
    }

    selected_ = selected;
    if (selected_ && syncGroup) {
        deselectGroupSiblings();
    }
    markPaintDirty();
    if (emitCallback && onChanged_) {
        onChanged_(selected_);
    }
}

void Radio::deselectGroupSiblings()
{
    if (group_.empty()) {
        return;
    }

    auto* container = dynamic_cast<Container*>(parent());
    if (container == nullptr) {
        return;
    }

    for (const auto& child : container->children()) {
        auto* sibling = dynamic_cast<Radio*>(child.get());
        if (sibling == nullptr || sibling == this) {
            continue;
        }
        if (sibling->group_ == group_ && sibling->selected_) {
            sibling->updateSelected(false, true, false);
        }
    }
}

}  // namespace tinalux::ui
