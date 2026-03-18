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

private:
    int drawCount_ = 0;
    tinalux::core::Color color_;
};

class IslandsRoot final : public tinalux::ui::Container {
public:
    tinalux::core::Size measure(const tinalux::ui::Constraints& constraints) override
    {
        return constraints.constrain(tinalux::core::Size::Make(240.0f, 100.0f));
    }

    void arrange(const tinalux::core::Rect& bounds) override
    {
        Widget::arrange(bounds);
        if (children_.size() > 0) {
            children_[0]->arrange(tinalux::core::Rect::MakeXYWH(10.0f, 20.0f, 40.0f, 40.0f));
        }
        if (children_.size() > 1) {
            children_[1]->arrange(tinalux::core::Rect::MakeXYWH(100.0f, 20.0f, 40.0f, 40.0f));
        }
        if (children_.size() > 2) {
            children_[2]->arrange(tinalux::core::Rect::MakeXYWH(190.0f, 20.0f, 40.0f, 40.0f));
        }
    }
};

}  // namespace

int main()
{
    using namespace tinalux;

    app::UIContext context;
    auto root = std::make_shared<IslandsRoot>();
    auto leftLeaf = std::make_shared<CountingLeaf>(core::colorRGB(255, 120, 80));
    auto centerLeaf = std::make_shared<CountingLeaf>(core::colorRGB(80, 180, 255));
    auto rightLeaf = std::make_shared<CountingLeaf>(core::colorRGB(120, 220, 120));
    root->addChild(leftLeaf);
    root->addChild(centerLeaf);
    root->addChild(rightLeaf);
    context.setRootWidget(root);

    const rendering::RenderSurface surface = rendering::createRasterSurface(240, 100);
    expect(static_cast<bool>(surface), "dirty subtree islands smoke should create raster surface");
    rendering::Canvas canvas = surface.canvas();
    expect(static_cast<bool>(canvas), "dirty subtree islands smoke should expose canvas");

    const bool firstFullRedraw = context.render(canvas, 240, 100, 1.0f);
    expect(firstFullRedraw, "initial render should redraw the full scene");
    expect(leftLeaf->drawCount() == 1, "initial render should draw left leaf once");
    expect(centerLeaf->drawCount() == 1, "initial render should draw center leaf once");
    expect(rightLeaf->drawCount() == 1, "initial render should draw right leaf once");

    leftLeaf->setColor(core::colorRGB(255, 200, 80));
    rightLeaf->setColor(core::colorRGB(120, 255, 150));
    const bool secondFullRedraw = context.render(canvas, 240, 100, 1.0f);
    expect(!secondFullRedraw, "disjoint paint updates should stay on partial redraw path");
    expect(leftLeaf->drawCount() == 2, "left dirty leaf should redraw on partial render");
    expect(centerLeaf->drawCount() == 1, "center clean leaf between dirty islands should stay pruned");
    expect(rightLeaf->drawCount() == 2, "right dirty leaf should redraw on partial render");

    return 0;
}
