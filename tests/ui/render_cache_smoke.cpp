#include <cstdlib>
#include <iostream>
#include <memory>

#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Panel.h"

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
    tinalux::core::Size measure(const tinalux::ui::Constraints& constraints) override
    {
        return constraints.constrain(tinalux::core::Size::Make(64.0f, 32.0f));
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
    tinalux::core::Color color_ = tinalux::core::colorRGB(255, 120, 80);
};

}  // namespace

int main()
{
    using namespace tinalux;

    const rendering::RenderSurface surface = rendering::createRasterSurface(256, 128);
    expect(static_cast<bool>(surface), "render cache test should create raster surface");
    rendering::Canvas canvas = surface.canvas();
    expect(static_cast<bool>(canvas), "render cache test should expose canvas");

    {
        auto panel = std::make_shared<ui::Panel>();
        panel->setRenderCacheEnabled(true);
        auto leaf = std::make_shared<CountingLeaf>();
        panel->addChild(leaf);
        panel->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 180.0f, 80.0f));
        leaf->arrange(core::Rect::MakeXYWH(16.0f, 16.0f, 64.0f, 32.0f));

        panel->draw(canvas);
        expect(leaf->drawCount() == 1, "first cached draw should render child into cache");

        panel->draw(canvas);
        expect(leaf->drawCount() == 1, "second cached draw should reuse cache without redrawing child");

        leaf->setColor(core::colorRGB(80, 180, 255));
        panel->draw(canvas);
        expect(leaf->drawCount() == 2, "child dirty should invalidate cache and redraw subtree");
    }

    {
        auto panel = std::make_shared<ui::Panel>();
        panel->setRenderCacheEnabled(false);
        auto leaf = std::make_shared<CountingLeaf>();
        panel->addChild(leaf);
        panel->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 180.0f, 80.0f));
        leaf->arrange(core::Rect::MakeXYWH(16.0f, 16.0f, 64.0f, 32.0f));

        panel->draw(canvas);
        panel->draw(canvas);
        expect(leaf->drawCount() == 2, "disabled render cache should redraw child every frame");
    }

    return 0;
}
