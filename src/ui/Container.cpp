#include "tinalux/ui/Container.h"

#include <algorithm>

#include "tinalux/ui/Layout.h"

namespace tinalux::ui {

Container::~Container() = default;

void Container::addChild(std::shared_ptr<Widget> child)
{
    if (child == nullptr) {
        return;
    }

    child->setParent(this);
    children_.push_back(std::move(child));
    markDirty();
}

void Container::removeChild(Widget* child)
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
        (*it)->setParent(nullptr);
        children_.erase(it);
        markDirty();
    }
}

void Container::setLayout(std::unique_ptr<Layout> layout)
{
    layout_ = std::move(layout);
    markDirty();
}

SkSize Container::measure(const Constraints& constraints)
{
    if (layout_ != nullptr) {
        return layout_->measure(constraints, children_);
    }

    if (!children_.empty()) {
        return children_.front()->measure(constraints);
    }

    return constraints.constrain(SkSize::Make(0.0f, 0.0f));
}

void Container::arrange(const SkRect& bounds)
{
    Widget::arrange(bounds);

    // 子组件布局使用容器本地坐标，避免重复叠加父组件偏移。
    const SkRect contentBounds = SkRect::MakeWH(bounds.width(), bounds.height());
    if (layout_ != nullptr) {
        layout_->arrange(contentBounds, children_);
        return;
    }

    if (!children_.empty()) {
        children_.front()->arrange(contentBounds);
    }
}

void Container::onDraw(SkCanvas* canvas)
{
    drawChildren(canvas);
}

Widget* Container::hitTest(float x, float y)
{
    if (!visible_) {
        return nullptr;
    }

    const SkRect localBounds = SkRect::MakeWH(bounds_.width(), bounds_.height());
    if (!localBounds.contains(x, y)) {
        return nullptr;
    }

    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        Widget* child = it->get();
        const SkRect childBounds = child->bounds();
        Widget* target = child->hitTest(x - childBounds.x(), y - childBounds.y());
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

void Container::drawChildren(SkCanvas* canvas)
{
    for (const auto& child : children_) {
        child->draw(canvas);
    }
}

}  // namespace tinalux::ui
