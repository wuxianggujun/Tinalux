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
        return constraints.constrain(tinalux::core::Size::Make(80.0f, 60.0f));
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

class OverlapRoot final : public tinalux::ui::Container {
public:
    tinalux::core::Size measure(const tinalux::ui::Constraints& constraints) override
    {
        return constraints.constrain(tinalux::core::Size::Make(200.0f, 160.0f));
    }

    void arrange(const tinalux::core::Rect& bounds) override
    {
        Widget::arrange(bounds);
        if (children_.size() > 0) {
            children_[0]->arrange(tinalux::core::Rect::MakeXYWH(20.0f, 20.0f, 80.0f, 60.0f));
        }
        if (children_.size() > 1) {
            children_[1]->arrange(tinalux::core::Rect::MakeXYWH(60.0f, 40.0f, 80.0f, 60.0f));
        }
        if (children_.size() > 2) {
            children_[2]->arrange(tinalux::core::Rect::MakeXYWH(140.0f, 20.0f, 40.0f, 40.0f));
        }
    }
};

}  // namespace

int main()
{
    using namespace tinalux;

    app::UIContext context;
    auto root = std::make_shared<OverlapRoot>();
    auto dirtyLeaf = std::make_shared<CountingLeaf>(core::colorRGB(255, 120, 80));
    auto overlappingLeaf = std::make_shared<CountingLeaf>(core::colorRGB(80, 180, 255));
    auto isolatedLeaf = std::make_shared<CountingLeaf>(core::colorRGB(120, 220, 120));
    root->addChild(dirtyLeaf);
    root->addChild(overlappingLeaf);
    root->addChild(isolatedLeaf);
    context.setRootWidget(root);

    const rendering::RenderSurface surface = rendering::createRasterSurface(200, 160);
    expect(static_cast<bool>(surface), "dirty subtree smoke should create raster surface");
    rendering::Canvas canvas = surface.canvas();
    expect(static_cast<bool>(canvas), "dirty subtree smoke should expose canvas");

    const bool firstFullRedraw = context.render(canvas, 200, 160, 1.0f);
    expect(firstFullRedraw, "initial render should redraw the full scene");
    expect(dirtyLeaf->drawCount() == 1, "initial render should draw the dirty probe once");
    expect(overlappingLeaf->drawCount() == 1, "initial render should draw overlapping sibling once");
    expect(isolatedLeaf->drawCount() == 1, "initial render should draw isolated sibling once");

    dirtyLeaf->setColor(core::colorRGB(255, 200, 80));
    const bool secondFullRedraw = context.render(canvas, 200, 160, 1.0f);
    expect(!secondFullRedraw, "paint-only child update should stay on partial redraw path");
    expect(dirtyLeaf->drawCount() == 2, "dirty child should redraw on partial render");
    expect(
        overlappingLeaf->drawCount() == 2,
        "clean overlapping sibling should redraw because partial clear erased the overlap region");
    expect(
        isolatedLeaf->drawCount() == 1,
        "clean sibling outside dirty region should stay pruned during partial redraw");

    return 0;
}
