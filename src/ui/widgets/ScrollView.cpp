#include "tinalux/ui/ScrollView.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "tinalux/rendering/rendering.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/ui/Theme.h"

namespace tinalux::ui {

void ScrollView::setContent(std::shared_ptr<Widget> content)
{
    if (content_ == content) {
        return;
    }

    if (content_ != nullptr) {
        content_->setParent(nullptr);
    }

    content_ = std::move(content);
    if (content_ != nullptr) {
        content_->setParent(this);
    }

    scrollOffset_ = 0.0f;
    contentHeight_ = 0.0f;
    cachedContentWidget_ = nullptr;
    cachedContentLayoutVersion_ = 0;
    contentMeasureCacheValid_ = false;
    markLayoutDirty();
}

Widget* ScrollView::content() const
{
    return content_.get();
}

void ScrollView::setPreferredHeight(float height)
{
    const float clampedHeight = std::max(0.0f, height);
    if (preferredHeight_ == clampedHeight) {
        return;
    }

    preferredHeight_ = clampedHeight;
    markLayoutDirty();
}

float ScrollView::preferredHeight() const
{
    return preferredHeight_;
}

float ScrollView::scrollOffset() const
{
    return scrollOffset_;
}

float ScrollView::maxScrollOffset() const
{
    return std::max(0.0f, contentHeight_ - bounds_.height());
}

float ScrollView::contentHeight() const
{
    return contentHeight_;
}

void ScrollView::setStyle(const ScrollViewStyle& style)
{
    customStyle_ = style;
    markPaintDirty();
}

void ScrollView::clearStyle()
{
    if (!customStyle_.has_value()) {
        return;
    }

    customStyle_.reset();
    markPaintDirty();
}

const ScrollViewStyle* ScrollView::style() const
{
    return customStyle_ ? &*customStyle_ : nullptr;
}

const ScrollViewStyle& ScrollView::resolvedStyle() const
{
    return customStyle_ ? *customStyle_ : resolvedTheme().scrollViewStyle;
}

core::Size ScrollView::measure(const Constraints& constraints)
{
    if (content_ == nullptr) {
        return constraints.constrain(core::Size::Make(0.0f, preferredHeight_));
    }

    Constraints contentConstraints = constraints.withMaxHeight(std::numeric_limits<float>::infinity());
    const bool canReuseContentMeasure = contentMeasureCacheValid_
        && cachedContentWidget_ == content_.get()
        && cachedContentLayoutVersion_ == content_->layoutVersion()
        && cachedContentConstraints_.minWidth == contentConstraints.minWidth
        && cachedContentConstraints_.maxWidth == contentConstraints.maxWidth
        && cachedContentConstraints_.minHeight == contentConstraints.minHeight
        && cachedContentConstraints_.maxHeight == contentConstraints.maxHeight;
    const core::Size contentSize = canReuseContentMeasure
        ? cachedContentSize_
        : content_->measure(contentConstraints);
    if (!canReuseContentMeasure) {
        cachedContentWidget_ = content_.get();
        cachedContentConstraints_ = contentConstraints;
        cachedContentSize_ = contentSize;
        cachedContentLayoutVersion_ = content_->layoutVersion();
        contentMeasureCacheValid_ = true;
    }
    contentHeight_ = contentSize.height();

    const float desiredHeight = preferredHeight_ > 0.0f
        ? std::min(preferredHeight_, contentHeight_)
        : contentHeight_;
    return constraints.constrain(core::Size::Make(contentSize.width(), desiredHeight));
}

void ScrollView::arrange(const core::Rect& bounds)
{
    Widget::arrange(bounds);

    if (content_ == nullptr) {
        contentHeight_ = 0.0f;
        scrollOffset_ = 0.0f;
        return;
    }

    const Constraints contentConstraints {
        .minWidth = 0.0f,
        .maxWidth = bounds.width(),
        .minHeight = 0.0f,
        .maxHeight = std::numeric_limits<float>::infinity(),
    };
    const bool canReuseContentMeasure = contentMeasureCacheValid_
        && cachedContentWidget_ == content_.get()
        && cachedContentLayoutVersion_ == content_->layoutVersion()
        && cachedContentConstraints_.minWidth == contentConstraints.minWidth
        && cachedContentConstraints_.maxWidth == contentConstraints.maxWidth
        && cachedContentConstraints_.minHeight == contentConstraints.minHeight
        && cachedContentConstraints_.maxHeight == contentConstraints.maxHeight;
    const core::Size contentSize = canReuseContentMeasure
        ? cachedContentSize_
        : content_->measure(contentConstraints);
    if (!canReuseContentMeasure) {
        cachedContentWidget_ = content_.get();
        cachedContentConstraints_ = contentConstraints;
        cachedContentSize_ = contentSize;
        cachedContentLayoutVersion_ = content_->layoutVersion();
        contentMeasureCacheValid_ = true;
    }
    contentHeight_ = contentSize.height();
    content_->arrange(core::Rect::MakeXYWH(
        0.0f,
        0.0f,
        std::max(bounds.width(), contentSize.width()),
        contentHeight_));
    clampScrollOffset();
}

void ScrollView::onDraw(rendering::Canvas& canvas)
{
    if (!canvas || content_ == nullptr) {
        return;
    }

    const ScrollViewStyle& style = resolvedStyle();

    canvas.save();
    canvas.translate(0.0f, -scrollOffset_);
    content_->draw(canvas);
    canvas.restore();

    const float maxOffset = maxScrollOffset();
    if (maxOffset <= 0.0f || bounds_.height() <= 0.0f) {
        return;
    }

    const float trackHeight =
        std::max(0.0f, bounds_.height() - style.scrollbarMargin * 2.0f);
    const float thumbHeight = std::clamp(
        bounds_.height() * (bounds_.height() / std::max(bounds_.height(), contentHeight_)),
        style.minThumbHeight,
        trackHeight);
    const float thumbTravel = std::max(0.0f, trackHeight - thumbHeight);
    const float thumbY = style.scrollbarMargin
        + (thumbTravel * (scrollOffset_ / std::max(1.0f, maxOffset)));

    const float scrollbarX = std::max(
        0.0f,
        bounds_.width() - style.scrollbarMargin - style.scrollbarWidth);
    const float scrollbarRadius = style.scrollbarWidth * 0.5f;

    if (style.scrollbarTrackColor.alpha() > 0) {
        canvas.drawRoundRect(
            core::Rect::MakeXYWH(
                scrollbarX,
                style.scrollbarMargin,
                style.scrollbarWidth,
                trackHeight),
            scrollbarRadius,
            scrollbarRadius,
            style.scrollbarTrackColor);
    }

    canvas.drawRoundRect(
        core::Rect::MakeXYWH(
            scrollbarX,
            thumbY,
            style.scrollbarWidth,
            thumbHeight),
        scrollbarRadius,
        scrollbarRadius,
        style.scrollbarThumbColor);
}

Widget* ScrollView::hitTest(float x, float y)
{
    if (!visible_) {
        return nullptr;
    }

    const core::Rect localBounds = core::Rect::MakeWH(bounds_.width(), bounds_.height());
    if (!localBounds.contains(x, y)) {
        return nullptr;
    }

    if (content_ != nullptr) {
        const core::Point contentOrigin = content_->parentAdjustedOrigin();
        if (Widget* target = content_->hitTest(x - contentOrigin.x(), y - contentOrigin.y());
            target != nullptr) {
            return target;
        }
    }

    return this;
}

bool ScrollView::onEvent(core::Event& event)
{
    if (event.type() != core::EventType::MouseScroll) {
        return false;
    }

    const auto& scrollEvent = static_cast<const core::MouseScrollEvent&>(event);
    const ScrollViewStyle& style = resolvedStyle();
    const float nextOffset = std::clamp(
        scrollOffset_ - static_cast<float>(scrollEvent.yOffset) * style.scrollStep,
        0.0f,
        maxScrollOffset());
    if (std::abs(nextOffset - scrollOffset_) <= 0.001f) {
        return false;
    }

    scrollOffset_ = nextOffset;
    markPaintDirty();
    return true;
}

void ScrollView::clampScrollOffset()
{
    scrollOffset_ = std::clamp(scrollOffset_, 0.0f, maxScrollOffset());
}

core::Point ScrollView::childOffsetAdjustment(const Widget& child) const
{
    if (content_ != nullptr && &child == content_.get()) {
        return core::Point::Make(0.0f, -scrollOffset_);
    }
    return Widget::childOffsetAdjustment(child);
}

}  // namespace tinalux::ui
