#include "tinalux/ui/Button.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Theme.h"

#include "HoverAnimationUtils.h"
#include "../TextPrimitives.h"

namespace tinalux::ui {

namespace {

core::Size resolveButtonIconSize(const rendering::Image& icon, core::Size requestedSize)
{
    if (!icon) {
        return core::Size::Make(0.0f, 0.0f);
    }

    const float naturalWidth = static_cast<float>(icon.width());
    const float naturalHeight = static_cast<float>(icon.height());
    if (naturalWidth <= 0.0f || naturalHeight <= 0.0f) {
        return core::Size::Make(0.0f, 0.0f);
    }

    const float requestedWidth = std::max(0.0f, requestedSize.width());
    const float requestedHeight = std::max(0.0f, requestedSize.height());
    if (requestedWidth > 0.0f && requestedHeight > 0.0f) {
        return core::Size::Make(requestedWidth, requestedHeight);
    }

    if (requestedWidth > 0.0f) {
        return core::Size::Make(requestedWidth, requestedWidth * (naturalHeight / naturalWidth));
    }

    if (requestedHeight > 0.0f) {
        return core::Size::Make(requestedHeight * (naturalWidth / naturalHeight), requestedHeight);
    }

    return icon.size();
}

core::Size resolveButtonIconSlotSize(
    const rendering::Image& icon,
    core::Size requestedSize,
    float fallbackSide)
{
    const core::Size resolvedImageSize = resolveButtonIconSize(icon, requestedSize);
    if (resolvedImageSize.width() > 0.0f && resolvedImageSize.height() > 0.0f) {
        return resolvedImageSize;
    }

    const float requestedWidth = std::max(0.0f, requestedSize.width());
    const float requestedHeight = std::max(0.0f, requestedSize.height());
    if (requestedWidth > 0.0f && requestedHeight > 0.0f) {
        return core::Size::Make(requestedWidth, requestedHeight);
    }

    if (requestedWidth > 0.0f) {
        return core::Size::Make(requestedWidth, requestedWidth);
    }

    if (requestedHeight > 0.0f) {
        return core::Size::Make(requestedHeight, requestedHeight);
    }

    const float fallback = std::max(0.0f, fallbackSide);
    return fallback > 0.0f ? core::Size::Make(fallback, fallback) : core::Size::Make(0.0f, 0.0f);
}

core::Color placeholderColor(core::Color textColor, bool failed)
{
    if (failed) {
        return core::colorARGB(255, 196, 88, 88);
    }

    return core::colorARGB(196, textColor.red(), textColor.green(), textColor.blue());
}

void drawButtonIconPlaceholder(
    rendering::Canvas& canvas,
    core::Rect bounds,
    core::Color color,
    bool failed)
{
    const float radius = std::max(2.0f, std::min(bounds.width(), bounds.height()) * 0.25f);
    canvas.drawRoundRect(bounds, radius, radius, color);
    if (!failed) {
        return;
    }

    const float inset = std::max(2.0f, std::min(bounds.width(), bounds.height()) * 0.22f);
    const core::Color strokeColor = core::colorARGB(255, 255, 244, 244);
    canvas.drawLine(
        bounds.x() + inset,
        bounds.y() + inset,
        bounds.right() - inset,
        bounds.bottom() - inset,
        strokeColor,
        1.5f);
    canvas.drawLine(
        bounds.right() - inset,
        bounds.y() + inset,
        bounds.x() + inset,
        bounds.bottom() - inset,
        strokeColor,
        1.5f);
}

}  // namespace

Button::Button(std::string label)
    : label_(std::move(label))
{
}

Button::~Button()
{
    if (hoverAnimationAlive_ != nullptr) {
        *hoverAnimationAlive_ = false;
    }
    if (iconLoadAlive_ != nullptr) {
        *iconLoadAlive_ = false;
    }
}

void Button::setLabel(const std::string& label)
{
    if (label_ == label) {
        return;
    }

    label_ = label;
    markLayoutDirty();
}

void Button::setIcon(rendering::Image icon)
{
    ++(*iconLoadGeneration_);
    pendingIcon_ = {};
    iconPath_.clear();
    iconLoading_ = false;
    icon_ = std::move(icon);
    iconLoadState_ = icon_ ? ButtonIconLoadState::Ready : ButtonIconLoadState::Idle;
    markLayoutDirty();
}

void Button::setIcon(IconType type, float sizeHint)
{
    setIcon(IconRegistry::instance().getIcon(type, sizeHint));
}

const rendering::Image& Button::icon() const
{
    return icon_;
}

void Button::loadIconAsync(const std::string& path)
{
    ++(*iconLoadGeneration_);
    iconPath_ = path;
    icon_ = {};
    iconLoading_ = !path.empty();
    iconLoadState_ = iconLoading_ ? ButtonIconLoadState::Loading : ButtonIconLoadState::Idle;
    pendingIcon_ = {};
    markLayoutDirty();
    if (path.empty()) {
        return;
    }

    const std::uint64_t generation = *iconLoadGeneration_;
    pendingIcon_ = ResourceLoader::instance().loadImageAsync(path);
    const std::shared_ptr<bool> alive = iconLoadAlive_;
    const std::shared_ptr<std::uint64_t> loadGeneration = iconLoadGeneration_;
    pendingIcon_.onReady([this, alive, loadGeneration, generation](const rendering::Image& image) {
        if (alive == nullptr || !*alive || loadGeneration == nullptr || *loadGeneration != generation) {
            return;
        }

        iconLoading_ = false;
        icon_ = image;
        iconLoadState_ = icon_ ? ButtonIconLoadState::Ready : ButtonIconLoadState::Failed;
        markLayoutDirty();
    });
}

const std::string& Button::iconPath() const
{
    return iconPath_;
}

bool Button::iconLoading() const
{
    return iconLoading_;
}

ButtonIconLoadState Button::iconLoadState() const
{
    return iconLoadState_;
}

void Button::setIconPlacement(ButtonIconPlacement placement)
{
    if (iconPlacement_ == placement) {
        return;
    }

    iconPlacement_ = placement;
    markPaintDirty();
}

ButtonIconPlacement Button::iconPlacement() const
{
    return iconPlacement_;
}

void Button::setIconSize(core::Size size)
{
    const core::Size clampedSize = core::Size::Make(
        std::max(0.0f, size.width()),
        std::max(0.0f, size.height()));
    if (iconSize_ == clampedSize) {
        return;
    }

    iconSize_ = clampedSize;
    markLayoutDirty();
}

core::Size Button::iconSize() const
{
    return iconSize_;
}

void Button::setStyle(const ButtonStyle& style)
{
    customStyle_ = style;
    markLayoutDirty();
}

void Button::clearStyle()
{
    if (!customStyle_.has_value()) {
        return;
    }

    customStyle_.reset();
    markLayoutDirty();
}

const ButtonStyle* Button::style() const
{
    return customStyle_ ? &*customStyle_ : nullptr;
}

void Button::onClick(std::function<void()> handler)
{
    onClick_ = std::move(handler);
}

bool Button::focusable() const
{
    return true;
}

const ButtonStyle& Button::resolvedStyle() const
{
    return customStyle_ ? *customStyle_ : resolvedTheme().buttonStyle;
}

WidgetState Button::currentState() const
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

void Button::animateHoverProgress(float targetProgress)
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

void Button::updateHovered(bool hovered)
{
    if (hovered_ == hovered) {
        return;
    }

    hovered_ = hovered;
    animateHoverProgress(hovered ? 1.0f : 0.0f);
    markPaintDirty();
}

core::Size Button::measure(const Constraints& constraints)
{
    const ButtonStyle& style = resolvedStyle();
    const TextMetrics metrics = measureTextMetrics(label_, style.textStyle.fontSize);
    const float minimumWidth = style.minWidth < 0.0f && std::isfinite(constraints.maxWidth)
        ? constraints.maxWidth
        : style.minWidth;
    const bool reserveIconSlot = static_cast<bool>(icon_)
        || iconLoading_
        || iconLoadState_ == ButtonIconLoadState::Failed;
    const core::Size resolvedIconSize = reserveIconSlot
        ? resolveButtonIconSlotSize(icon_, iconSize_, style.textStyle.fontSize)
        : core::Size::Make(0.0f, 0.0f);
    const bool hasIcon = resolvedIconSize.width() > 0.0f && resolvedIconSize.height() > 0.0f;
    const bool hasLabel = !label_.empty();
    const float contentWidth = metrics.width
        + (hasIcon ? resolvedIconSize.width() : 0.0f)
        + (hasIcon && hasLabel ? style.iconSpacing : 0.0f);
    const float contentHeight = std::max(metrics.height, resolvedIconSize.height());

    return constraints.constrain(core::Size::Make(
        std::max(minimumWidth, contentWidth + style.paddingHorizontal * 2.0f),
        std::max(style.minHeight, contentHeight + style.paddingVertical * 2.0f)));
}

void Button::onDraw(rendering::Canvas& canvas)
{
    if (!canvas) {
        return;
    }

    const ButtonStyle& style = resolvedStyle();
    const WidgetState state = currentState();
    const WidgetState baseState = focused() ? WidgetState::Focused : WidgetState::Normal;
    const float hoverProgress = animatedHoverProgress_ != nullptr ? *animatedHoverProgress_ : 0.0f;
    const core::Color backgroundColor = pressed_
        ? style.backgroundColor.resolve(state)
        : detail::lerpColor(
            style.backgroundColor.resolve(baseState),
            style.backgroundColor.resolve(WidgetState::Hovered),
            hoverProgress);
    const core::Color borderColor = style.borderColor.resolve(state);
    const float borderWidth = style.borderWidth.resolve(state);
    const core::Color textColor = style.textColor.resolve(state);

    canvas.drawRoundRect(
        core::Rect::MakeWH(bounds_.width(), bounds_.height()),
        style.borderRadius,
        style.borderRadius,
        backgroundColor);

    if (borderWidth > 0.0f) {
        const float inset = borderWidth * 0.5f;
        canvas.drawRoundRect(
            core::Rect::MakeXYWH(
                inset,
                inset,
                std::max(0.0f, bounds_.width() - borderWidth),
                std::max(0.0f, bounds_.height() - borderWidth)),
            std::max(0.0f, style.borderRadius - inset),
            std::max(0.0f, style.borderRadius - inset),
            borderColor,
            rendering::PaintStyle::Stroke,
            borderWidth);
    }

    if (focused()) {
        const float inset = std::max(1.0f, style.focusRingWidth * 0.5f);
        canvas.drawRoundRect(
            core::Rect::MakeXYWH(
                inset,
                inset,
                std::max(0.0f, bounds_.width() - inset * 2.0f),
                std::max(0.0f, bounds_.height() - inset * 2.0f)),
            std::max(0.0f, style.borderRadius - inset),
            std::max(0.0f, style.borderRadius - inset),
            style.focusRingColor,
            rendering::PaintStyle::Stroke,
            style.focusRingWidth);
    }

    const TextMetrics metrics = measureTextMetrics(label_, style.textStyle.fontSize);
    const bool reserveIconSlot = static_cast<bool>(icon_)
        || iconLoading_
        || iconLoadState_ == ButtonIconLoadState::Failed;
    const core::Size resolvedIconSize = reserveIconSlot
        ? resolveButtonIconSlotSize(icon_, iconSize_, style.textStyle.fontSize)
        : core::Size::Make(0.0f, 0.0f);
    const bool hasIcon = resolvedIconSize.width() > 0.0f && resolvedIconSize.height() > 0.0f;
    const bool hasIconImage = static_cast<bool>(icon_);
    const bool showIconPlaceholder = hasIcon && !hasIconImage
        && (iconLoading_ || iconLoadState_ == ButtonIconLoadState::Failed);
    const bool hasLabel = !label_.empty();
    const float contentWidth = metrics.width
        + (hasIcon ? resolvedIconSize.width() : 0.0f)
        + (hasIcon && hasLabel ? style.iconSpacing : 0.0f);
    float cursorX = (bounds_.width() - contentWidth) * 0.5f;
    const float textY = (bounds_.height() - metrics.height) * 0.5f + metrics.baseline;
    const float iconY = (bounds_.height() - resolvedIconSize.height()) * 0.5f;
    const core::Color iconPlaceholder = placeholderColor(
        textColor,
        iconLoadState_ == ButtonIconLoadState::Failed);

    if (hasIcon && iconPlacement_ == ButtonIconPlacement::Start) {
        const core::Rect iconBounds = core::Rect::MakeXYWH(
            cursorX,
            iconY,
            resolvedIconSize.width(),
            resolvedIconSize.height());
        if (hasIconImage) {
            canvas.drawImage(icon_, iconBounds);
        } else if (showIconPlaceholder) {
            drawButtonIconPlaceholder(
                canvas,
                iconBounds,
                iconPlaceholder,
                iconLoadState_ == ButtonIconLoadState::Failed);
        }
        cursorX += resolvedIconSize.width();
        if (hasLabel) {
            cursorX += style.iconSpacing;
        }
    }

    if (hasLabel) {
        canvas.drawText(label_, cursorX + metrics.drawX, textY, style.textStyle.fontSize, textColor);
        cursorX += metrics.width;
    }

    if (hasIcon && iconPlacement_ == ButtonIconPlacement::End) {
        if (hasLabel) {
            cursorX += style.iconSpacing;
        }
        const core::Rect iconBounds = core::Rect::MakeXYWH(
            cursorX,
            iconY,
            resolvedIconSize.width(),
            resolvedIconSize.height());
        if (hasIconImage) {
            canvas.drawImage(icon_, iconBounds);
        } else if (showIconPlaceholder) {
            drawButtonIconPlaceholder(
                canvas,
                iconBounds,
                iconPlaceholder,
                iconLoadState_ == ButtonIconLoadState::Failed);
        }
    }
}

bool Button::onEvent(core::Event& event)
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
        return false;
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
        const bool shouldClick = pressed_ && inside;
        if (pressed_ || hovered_ != inside) {
            pressed_ = false;
            updateHovered(inside);
            markPaintDirty();
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
            updateHovered(false);
            markPaintDirty();
            return true;
        }
        return false;
    }
    default:
        return false;
    }
}

}  // namespace tinalux::ui
