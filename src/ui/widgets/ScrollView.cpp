#include "tinalux/ui/ScrollView.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/ui/Theme.h"

namespace tinalux::ui {

namespace {

constexpr float kScrollStep = 48.0f;
constexpr float kScrollbarMargin = 6.0f;
constexpr float kScrollbarWidth = 6.0f;
constexpr float kMinThumbHeight = 24.0f;

}  // namespace

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
    markDirty();
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
    markDirty();
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

SkSize ScrollView::measure(const Constraints& constraints)
{
    if (content_ == nullptr) {
        return constraints.constrain(SkSize::Make(0.0f, preferredHeight_));
    }

    Constraints contentConstraints = constraints.withMaxHeight(std::numeric_limits<float>::infinity());
    const SkSize contentSize = content_->measure(contentConstraints);
    contentHeight_ = contentSize.height();

    const float desiredHeight = preferredHeight_ > 0.0f
        ? std::min(preferredHeight_, contentHeight_)
        : contentHeight_;
    return constraints.constrain(SkSize::Make(contentSize.width(), desiredHeight));
}

void ScrollView::arrange(const SkRect& bounds)
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
    const SkSize contentSize = content_->measure(contentConstraints);
    contentHeight_ = contentSize.height();
    content_->arrange(SkRect::MakeXYWH(
        0.0f,
        0.0f,
        std::max(bounds.width(), contentSize.width()),
        contentHeight_));
    clampScrollOffset();
}

void ScrollView::onDraw(SkCanvas* canvas)
{
    if (canvas == nullptr || content_ == nullptr) {
        return;
    }

    canvas->save();
    canvas->translate(0.0f, -scrollOffset_);
    content_->draw(canvas);
    canvas->restore();

    const float maxOffset = maxScrollOffset();
    if (maxOffset <= 0.0f || bounds_.height() <= 0.0f) {
        return;
    }

    const Theme& theme = currentTheme();
    const float trackHeight = std::max(0.0f, bounds_.height() - kScrollbarMargin * 2.0f);
    const float thumbHeight = std::clamp(
        bounds_.height() * (bounds_.height() / std::max(bounds_.height(), contentHeight_)),
        kMinThumbHeight,
        trackHeight);
    const float thumbTravel = std::max(0.0f, trackHeight - thumbHeight);
    const float thumbY = kScrollbarMargin
        + (thumbTravel * (scrollOffset_ / std::max(1.0f, maxOffset)));

    SkPaint thumbPaint;
    thumbPaint.setAntiAlias(true);
    thumbPaint.setColor(SkColorSetARGB(
        120,
        SkColorGetR(theme.textSecondary),
        SkColorGetG(theme.textSecondary),
        SkColorGetB(theme.textSecondary)));
    canvas->drawRRect(
        SkRRect::MakeRectXY(
            SkRect::MakeXYWH(
                std::max(0.0f, bounds_.width() - kScrollbarMargin - kScrollbarWidth),
                thumbY,
                kScrollbarWidth,
                thumbHeight),
            kScrollbarWidth * 0.5f,
            kScrollbarWidth * 0.5f),
        thumbPaint);
}

Widget* ScrollView::hitTest(float x, float y)
{
    if (!visible_) {
        return nullptr;
    }

    const SkRect localBounds = SkRect::MakeWH(bounds_.width(), bounds_.height());
    if (!localBounds.contains(x, y)) {
        return nullptr;
    }

    if (content_ != nullptr) {
        if (Widget* target = content_->hitTest(x, y + scrollOffset_); target != nullptr) {
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
    const float nextOffset = std::clamp(
        scrollOffset_ - static_cast<float>(scrollEvent.yOffset) * kScrollStep,
        0.0f,
        maxScrollOffset());
    if (std::abs(nextOffset - scrollOffset_) <= 0.001f) {
        return false;
    }

    scrollOffset_ = nextOffset;
    markDirty();
    return true;
}

void ScrollView::clampScrollOffset()
{
    scrollOffset_ = std::clamp(scrollOffset_, 0.0f, maxScrollOffset());
}

}  // namespace tinalux::ui
