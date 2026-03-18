#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

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

class GridRoot final : public tinalux::ui::Container {
public:
    tinalux::core::Size measure(const tinalux::ui::Constraints& constraints) override
    {
        return constraints.constrain(tinalux::core::Size::Make(240.0f, 180.0f));
    }

    void arrange(const tinalux::core::Rect& bounds) override
    {
        Widget::arrange(bounds);
        for (std::size_t index = 0; index < children_.size(); ++index) {
            const float x = 12.0f + static_cast<float>(index % 3) * 76.0f;
            const float y = 16.0f + static_cast<float>(index / 3) * 52.0f;
            children_[index]->arrange(tinalux::core::Rect::MakeXYWH(x, y, 40.0f, 40.0f));
        }
    }
};

}  // namespace

int main()
{
    using namespace tinalux;

    app::UIContext context;
    auto root = std::make_shared<GridRoot>();
    std::vector<std::shared_ptr<CountingLeaf>> leaves;
    leaves.reserve(9);
    for (int index = 0; index < 9; ++index) {
        auto leaf = std::make_shared<CountingLeaf>(core::colorRGB(80 + index * 10, 140, 220 - index * 10));
        leaves.push_back(leaf);
        root->addChild(leaf);
    }
    context.setRootWidget(root);

    const rendering::RenderSurface surface = rendering::createRasterSurface(240, 180);
    expect(static_cast<bool>(surface), "dirty subtree fallback smoke should create raster surface");
    rendering::Canvas canvas = surface.canvas();
    expect(static_cast<bool>(canvas), "dirty subtree fallback smoke should expose canvas");

    const bool initialFullRedraw = context.render(canvas, 240, 180, 1.0f);
    expect(initialFullRedraw, "initial render should redraw the full scene");
    for (const auto& leaf : leaves) {
        expect(leaf->drawCount() == 1, "initial render should draw every leaf once");
    }

    for (std::size_t index = 0; index < leaves.size(); ++index) {
        leaves[index]->setColor(core::colorRGB(220, 90 + static_cast<int>(index) * 8, 120));
    }

    const bool promotedFullRedraw = context.render(canvas, 240, 180, 1.0f);
    expect(
        promotedFullRedraw,
        "many disjoint dirty regions should promote redraw back to the full-scene path");
    for (const auto& leaf : leaves) {
        expect(leaf->drawCount() == 2, "promoted full redraw should redraw every leaf");
    }

    return 0;
}
