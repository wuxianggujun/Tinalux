#include "tinalux/ui/Widget.h"

#include <cmath>

#include "include/core/SkCanvas.h"
#include "tinalux/ui/ScrollView.h"

namespace tinalux::ui {

namespace {

bool sameRect(const SkRect& lhs, const SkRect& rhs)
{
    constexpr float kTolerance = 0.001f;
    return std::abs(lhs.left() - rhs.left()) <= kTolerance
        && std::abs(lhs.top() - rhs.top()) <= kTolerance
        && std::abs(lhs.right() - rhs.right()) <= kTolerance
        && std::abs(lhs.bottom() - rhs.bottom()) <= kTolerance;
}

}  // namespace

void Widget::arrange(const SkRect& bounds)
{
    if (!sameRect(bounds_, bounds)) {
        bounds_ = bounds;
        markDirty();
        return;
    }

    bounds_ = bounds;
}

bool Widget::onEventCapture(core::Event&)
{
    return false;
}

bool Widget::onEvent(core::Event&)
{
    return false;
}

Widget* Widget::hitTest(float x, float y)
{
    if (!visible_) {
        return nullptr;
    }

    const SkRect localBounds = SkRect::MakeWH(bounds_.width(), bounds_.height());
    return localBounds.contains(x, y) ? this : nullptr;
}

void Widget::draw(SkCanvas* canvas)
{
    if (canvas == nullptr || !visible_ || bounds_.isEmpty()) {
        return;
    }

    canvas->save();
    canvas->translate(bounds_.x(), bounds_.y());
    canvas->clipRect(SkRect::MakeWH(bounds_.width(), bounds_.height()));
    onDraw(canvas);
    canvas->restore();
    dirty_ = false;
}

SkRect Widget::bounds() const
{
    return bounds_;
}

SkRect Widget::globalBounds() const
{
    SkRect result = bounds_;
    for (const Widget* current = parent_; current != nullptr; current = current->parent_) {
        result.offset(current->bounds_.x(), current->bounds_.y());
        if (const auto* scrollView = dynamic_cast<const ScrollView*>(current); scrollView != nullptr) {
            result.offset(0.0f, -scrollView->scrollOffset());
        }
    }
    return result;
}

bool Widget::containsGlobalPoint(float x, float y) const
{
    return visible_ && globalBounds().contains(x, y);
}

bool Widget::visible() const
{
    return visible_;
}

void Widget::setVisible(bool visible)
{
    if (visible_ == visible) {
        return;
    }

    visible_ = visible;
    markDirty();
}

bool Widget::focused() const
{
    return focused_;
}

void Widget::setFocused(bool focused)
{
    if (focused_ == focused) {
        return;
    }

    focused_ = focused;
    markDirty();
}

bool Widget::focusable() const
{
    return false;
}

void Widget::markDirty()
{
    if (dirty_) {
        return;
    }

    dirty_ = true;
    if (parent_ != nullptr) {
        parent_->markDirty();
    }
}

bool Widget::isDirty() const
{
    return dirty_;
}

Widget* Widget::parent() const
{
    return parent_;
}

void Widget::setParent(Widget* parent)
{
    parent_ = parent;
}

}  // namespace tinalux::ui
