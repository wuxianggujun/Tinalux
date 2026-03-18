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
    void drawChildren(rendering::Canvas& canvas);

    std::vector<std::shared_ptr<Widget>> children_;
    std::unique_ptr<Layout> layout_;

private:
    void removeChildInternal(Widget* child, bool preserveFocusState);
};

}  // namespace tinalux::ui
