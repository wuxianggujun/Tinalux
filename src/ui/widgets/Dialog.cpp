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

float closeButtonSide(const DialogStyle& style)
{
    return std::max(18.0f, style.titleTextStyle.fontSize * 0.85f);
}

float headerHeight(const DialogStyle& style, const TextMetrics& titleMetrics, bool showCloseButton)
{
    return std::max(titleMetrics.height, showCloseButton ? closeButtonSide(style) : 0.0f);
}

void drawFallbackCloseIcon(rendering::Canvas& canvas, core::Rect bounds, core::Color color)
{
    const float inset = std::max(2.0f, std::min(bounds.width(), bounds.height()) * 0.22f);
    const float stroke = std::max(1.5f, std::min(bounds.width(), bounds.height()) * 0.12f);
    canvas.drawLine(
        bounds.left() + inset,
        bounds.top() + inset,
        bounds.right() - inset,
        bounds.bottom() - inset,
        color,
        stroke,
        true);
    canvas.drawLine(
        bounds.right() - inset,
        bounds.top() + inset,
        bounds.left() + inset,
        bounds.bottom() - inset,
        color,
        stroke,
        true);
}

core::Color closeButtonFill(core::Color titleColor, bool hovered, bool pressed)
{
    const core::Color::Channel alpha = pressed ? 84 : (hovered ? 48 : 20);
    return core::colorARGB(alpha, titleColor.red(), titleColor.green(), titleColor.blue());
}

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

void Dialog::setShowCloseButton(bool show)
{
    if (showCloseButton_ == show) {
        return;
    }

    showCloseButton_ = show;
    closeButtonHovered_ = false;
    closeButtonPressed_ = false;
    markLayoutDirty();
}

bool Dialog::showCloseButton() const
{
    return showCloseButton_;
}

void Dialog::setCloseIcon(rendering::Image icon)
{
    closeIcon_ = std::move(icon);
    markPaintDirty();
}

void Dialog::setCloseIcon(IconType type, float sizeHint)
{
    setCloseIcon(IconRegistry::instance().getIcon(type, sizeHint));
}

const rendering::Image& Dialog::closeIcon() const
{
    return closeIcon_;
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
    const float headerBoxHeight = headerHeight(style, titleMetrics, showCloseButton_);
    const float headerReservation = showCloseButton_
        ? closeButtonSide(style) + (title_.empty() ? 0.0f : style.padding * 0.5f)
        : 0.0f;
    const float availableCardWidth = std::isfinite(constraints.maxWidth)
        ? std::min(style.maxSize.width() > 0.0f ? style.maxSize.width() : constraints.maxWidth, constraints.maxWidth)
        : style.maxSize.width();
    const float availableCardHeight = std::isfinite(constraints.maxHeight)
        ? std::min(style.maxSize.height() > 0.0f ? style.maxSize.height() : constraints.maxHeight, constraints.maxHeight)
        : style.maxSize.height();
    const float maxContentWidth = std::max(0.0f, availableCardWidth - style.padding * 2.0f);
    const float maxContentHeight = std::max(
        0.0f,
        availableCardHeight - style.padding * 2.0f - headerBoxHeight
            - (headerBoxHeight > 0.0f ? style.titleGap : 0.0f));

    Widget* contentWidgetPtr = contentWidget();
    const core::Size contentSize = contentWidgetPtr != nullptr
        ? contentWidgetPtr->measure(Constraints::loose(maxContentWidth, maxContentHeight))
        : core::Size::Make(0.0f, 0.0f);

    const float cardWidth = std::max(titleMetrics.width + headerReservation, contentSize.width()) + style.padding * 2.0f;
    const float cardHeight = style.padding * 2.0f
        + headerBoxHeight
        + (contentWidgetPtr != nullptr && headerBoxHeight > 0.0f ? style.titleGap : 0.0f)
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
    const float headerBoxHeight = headerHeight(style, titleMetrics, showCloseButton_);
    const float headerReservation = showCloseButton_
        ? closeButtonSide(style) + (title_.empty() ? 0.0f : style.padding * 0.5f)
        : 0.0f;
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
            maxCardHeight - style.padding * 2.0f - headerBoxHeight
                - (headerBoxHeight > 0.0f ? style.titleGap : 0.0f));
        contentSize = contentWidgetPtr->measure(Constraints::loose(maxContentWidth, maxContentHeight));
    }

    const float cardWidth = std::min(
        maxCardWidth,
        std::max(titleMetrics.width + headerReservation, contentSize.width()) + style.padding * 2.0f);
    const float cardHeight = std::min(
        maxCardHeight,
        style.padding * 2.0f
            + headerBoxHeight
            + (contentWidget() != nullptr && headerBoxHeight > 0.0f ? style.titleGap : 0.0f)
            + contentSize.height());

    cardBounds_ = core::Rect::MakeXYWH(
        (bounds.width() - cardWidth) * 0.5f,
        (bounds.height() - cardHeight) * 0.5f,
        cardWidth,
        cardHeight);

    if (showCloseButton_) {
        const float side = closeButtonSide(style);
        closeButtonBounds_ = core::Rect::MakeXYWH(
            cardBounds_.right() - style.padding - side,
            cardBounds_.y() + style.padding + (headerBoxHeight - side) * 0.5f,
            side,
            side);
    } else {
        closeButtonBounds_ = core::Rect::MakeEmpty();
    }

    if (Widget* contentWidgetPtr = contentWidget(); contentWidgetPtr != nullptr) {
        const float contentY = cardBounds_.y() + style.padding + headerBoxHeight
            + (headerBoxHeight > 0.0f ? style.titleGap : 0.0f);
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
    const float headerBoxHeight = headerHeight(style, titleMetrics, showCloseButton_);

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
            cardBounds_.y() + style.padding + (headerBoxHeight - titleMetrics.height) * 0.5f + titleMetrics.baseline,
            titleFontSize,
            style.titleColor);
    }

    if (showCloseButton_ && !closeButtonBounds_.isEmpty()) {
        canvas.drawRoundRect(
            closeButtonBounds_,
            std::max(6.0f, closeButtonBounds_.width() * 0.28f),
            std::max(6.0f, closeButtonBounds_.height() * 0.28f),
            closeButtonFill(style.titleColor, closeButtonHovered_, closeButtonPressed_));
        if (closeIcon_) {
            const float inset = std::max(2.0f, closeButtonBounds_.width() * 0.16f);
            canvas.drawImage(
                closeIcon_,
                core::Rect::MakeXYWH(
                    closeButtonBounds_.x() + inset,
                    closeButtonBounds_.y() + inset,
                    std::max(0.0f, closeButtonBounds_.width() - inset * 2.0f),
                    std::max(0.0f, closeButtonBounds_.height() - inset * 2.0f)));
        } else {
            drawFallbackCloseIcon(canvas, closeButtonBounds_, style.titleColor);
        }
    }

    drawChildren(canvas);
}

void Dialog::drawPartial(rendering::Canvas& canvas, const core::Rect& redrawRegion)
{
    if (!canvas || !visible_ || bounds_.isEmpty()) {
        return;
    }

    const core::Rect drawBounds = globalDrawBounds();
    if (drawBounds.isEmpty() || !drawBounds.intersects(redrawRegion) || canvas.quickReject(bounds_)) {
        return;
    }

    canvas.save();
    canvas.translate(bounds_.x(), bounds_.y());
    canvas.clipRect(core::Rect::MakeWH(bounds_.width(), bounds_.height()));

    const DialogStyle style = resolvedStyle();
    const float titleFontSize = style.titleTextStyle.fontSize;
    const TextMetrics titleMetrics = title_.empty()
        ? TextMetrics {}
        : measureTextMetrics(title_, titleFontSize);
    const float headerBoxHeight = headerHeight(style, titleMetrics, showCloseButton_);

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
            cardBounds_.y() + style.padding + (headerBoxHeight - titleMetrics.height) * 0.5f + titleMetrics.baseline,
            titleFontSize,
            style.titleColor);
    }

    if (showCloseButton_ && !closeButtonBounds_.isEmpty()) {
        canvas.drawRoundRect(
            closeButtonBounds_,
            std::max(6.0f, closeButtonBounds_.width() * 0.28f),
            std::max(6.0f, closeButtonBounds_.height() * 0.28f),
            closeButtonFill(style.titleColor, closeButtonHovered_, closeButtonPressed_));
        if (closeIcon_) {
            const float inset = std::max(2.0f, closeButtonBounds_.width() * 0.16f);
            canvas.drawImage(
                closeIcon_,
                core::Rect::MakeXYWH(
                    closeButtonBounds_.x() + inset,
                    closeButtonBounds_.y() + inset,
                    std::max(0.0f, closeButtonBounds_.width() - inset * 2.0f),
                    std::max(0.0f, closeButtonBounds_.height() - inset * 2.0f)));
        } else {
            drawFallbackCloseIcon(canvas, closeButtonBounds_, style.titleColor);
        }
    }

    drawPartialChildren(canvas, redrawRegion);

    canvas.restore();
    clearDirtyState();
}

void Dialog::collectDirtyDrawRegions(std::vector<core::Rect>& regions) const
{
    if (!visible_) {
        return;
    }

    if (layoutDirty_) {
        const core::Rect drawBounds = globalDrawBounds();
        if (!drawBounds.isEmpty()) {
            regions.push_back(drawBounds);
        }
        return;
    }

    if (!hasDirtyRegion()) {
        return;
    }

    if (paintDirty_) {
        regions.push_back(dirtyRegion_);
        return;
    }

    const std::size_t regionCountBeforeChildren = regions.size();
    for (const auto& child : children_) {
        if (child == nullptr || !child->visible() || !child->hasDirtyRegion()) {
            continue;
        }
        child->collectDirtyDrawRegions(regions);
    }

    if (regions.size() == regionCountBeforeChildren && !dirtyRegion_.isEmpty()) {
        regions.push_back(dirtyRegion_);
    }
}

core::Rect Dialog::localDrawBounds() const
{
    return Widget::localDrawBounds();
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
    case core::EventType::MouseMove: {
        const auto& mouseEvent = static_cast<const core::MouseMoveEvent&>(event);
        const core::Point localPoint = globalToLocal(core::Point::Make(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y)));
        const bool hovered = showCloseButton_ && closeButtonBounds_.contains(localPoint);
        if (closeButtonHovered_ != hovered) {
            closeButtonHovered_ = hovered;
            markPaintDirty();
        }
        return closeButtonPressed_;
    }
    case core::EventType::MouseButtonPress: {
        const auto& mouseEvent = static_cast<const core::MouseButtonEvent&>(event);
        const core::Point localPoint = globalToLocal(core::Point::Make(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y)));
        if (mouseEvent.button == core::mouse::kLeft
            && showCloseButton_
            && closeButtonBounds_.contains(localPoint)) {
            closeButtonPressed_ = true;
            closeButtonHovered_ = true;
            backdropPressed_ = false;
            markPaintDirty();
            return true;
        }
        backdropPressed_ = dismissOnBackdrop_ && !cardBounds_.contains(localPoint);
        return backdropPressed_;
    }
    case core::EventType::MouseButtonRelease: {
        const auto& mouseEvent = static_cast<const core::MouseButtonEvent&>(event);
        const core::Point localPoint = globalToLocal(core::Point::Make(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y)));
        if (closeButtonPressed_) {
            const bool shouldDismiss = closeButtonBounds_.contains(localPoint);
            closeButtonPressed_ = false;
            closeButtonHovered_ = showCloseButton_ && closeButtonBounds_.contains(localPoint);
            markPaintDirty();
            if (shouldDismiss) {
                dismiss();
                return true;
            }
            return false;
        }
        const bool shouldDismiss = backdropPressed_ && dismissOnBackdrop_ && !cardBounds_.contains(localPoint);
        backdropPressed_ = false;
        if (shouldDismiss) {
            dismiss();
            return true;
        }
        return false;
    }
    case core::EventType::MouseLeave:
        if (closeButtonHovered_ || closeButtonPressed_) {
            closeButtonHovered_ = false;
            closeButtonPressed_ = false;
            markPaintDirty();
        }
        return false;
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
