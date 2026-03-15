#include "tinalux/ui/Dialog.h"

#include <algorithm>
#include <cmath>

#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Theme.h"

#include "../TextPrimitives.h"

namespace tinalux::ui {

Dialog::Dialog(std::string title)
    : title_(std::move(title))
{
}

void Dialog::setTitle(const std::string& title)
{
    if (title_ == title) {
        return;
    }

    title_ = title;
    markLayoutDirty();
}

const std::string& Dialog::title() const
{
    return title_;
}

void Dialog::setContent(std::shared_ptr<Widget> content)
{
    if (!children_.empty()) {
        Container::removeChild(children_.front().get());
    }

    if (content != nullptr) {
        Container::addChild(std::move(content));
    } else {
        markLayoutDirty();
    }
}

Widget* Dialog::content() const
{
    return contentWidget();
}

void Dialog::onDismiss(std::function<void()> handler)
{
    onDismiss_ = std::move(handler);
}

void Dialog::setDismissOnBackdrop(bool enabled)
{
    dismissOnBackdrop_ = enabled;
}

bool Dialog::dismissOnBackdrop() const
{
    return dismissOnBackdrop_;
}

void Dialog::setDismissOnEscape(bool enabled)
{
    dismissOnEscape_ = enabled;
}

bool Dialog::dismissOnEscape() const
{
    return dismissOnEscape_;
}

void Dialog::setBackdropColor(core::Color color)
{
    if (backdropColorOverride_ && *backdropColorOverride_ == color) {
        return;
    }

    backdropColorOverride_ = color;
    markPaintDirty();
}

void Dialog::setBackgroundColor(core::Color color)
{
    if (backgroundColorOverride_ && *backgroundColorOverride_ == color) {
        return;
    }

    backgroundColorOverride_ = color;
    markPaintDirty();
}

void Dialog::setCornerRadius(float radius)
{
    const float clampedRadius = std::max(radius, 0.0f);
    if (cornerRadiusOverride_ && *cornerRadiusOverride_ == clampedRadius) {
        return;
    }

    cornerRadiusOverride_ = clampedRadius;
    markPaintDirty();
}

void Dialog::setPadding(float padding)
{
    const float clampedPadding = std::max(padding, 0.0f);
    if (paddingOverride_ && *paddingOverride_ == clampedPadding) {
        return;
    }

    paddingOverride_ = clampedPadding;
    markLayoutDirty();
}

void Dialog::setMaxSize(core::Size size)
{
    const core::Size clampedSize = core::Size::Make(
        std::max(0.0f, size.width()),
        std::max(0.0f, size.height()));
    if (maxSizeOverride_ && *maxSizeOverride_ == clampedSize) {
        return;
    }

    maxSizeOverride_ = clampedSize;
    markLayoutDirty();
}

void Dialog::setStyle(const DialogStyle& style)
{
    customStyle_ = style;
    markLayoutDirty();
}

void Dialog::clearStyle()
{
    if (!customStyle_.has_value()) {
        return;
    }

    customStyle_.reset();
    markLayoutDirty();
}

const DialogStyle* Dialog::style() const
{
    return customStyle_ ? &*customStyle_ : nullptr;
}

bool Dialog::focusable() const
{
    return children_.empty();
}

DialogStyle Dialog::resolvedStyle() const
{
    DialogStyle style = customStyle_ ? *customStyle_ : resolvedTheme().dialogStyle;
    if (backdropColorOverride_) {
        style.backdropColor = *backdropColorOverride_;
    }
    if (backgroundColorOverride_) {
        style.backgroundColor = *backgroundColorOverride_;
    }
    if (cornerRadiusOverride_) {
        style.cornerRadius = *cornerRadiusOverride_;
    }
    if (paddingOverride_) {
        style.padding = *paddingOverride_;
    }
    if (maxSizeOverride_) {
        style.maxSize = *maxSizeOverride_;
    }
    return style;
}

core::Size Dialog::measure(const Constraints& constraints)
{
    const DialogStyle style = resolvedStyle();
    const float titleFontSize = style.titleTextStyle.fontSize;
    const TextMetrics titleMetrics = title_.empty()
        ? TextMetrics {}
        : measureTextMetrics(title_, titleFontSize);
    const float titleHeight = title_.empty() ? 0.0f : titleMetrics.height;
    const float availableCardWidth = std::isfinite(constraints.maxWidth)
        ? std::min(style.maxSize.width() > 0.0f ? style.maxSize.width() : constraints.maxWidth, constraints.maxWidth)
        : style.maxSize.width();
    const float availableCardHeight = std::isfinite(constraints.maxHeight)
        ? std::min(style.maxSize.height() > 0.0f ? style.maxSize.height() : constraints.maxHeight, constraints.maxHeight)
        : style.maxSize.height();
    const float maxContentWidth = std::max(0.0f, availableCardWidth - style.padding * 2.0f);
    const float maxContentHeight = std::max(
        0.0f,
        availableCardHeight - style.padding * 2.0f - titleHeight
            - (title_.empty() ? 0.0f : style.titleGap));

    Widget* contentWidgetPtr = contentWidget();
    const core::Size contentSize = contentWidgetPtr != nullptr
        ? contentWidgetPtr->measure(Constraints::loose(maxContentWidth, maxContentHeight))
        : core::Size::Make(0.0f, 0.0f);

    const float cardWidth = std::max(titleMetrics.width, contentSize.width()) + style.padding * 2.0f;
    const float cardHeight = style.padding * 2.0f
        + titleHeight
        + (contentWidgetPtr != nullptr && !title_.empty() ? style.titleGap : 0.0f)
        + contentSize.height();

    return constraints.constrain(core::Size::Make(cardWidth, cardHeight));
}

void Dialog::arrange(const core::Rect& bounds)
{
    Widget::arrange(bounds);

    const DialogStyle style = resolvedStyle();
    const float titleFontSize = style.titleTextStyle.fontSize;
    const TextMetrics titleMetrics = title_.empty()
        ? TextMetrics {}
        : measureTextMetrics(title_, titleFontSize);
    const float titleHeight = title_.empty() ? 0.0f : titleMetrics.height;
    const float maxCardWidth = std::min(
        bounds.width() * 0.9f,
        style.maxSize.width() > 0.0f ? style.maxSize.width() : bounds.width());
    const float maxCardHeight = std::min(
        bounds.height() * 0.9f,
        style.maxSize.height() > 0.0f ? style.maxSize.height() : bounds.height());

    core::Size contentSize = core::Size::Make(0.0f, 0.0f);
    if (Widget* contentWidgetPtr = contentWidget(); contentWidgetPtr != nullptr) {
        const float maxContentWidth = std::max(0.0f, maxCardWidth - style.padding * 2.0f);
        const float maxContentHeight = std::max(
            0.0f,
            maxCardHeight - style.padding * 2.0f - titleHeight
                - (title_.empty() ? 0.0f : style.titleGap));
        contentSize = contentWidgetPtr->measure(Constraints::loose(maxContentWidth, maxContentHeight));
    }

    const float cardWidth = std::min(
        maxCardWidth,
        std::max(titleMetrics.width, contentSize.width()) + style.padding * 2.0f);
    const float cardHeight = std::min(
        maxCardHeight,
        style.padding * 2.0f
            + titleHeight
            + (contentWidget() != nullptr && !title_.empty() ? style.titleGap : 0.0f)
            + contentSize.height());

    cardBounds_ = core::Rect::MakeXYWH(
        (bounds.width() - cardWidth) * 0.5f,
        (bounds.height() - cardHeight) * 0.5f,
        cardWidth,
        cardHeight);

    if (Widget* contentWidgetPtr = contentWidget(); contentWidgetPtr != nullptr) {
        const float contentY = cardBounds_.y() + style.padding + titleHeight
            + (!title_.empty() ? style.titleGap : 0.0f);
        contentBounds_ = core::Rect::MakeXYWH(
            cardBounds_.x() + style.padding,
            contentY,
            std::max(0.0f, cardBounds_.width() - style.padding * 2.0f),
            contentSize.height());
        contentWidgetPtr->arrange(contentBounds_);
    } else {
        contentBounds_ = core::Rect::MakeEmpty();
    }
}

void Dialog::onDraw(rendering::Canvas& canvas)
{
    if (!canvas) {
        return;
    }

    const DialogStyle style = resolvedStyle();
    const float titleFontSize = style.titleTextStyle.fontSize;
    const TextMetrics titleMetrics = title_.empty()
        ? TextMetrics {}
        : measureTextMetrics(title_, titleFontSize);

    canvas.drawRect(
        core::Rect::MakeWH(bounds_.width(), bounds_.height()),
        style.backdropColor);
    canvas.drawRoundRect(
        cardBounds_,
        style.cornerRadius,
        style.cornerRadius,
        style.backgroundColor);

    if (!title_.empty()) {
        canvas.drawText(
            title_,
            cardBounds_.x() + style.padding + titleMetrics.drawX,
            cardBounds_.y() + style.padding + titleMetrics.baseline,
            titleFontSize,
            style.titleColor);
    }

    drawChildren(canvas);
}

Widget* Dialog::hitTest(float x, float y)
{
    if (!visible_) {
        return nullptr;
    }

    const core::Rect localBounds = core::Rect::MakeWH(bounds_.width(), bounds_.height());
    if (!localBounds.contains(x, y)) {
        return nullptr;
    }

    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        Widget* child = it->get();
        const core::Rect childBounds = child->bounds();
        if (Widget* target = child->hitTest(x - childBounds.x(), y - childBounds.y()); target != nullptr) {
            return target;
        }
    }

    return this;
}

bool Dialog::onEvent(core::Event& event)
{
    switch (event.type()) {
    case core::EventType::MouseButtonPress: {
        const auto& mouseEvent = static_cast<const core::MouseButtonEvent&>(event);
        const core::Point localPoint = globalToLocal(core::Point::Make(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y)));
        backdropPressed_ = dismissOnBackdrop_ && !cardBounds_.contains(localPoint);
        return backdropPressed_;
    }
    case core::EventType::MouseButtonRelease: {
        const auto& mouseEvent = static_cast<const core::MouseButtonEvent&>(event);
        const core::Point localPoint = globalToLocal(core::Point::Make(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y)));
        const bool shouldDismiss = backdropPressed_ && dismissOnBackdrop_ && !cardBounds_.contains(localPoint);
        backdropPressed_ = false;
        if (shouldDismiss) {
            dismiss();
            return true;
        }
        return false;
    }
    case core::EventType::KeyPress: {
        const auto& keyEvent = static_cast<const core::KeyEvent&>(event);
        if (dismissOnEscape_ && keyEvent.key == core::keys::kEscape) {
            dismiss();
            return true;
        }
        return false;
    }
    default:
        return false;
    }
}

Widget* Dialog::contentWidget() const
{
    return children_.empty() ? nullptr : children_.front().get();
}

void Dialog::dismiss()
{
    if (onDismiss_) {
        onDismiss_();
    }
}

}  // namespace tinalux::ui
