#pragma once

#include <memory>
#include <vector>

#include "tinalux/ui/Layout.h"
#include "tinalux/ui/Widget.h"

namespace tinalux::ui {

class Container : public Widget {
public:
    ~Container() override;

    void addChild(std::shared_ptr<Widget> child);
    void removeChild(Widget* child);
    void clearChildren();
    void setLayout(std::unique_ptr<Layout> layout);

    core::Size measure(const Constraints& constraints) override;
    void arrange(const core::Rect& bounds) override;
    void onDraw(rendering::Canvas& canvas) override;
    Widget* hitTest(float x, float y) override;

    const std::vector<std::shared_ptr<Widget>>& children() const;

protected:
    core::Rect localDrawBounds() const override;
    void drawChildren(rendering::Canvas& canvas);
    void replaceChildrenDirect(std::vector<std::shared_ptr<Widget>> children);

    std::vector<std::shared_ptr<Widget>> children_;
    std::unique_ptr<Layout> layout_;

private:
    void invalidateLocalDrawBoundsCache();
    void removeChildInternal(Widget* child, bool preserveFocusState);

    std::uint64_t localDrawBoundsRevision_ = 1;
    mutable std::uint64_t cachedLocalDrawBoundsLayoutVersion_ = 0;
    mutable std::uint64_t cachedLocalDrawBoundsRevision_ = 0;
    mutable core::Rect cachedLocalDrawBounds_ = core::Rect::MakeEmpty();
};

}  // namespace tinalux::ui
