#include "tinalux/ui/Widget.h"

#include <cmath>

#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/ScrollView.h"
#include "RuntimeState.h"

namespace tinalux::ui {

namespace {

bool sameRect(const core::Rect& lhs, const core::Rect& rhs)
{
    constexpr float kTolerance = 0.001f;
    return std::abs(lhs.left() - rhs.left()) <= kTolerance
        && std::abs(lhs.top() - rhs.top()) <= kTolerance
        && std::abs(lhs.right() - rhs.right()) <= kTolerance
        && std::abs(lhs.bottom() - rhs.bottom()) <= kTolerance;
}

}  // namespace

const Theme& Widget::resolvedTheme() const
{
    return runtimeTheme();
}

AnimationSink& Widget::animationSink() const
{
    return runtimeAnimationSink();
}

float Widget::resolvedDevicePixelRatio() const
{
    return runtimeDevicePixelRatio();
}

std::uint64_t Widget::resolvedThemeGeneration() const
{
    return runtimeThemeGeneration();
}

core::Point Widget::childOffsetAdjustment(const Widget&) const
{
    return core::Point::Make(0.0f, 0.0f);
}

core::Point Widget::parentAdjustedOrigin() const
{
    core::Point origin = core::Point::Make(bounds_.x(), bounds_.y());
    if (parent_ != nullptr) {
        const core::Point adjustment = parent_->childOffsetAdjustment(*this);
        origin.offset(adjustment.x(), adjustment.y());
    }
    return origin;
}

void Widget::arrange(const core::Rect& bounds)
{
    if (!sameRect(bounds_, bounds)) {
        const core::Rect previousGlobalBounds = globalBounds();
        markDirtyRect(previousGlobalBounds);
        bounds_ = bounds;
        markPaintDirty();
        layoutDirty_ = false;
        return;
    }

    bounds_ = bounds;
    layoutDirty_ = false;
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

    const core::Rect localBounds = core::Rect::MakeWH(bounds_.width(), bounds_.height());
    return localBounds.contains(x, y) ? this : nullptr;
}

Widget* Widget::hitTestGlobal(float x, float y)
{
    const core::Point localPoint = globalToLocal(core::Point::Make(x, y));
    return hitTest(localPoint.x(), localPoint.y());
}

void Widget::draw(rendering::Canvas& canvas)
{
    if (!canvas || !visible_ || bounds_.isEmpty()) {
        return;
    }

    if (canvas.quickReject(bounds_)) {
        return;
    }

    canvas.save();
    canvas.translate(bounds_.x(), bounds_.y());
    canvas.clipRect(core::Rect::MakeWH(bounds_.width(), bounds_.height()));
    onDraw(canvas);
    canvas.restore();
    paintDirty_ = false;
    subtreePaintDirty_ = false;
    dirtyRegion_ = core::Rect::MakeEmpty();
}

core::Rect Widget::bounds() const
{
    return bounds_;
}

core::Point Widget::localToGlobal(core::Point point) const
{
    for (const Widget* current = this; current != nullptr; current = current->parent_) {
        const core::Point origin = current->parentAdjustedOrigin();
        point.offset(origin.x(), origin.y());
    }
    return point;
}

core::Point Widget::globalToLocal(core::Point point) const
{
    const core::Point globalOrigin = localToGlobal();
    point.offset(-globalOrigin.x(), -globalOrigin.y());
    return point;
}

core::Rect Widget::globalBounds() const
{
    const core::Point origin = localToGlobal();
    return core::Rect::MakeXYWH(origin.x(), origin.y(), bounds_.width(), bounds_.height());
}

bool Widget::containsLocalPoint(float x, float y) const
{
    if (!visible_) {
        return false;
    }

    const core::Rect localBounds = core::Rect::MakeWH(bounds_.width(), bounds_.height());
    return localBounds.contains(x, y);
}

bool Widget::containsGlobalPoint(float x, float y) const
{
    const core::Point localPoint = globalToLocal(core::Point::Make(x, y));
    return containsLocalPoint(localPoint.x(), localPoint.y());
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
    markLayoutDirty();
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
    markPaintDirty();
}

bool Widget::focusable() const
{
    return false;
}

bool Widget::wantsTextInput() const
{
    return false;
}

std::optional<core::Rect> Widget::imeCursorRect()
{
    return std::nullopt;
}

void Widget::markLayoutDirty()
{
    const bool alreadyLayoutDirty = layoutDirty_;
    const bool alreadySubtreePaintDirty = subtreePaintDirty_;
    paintDirty_ = true;
    subtreePaintDirty_ = true;
    layoutDirty_ = true;
    if (!alreadyLayoutDirty) {
        ++layoutVersion_;
    }
    markDirtyRect(globalBounds());

    if (alreadyLayoutDirty && alreadySubtreePaintDirty) {
        return;
    }

    if (parent_ != nullptr) {
        parent_->markAncestorLayoutDirty();
    }
}

void Widget::markPaintDirty()
{
    markDirtyRect(globalBounds());
    paintDirty_ = true;
    markSubtreePaintDirty();
}

bool Widget::isDirty() const
{
    return paintDirty_ || subtreePaintDirty_ || layoutDirty_;
}

bool Widget::isLayoutDirty() const
{
    return layoutDirty_;
}

std::uint64_t Widget::layoutVersion() const
{
    return layoutVersion_;
}

bool Widget::hasDirtyRegion() const
{
    return !dirtyRegion_.isEmpty();
}

core::Rect Widget::dirtyRegion() const
{
    return dirtyRegion_;
}

Widget* Widget::parent() const
{
    return parent_;
}

std::shared_ptr<Widget> Widget::sharedHandle()
{
    return weak_from_this().lock();
}

std::weak_ptr<Widget> Widget::weakHandle()
{
    return weak_from_this();
}

void Widget::markDirtyRect(const core::Rect& rect)
{
    if (rect.isEmpty()) {
        return;
    }

    if (dirtyRegion_.isEmpty()) {
        dirtyRegion_ = rect;
    } else {
        dirtyRegion_.join(rect);
    }

    if (parent_ != nullptr) {
        parent_->markDirtyRect(rect);
    }
}

void Widget::setParent(Widget* parent)
{
    parent_ = parent;
}

void Widget::markAncestorLayoutDirty()
{
    const bool alreadyLayoutDirty = layoutDirty_;
    const bool alreadySubtreePaintDirty = subtreePaintDirty_;
    subtreePaintDirty_ = true;
    layoutDirty_ = true;
    markDirtyRect(globalBounds());
    if (!alreadyLayoutDirty) {
        ++layoutVersion_;
    }

    if (parent_ != nullptr && !(alreadyLayoutDirty && alreadySubtreePaintDirty)) {
        parent_->markAncestorLayoutDirty();
    }
}

void Widget::markSubtreePaintDirty()
{
    const bool alreadySubtreePaintDirty = subtreePaintDirty_;
    subtreePaintDirty_ = true;

    if (parent_ != nullptr && !alreadySubtreePaintDirty) {
        parent_->markSubtreePaintDirty();
    }
}

}  // namespace tinalux::ui
