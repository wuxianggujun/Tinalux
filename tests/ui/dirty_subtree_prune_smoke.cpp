#include <cstdlib>
#include <iostream>
#include <memory>

#include "../../src/app/UIContext.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Container.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class CountingLeaf final : public tinalux::ui::Widget {
public:
    explicit CountingLeaf(tinalux::core::Color color)
        : color_(color)
    {
    }

    tinalux::core::Size measure(const tinalux::ui::Constraints& constraints) override
    {
        return constraints.constrain(tinalux::core::Size::Make(40.0f, 40.0f));
    }

    void onDraw(tinalux::rendering::Canvas& canvas) override
    {
        ++drawCount_;
        canvas.drawRect(
            tinalux::core::Rect::MakeWH(bounds_.width(), bounds_.height()),
            color_);
    }

    void setColor(tinalux::core::Color color)
    {
        if (color_ == color) {
            return;
        }

        color_ = color;
        markPaintDirty();
    }

    int drawCount() const
    {
        return drawCount_;
    }

    int drawBoundsQueryCount() const
    {
        return drawBoundsQueryCount_;
    }

protected:
    tinalux::core::Rect localDrawBounds() const override
    {
        ++drawBoundsQueryCount_;
        return Widget::localDrawBounds();
    }

private:
    int drawCount_ = 0;
    mutable int drawBoundsQueryCount_ = 0;
    tinalux::core::Color color_;
};

class CountingBranch final : public tinalux::ui::Container {
public:
    explicit CountingBranch(tinalux::core::Rect leafBounds)
        : leafBounds_(leafBounds)
    {
    }

    tinalux::core::Size measure(const tinalux::ui::Constraints& constraints) override
    {
        return constraints.constrain(tinalux::core::Size::Make(200.0f, 160.0f));
    }

    void arrange(const tinalux::core::Rect& bounds) override
    {
        Widget::arrange(bounds);
        if (!children_.empty()) {
            children_.front()->arrange(leafBounds_);
        }
    }

    void onDraw(tinalux::rendering::Canvas& canvas) override
    {
        ++drawCount_;
        drawChildren(canvas);
    }

    int drawCount() const
    {
        return drawCount_;
    }

private:
    int drawCount_ = 0;
    tinalux::core::Rect leafBounds_;
};

class RootContainer final : public tinalux::ui::Container {
public:
    tinalux::core::Size measure(const tinalux::ui::Constraints& constraints) override
    {
        return constraints.constrain(tinalux::core::Size::Make(200.0f, 160.0f));
    }

    void arrange(const tinalux::core::Rect& bounds) override
    {
        Widget::arrange(bounds);
        for (const auto& child : children_) {
            child->arrange(tinalux::core::Rect::MakeXYWH(0.0f, 0.0f, 200.0f, 160.0f));
        }
    }
};

}  // namespace

int main()
{
    using namespace tinalux;

    app::UIContext context;
    auto root = std::make_shared<RootContainer>();
    auto leftBranch = std::make_shared<CountingBranch>(core::Rect::MakeXYWH(16.0f, 16.0f, 40.0f, 40.0f));
    auto rightBranch = std::make_shared<CountingBranch>(core::Rect::MakeXYWH(144.0f, 16.0f, 40.0f, 40.0f));
    auto leftLeaf = std::make_shared<CountingLeaf>(core::colorRGB(255, 120, 80));
    auto rightLeaf = std::make_shared<CountingLeaf>(core::colorRGB(80, 180, 255));
    leftBranch->addChild(leftLeaf);
    rightBranch->addChild(rightLeaf);
    root->addChild(leftBranch);
    root->addChild(rightBranch);
    context.setRootWidget(root);

    const rendering::RenderSurface surface = rendering::createRasterSurface(200, 160);
    expect(static_cast<bool>(surface), "dirty subtree prune smoke should create raster surface");
    rendering::Canvas canvas = surface.canvas();
    expect(static_cast<bool>(canvas), "dirty subtree prune smoke should expose canvas");

    context.render(canvas, 200, 160, 1.0f);
    expect(leftBranch->drawCount() == 1, "initial render should draw left branch once");
    expect(rightBranch->drawCount() == 1, "initial render should draw right branch once");
    expect(leftLeaf->drawCount() == 1, "initial render should draw left leaf once");
    expect(rightLeaf->drawCount() == 1, "initial render should draw right leaf once");
    expect(leftLeaf->drawBoundsQueryCount() == 2, "initial render should query left leaf draw bounds twice");
    expect(rightLeaf->drawBoundsQueryCount() == 2, "initial render should query right leaf draw bounds twice");

    leftLeaf->setColor(core::colorRGB(255, 220, 80));
    const bool fullRedraw = context.render(canvas, 200, 160, 1.0f);
    expect(!fullRedraw, "leaf paint update should stay on partial redraw path");
    expect(
        leftBranch->drawCount() == 1,
        "partial redraw should descend through structural dirty branches without re-entering onDraw");
    expect(
        rightBranch->drawCount() == 1,
        "clean off-clip branch should still be pruned before entering onDraw");
    expect(leftLeaf->drawCount() == 2, "dirty leaf should redraw on partial render");
    expect(rightLeaf->drawCount() == 1, "pruned branch leaf should not redraw");
    expect(
        leftLeaf->drawBoundsQueryCount() == 3,
        "dirty branch should reuse cached subtree bounds before querying the visible leaf once more");
    expect(
        rightLeaf->drawBoundsQueryCount() == 2,
        "clean off-clip branch should reuse cached subtree bounds without re-querying its leaf");

    return 0;
}
