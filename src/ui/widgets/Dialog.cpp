#include "tinalux/ui/Dialog.h"

#include <algorithm>
#include <cmath>

#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Theme.h"

#include "../TextPrimitives.h"

namespace tinalux::ui {

namespace {

constexpr float kTitleGap = 12.0f;

}  // namespace

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
    if (backdropColor_ == color) {
        return;
    }

    backdropColor_ = color;
    markPaintDirty();
}

void Dialog::setBackgroundColor(core::Color color)
{
    if (backgroundColor_ == color) {
        return;
    }

    backgroundColor_ = color;
    markPaintDirty();
}

void Dialog::setCornerRadius(float radius)
{
    const float clampedRadius = std::max(radius, 0.0f);
    if (cornerRadius_ == clampedRadius) {
        return;
    }

    cornerRadius_ = clampedRadius;
    markPaintDirty();
}

void Dialog::setPadding(float padding)
{
    const float clampedPadding = std::max(padding, 0.0f);
    if (padding_ == clampedPadding) {
        return;
    }

    padding_ = clampedPadding;
    markLayoutDirty();
}

void Dialog::setMaxSize(core::Size size)
{
    const core::Size clampedSize = core::Size::Make(
        std::max(0.0f, size.width()),
        std::max(0.0f, size.height()));
    if (maxSize_ == clampedSize) {
        return;
    }

    maxSize_ = clampedSize;
    markLayoutDirty();
}

bool Dialog::focusable() const
{
    return children_.empty();
}

core::Size Dialog::measure(const Constraints& constraints)
{
    const Theme& theme = resolvedTheme();
    const float titleFontSize = theme.fontSizeLarge;
    const TextMetrics titleMetrics = title_.empty()
        ? TextMetrics {}
        : measureTextMetrics(title_, titleFontSize);
    const float titleHeight = title_.empty() ? 0.0f : titleMetrics.height;
    const float availableCardWidth = std::isfinite(constraints.maxWidth)
        ? std::min(maxSize_.width() > 0.0f ? maxSize_.width() : constraints.maxWidth, constraints.maxWidth)
        : maxSize_.width();
    const float availableCardHeight = std::isfinite(constraints.maxHeight)
        ? std::min(maxSize_.height() > 0.0f ? maxSize_.height() : constraints.maxHeight, constraints.maxHeight)
        : maxSize_.height();
    const float maxContentWidth = std::max(0.0f, availableCardWidth - padding_ * 2.0f);
    const float maxContentHeight = std::max(
        0.0f,
        availableCardHeight - padding_ * 2.0f - titleHeight - (title_.empty() ? 0.0f : kTitleGap));

    Widget* contentWidgetPtr = contentWidget();
    const core::Size contentSize = contentWidgetPtr != nullptr
        ? contentWidgetPtr->measure(Constraints::loose(maxContentWidth, maxContentHeight))
        : core::Size::Make(0.0f, 0.0f);

    const float cardWidth = std::max(titleMetrics.width, contentSize.width()) + padding_ * 2.0f;
    const float cardHeight = padding_ * 2.0f
        + titleHeight
        + (contentWidgetPtr != nullptr && !title_.empty() ? kTitleGap : 0.0f)
        + contentSize.height();

    return constraints.constrain(core::Size::Make(cardWidth, cardHeight));
}

void Dialog::arrange(const core::Rect& bounds)
{
    Widget::arrange(bounds);

    const Theme& theme = resolvedTheme();
    const float titleFontSize = theme.fontSizeLarge;
    const TextMetrics titleMetrics = title_.empty()
        ? TextMetrics {}
        : measureTextMetrics(title_, titleFontSize);
    const float titleHeight = title_.empty() ? 0.0f : titleMetrics.height;
    const float maxCardWidth = std::min(
        bounds.width() * 0.9f,
        maxSize_.width() > 0.0f ? maxSize_.width() : bounds.width());
    const float maxCardHeight = std::min(
        bounds.height() * 0.9f,
        maxSize_.height() > 0.0f ? maxSize_.height() : bounds.height());

    core::Size contentSize = core::Size::Make(0.0f, 0.0f);
    if (Widget* contentWidgetPtr = contentWidget(); contentWidgetPtr != nullptr) {
        const float maxContentWidth = std::max(0.0f, maxCardWidth - padding_ * 2.0f);
        const float maxContentHeight = std::max(
            0.0f,
            maxCardHeight - padding_ * 2.0f - titleHeight - (title_.empty() ? 0.0f : kTitleGap));
        contentSize = contentWidgetPtr->measure(Constraints::loose(maxContentWidth, maxContentHeight));
    }

    const float cardWidth = std::min(
        maxCardWidth,
        std::max(titleMetrics.width, contentSize.width()) + padding_ * 2.0f);
    const float cardHeight = std::min(
        maxCardHeight,
        padding_ * 2.0f
            + titleHeight
            + (contentWidget() != nullptr && !title_.empty() ? kTitleGap : 0.0f)
            + contentSize.height());

    cardBounds_ = core::Rect::MakeXYWH(
        (bounds.width() - cardWidth) * 0.5f,
        (bounds.height() - cardHeight) * 0.5f,
        cardWidth,
        cardHeight);

    if (Widget* contentWidgetPtr = contentWidget(); contentWidgetPtr != nullptr) {
        const float contentY = cardBounds_.y() + padding_ + titleHeight
            + (!title_.empty() ? kTitleGap : 0.0f);
        contentBounds_ = core::Rect::MakeXYWH(
            cardBounds_.x() + padding_,
            contentY,
            std::max(0.0f, cardBounds_.width() - padding_ * 2.0f),
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

    const Theme& theme = resolvedTheme();
    const float titleFontSize = theme.fontSizeLarge;
    const TextMetrics titleMetrics = title_.empty()
        ? TextMetrics {}
        : measureTextMetrics(title_, titleFontSize);

    canvas.drawRect(
        core::Rect::MakeWH(bounds_.width(), bounds_.height()),
        backdropColor_);
    canvas.drawRoundRect(
        cardBounds_,
        cornerRadius_,
        cornerRadius_,
        backgroundColor_);

    if (!title_.empty()) {
        canvas.drawText(
            title_,
            cardBounds_.x() + padding_ + titleMetrics.drawX,
            cardBounds_.y() + padding_ + titleMetrics.baseline,
            titleFontSize,
            theme.text);
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
