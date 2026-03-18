#include "tinalux/ui/Container.h"

#include <algorithm>

#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Layout.h"

namespace tinalux::ui {

namespace {

void clearFocusRecursive(Widget* widget)
{
    if (widget == nullptr) {
        return;
    }

    if (widget->focused()) {
        widget->setFocused(false);
    }

    if (auto* container = dynamic_cast<Container*>(widget); container != nullptr) {
        for (const auto& child : container->children()) {
            clearFocusRecursive(child.get());
        }
    }
}

}  // namespace

Container::~Container()
{
    for (const auto& child : children_) {
        if (child != nullptr) {
            child->setParent(nullptr);
        }
    }
}

void Container::addChild(std::shared_ptr<Widget> child)
{
    if (child == nullptr) {
        return;
    }

    if (child->parent() == this) {
        const auto existing = std::find_if(
            children_.begin(),
            children_.end(),
            [&child](const std::shared_ptr<Widget>& candidate) {
                return candidate.get() == child.get();
            });
        if (existing != children_.end()) {
            return;
        }
    }
    if (Widget* existingParent = child->parent();
        existingParent != nullptr && existingParent != this) {
        if (auto* existingContainer = dynamic_cast<Container*>(existingParent);
            existingContainer != nullptr) {
            existingContainer->removeChildInternal(child.get(), true);
        } else {
            child->setParent(nullptr);
        }
    }

    child->setParent(this);
    children_.push_back(std::move(child));
    invalidateLocalDrawBoundsCache();
    markLayoutDirty();
}

void Container::removeChild(Widget* child)
{
    removeChildInternal(child, false);
}

void Container::clearChildren()
{
    if (children_.empty()) {
        return;
    }

    for (const auto& child : children_) {
        clearFocusRecursive(child.get());
        child->setParent(nullptr);
    }
    children_.clear();
    invalidateLocalDrawBoundsCache();
    markLayoutDirty();
}

void Container::removeChildInternal(Widget* child, bool preserveFocusState)
{
    if (child == nullptr) {
        return;
    }

    const auto it = std::find_if(
        children_.begin(),
        children_.end(),
        [child](const std::shared_ptr<Widget>& candidate) {
            return candidate.get() == child;
        });

    if (it != children_.end()) {
        if (!preserveFocusState) {
            clearFocusRecursive(it->get());
        }
        (*it)->setParent(nullptr);
        children_.erase(it);
        invalidateLocalDrawBoundsCache();
        markLayoutDirty();
    }
}

void Container::setLayout(std::unique_ptr<Layout> layout)
{
    layout_ = std::move(layout);
    markLayoutDirty();
}

core::Size Container::measure(const Constraints& constraints)
{
    if (layout_ != nullptr) {
        return layout_->measure(constraints, children_);
    }

    if (!children_.empty()) {
        return children_.front()->measure(constraints);
    }

    return constraints.constrain(core::Size::Make(0.0f, 0.0f));
}

void Container::arrange(const core::Rect& bounds)
{
    Widget::arrange(bounds);

    // 子组件布局使用容器本地坐标，避免重复叠加父组件偏移。
    const core::Rect contentBounds = core::Rect::MakeWH(bounds.width(), bounds.height());
    if (layout_ != nullptr) {
        layout_->arrange(contentBounds, children_);
        return;
    }

    if (!children_.empty()) {
        children_.front()->arrange(contentBounds);
    }
}

void Container::onDraw(rendering::Canvas& canvas)
{
    drawChildren(canvas);
}

void Container::drawPartial(rendering::Canvas& canvas, const core::Rect& redrawRegion)
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
    drawPartialChildren(canvas, redrawRegion);
    canvas.restore();
    clearDirtyState();
}

Widget* Container::hitTest(float x, float y)
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
        const core::Point childOrigin = child->parentAdjustedOrigin();
        Widget* target = child->hitTest(x - childOrigin.x(), y - childOrigin.y());
        if (target != nullptr) {
            return target;
        }
    }

    return this;
}

const std::vector<std::shared_ptr<Widget>>& Container::children() const
{
    return children_;
}

core::Rect Container::localDrawBounds() const
{
    if (cachedLocalDrawBoundsLayoutVersion_ == layoutVersion()
        && cachedLocalDrawBoundsRevision_ == localDrawBoundsRevision_) {
        return cachedLocalDrawBounds_;
    }

    core::Rect bounds = core::Rect::MakeEmpty();
    for (const auto& child : children_) {
        if (child == nullptr || !child->visible()) {
            continue;
        }
        bounds.join(child->drawBoundsInParent());
    }
    cachedLocalDrawBounds_ = bounds;
    cachedLocalDrawBoundsLayoutVersion_ = layoutVersion();
    cachedLocalDrawBoundsRevision_ = localDrawBoundsRevision_;
    return cachedLocalDrawBounds_;
}

void Container::drawChildren(rendering::Canvas& canvas)
{
    for (const auto& child : children_) {
        const core::Rect childDrawBounds = child != nullptr
            ? child->drawBoundsInParent()
            : core::Rect::MakeEmpty();
        if (child == nullptr
            || !child->visible()
            || childDrawBounds.isEmpty()
            || canvas.quickReject(childDrawBounds)) {
            continue;
        }
        child->draw(canvas);
    }
}

void Container::drawPartialChildren(rendering::Canvas& canvas, const core::Rect& redrawRegion)
{
    for (const auto& child : children_) {
        if (child == nullptr || !child->visible()) {
            continue;
        }

        const core::Rect childDrawBounds = child->globalDrawBounds();
        if (childDrawBounds.isEmpty() || !childDrawBounds.intersects(redrawRegion)) {
            continue;
        }

        child->drawPartial(canvas, redrawRegion);
    }
}

void Container::replaceChildrenDirect(std::vector<std::shared_ptr<Widget>> children)
{
    for (const auto& child : children_) {
        child->setParent(nullptr);
    }

    children_.clear();
    children_.reserve(children.size());
    for (auto& child : children) {
        if (child == nullptr) {
            continue;
        }
        child->setParent(this);
        children_.push_back(std::move(child));
    }
    invalidateLocalDrawBoundsCache();
}

void Container::invalidateLocalDrawBoundsCache()
{
    ++localDrawBoundsRevision_;
    cachedLocalDrawBoundsLayoutVersion_ = 0;
    cachedLocalDrawBoundsRevision_ = 0;
    cachedLocalDrawBounds_ = core::Rect::MakeEmpty();
}

}  // namespace tinalux::ui
