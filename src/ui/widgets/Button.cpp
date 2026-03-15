#include "tinalux/ui/Button.h"

#include <algorithm>

#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Theme.h"

#include "../TextPrimitives.h"

namespace tinalux::ui {

namespace {

constexpr float kVerticalPadding = 10.0f;
constexpr float kIconSpacing = 8.0f;

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

}  // namespace

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
    markLayoutDirty();
}

void Button::setIcon(rendering::Image icon)
{
    icon_ = std::move(icon);
    markLayoutDirty();
}

const rendering::Image& Button::icon() const
{
    return icon_;
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

void Button::onClick(std::function<void()> handler)
{
    onClick_ = std::move(handler);
}

bool Button::focusable() const
{
    return true;
}

core::Size Button::measure(const Constraints& constraints)
{
    const Theme& theme = resolvedTheme();

    const TextMetrics metrics = measureTextMetrics(label_, theme.fontSize);
    const core::Size resolvedIconSize = resolveButtonIconSize(icon_, iconSize_);
    const bool hasIcon = resolvedIconSize.width() > 0.0f && resolvedIconSize.height() > 0.0f;
    const bool hasLabel = !label_.empty();
    const float contentWidth = metrics.width
        + (hasIcon ? resolvedIconSize.width() : 0.0f)
        + (hasIcon && hasLabel ? kIconSpacing : 0.0f);
    const float contentHeight = std::max(metrics.height, resolvedIconSize.height());

    return constraints.constrain(core::Size::Make(
        contentWidth + theme.padding * 2.0f,
        contentHeight + kVerticalPadding * 2.0f));
}

void Button::onDraw(rendering::Canvas& canvas)
{
    if (!canvas) {
        return;
    }

    const Theme& theme = resolvedTheme();
    const core::Color backgroundColor = pressed_
        ? theme.primary
        : hovered_
            ? theme.border
            : theme.surface;
    const core::Color textColor = pressed_
        ? theme.onPrimary
        : theme.text;

    canvas.drawRoundRect(
        core::Rect::MakeWH(bounds_.width(), bounds_.height()),
        theme.cornerRadius,
        theme.cornerRadius,
        backgroundColor);

    if (focused()) {
        canvas.drawRoundRect(
            core::Rect::MakeXYWH(1.0f, 1.0f, bounds_.width() - 2.0f, bounds_.height() - 2.0f),
            theme.cornerRadius - 1.0f,
            theme.cornerRadius - 1.0f,
            theme.primary,
            rendering::PaintStyle::Stroke,
            2.0f);
    }

    const TextMetrics metrics = measureTextMetrics(label_, theme.fontSize);
    const core::Size resolvedIconSize = resolveButtonIconSize(icon_, iconSize_);
    const bool hasIcon = resolvedIconSize.width() > 0.0f && resolvedIconSize.height() > 0.0f;
    const bool hasLabel = !label_.empty();
    const float contentWidth = metrics.width
        + (hasIcon ? resolvedIconSize.width() : 0.0f)
        + (hasIcon && hasLabel ? kIconSpacing : 0.0f);
    float cursorX = (bounds_.width() - contentWidth) * 0.5f;
    const float textY = (bounds_.height() - metrics.height) * 0.5f + metrics.baseline;
    const float iconY = (bounds_.height() - resolvedIconSize.height()) * 0.5f;

    if (hasIcon && iconPlacement_ == ButtonIconPlacement::Start) {
        canvas.drawImage(
            icon_,
            core::Rect::MakeXYWH(cursorX, iconY, resolvedIconSize.width(), resolvedIconSize.height()));
        cursorX += resolvedIconSize.width();
        if (hasLabel) {
            cursorX += kIconSpacing;
        }
    }

    if (hasLabel) {
        canvas.drawText(label_, cursorX + metrics.drawX, textY, theme.fontSize, textColor);
        cursorX += metrics.width;
    }

    if (hasIcon && iconPlacement_ == ButtonIconPlacement::End) {
        if (hasLabel) {
            cursorX += kIconSpacing;
        }
        canvas.drawImage(
            icon_,
            core::Rect::MakeXYWH(cursorX, iconY, resolvedIconSize.width(), resolvedIconSize.height()));
    }
}

bool Button::onEvent(core::Event& event)
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
            if (!pressed_) {
                markPaintDirty();
            }
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
        const bool shouldClick = pressed_ && inside;
        if (pressed_ || hovered_ != inside) {
            pressed_ = false;
            hovered_ = inside;
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
            hovered_ = false;
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
