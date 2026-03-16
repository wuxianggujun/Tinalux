#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/Container.h"
#include "tinalux/ui/FlexLayout.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool nearlyEqual(float lhs, float rhs)
{
    return std::abs(lhs - rhs) <= 0.01f;
}

class ProbeWidget final : public tinalux::ui::Widget {
public:
    explicit ProbeWidget(tinalux::core::Size preferred)
        : preferred_(preferred)
    {
    }

    tinalux::core::Size measure(const tinalux::ui::Constraints& constraints) override
    {
        return constraints.constrain(preferred_);
    }

    void onDraw(tinalux::rendering::Canvas&) override
    {
    }

private:
    tinalux::core::Size preferred_;
};

}  // namespace

int main()
{
    using namespace tinalux;

    {
        auto container = std::make_shared<ui::Container>();
        auto layout = std::make_unique<ui::FlexLayout>();
        layout->direction = ui::FlexDirection::Row;
        layout->spacing = 10.0f;
        layout->padding = 10.0f;
        layout->alignItems = ui::AlignItems::Stretch;

        auto first = std::make_shared<ProbeWidget>(core::Size::Make(50.0f, 20.0f));
        auto second = std::make_shared<ProbeWidget>(core::Size::Make(30.0f, 30.0f));
        container->addChild(first);
        container->addChild(second);
        layout->setFlex(second.get(), 1.0f, 1.0f);
        container->setLayout(std::move(layout));

        const core::Size measured = container->measure(ui::Constraints::loose(300.0f, 120.0f));
        expect(nearlyEqual(measured.width(), 110.0f), "row flex should report intrinsic width before growth");
        expect(nearlyEqual(measured.height(), 50.0f), "row flex should report intrinsic cross size");

        container->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 300.0f, 100.0f));
        expect(nearlyEqual(first->bounds().x(), 10.0f), "row first item should respect padding");
        expect(nearlyEqual(first->bounds().width(), 50.0f), "row first item should keep intrinsic width");
        expect(nearlyEqual(first->bounds().height(), 80.0f), "stretch should expand first item height");
        expect(nearlyEqual(second->bounds().x(), 70.0f), "row second item should follow first item and spacing");
        expect(nearlyEqual(second->bounds().width(), 220.0f), "grow item should consume remaining width");
        expect(nearlyEqual(second->bounds().height(), 80.0f), "stretch should expand second item height");
    }

    {
        auto container = std::make_shared<ui::Container>();
        auto layout = std::make_unique<ui::FlexLayout>();
        layout->direction = ui::FlexDirection::Column;
        layout->alignItems = ui::AlignItems::End;
        layout->padding = 8.0f;

        auto first = std::make_shared<ProbeWidget>(core::Size::Make(40.0f, 20.0f));
        auto second = std::make_shared<ProbeWidget>(core::Size::Make(30.0f, 30.0f));
        container->addChild(first);
        container->addChild(second);
        container->setLayout(std::move(layout));

        container->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 120.0f, 140.0f));
        expect(nearlyEqual(first->bounds().x(), 72.0f), "column align end should right-align first item");
        expect(nearlyEqual(first->bounds().y(), 8.0f), "column first item should start after top padding");
        expect(nearlyEqual(second->bounds().x(), 82.0f), "column align end should right-align second item");
        expect(nearlyEqual(second->bounds().y(), 28.0f), "column second item should follow the first item");
    }

    {
        auto container = std::make_shared<ui::Container>();
        auto layout = std::make_unique<ui::FlexLayout>();
        layout->direction = ui::FlexDirection::Row;
        layout->justifyContent = ui::JustifyContent::SpaceBetween;

        auto first = std::make_shared<ProbeWidget>(core::Size::Make(30.0f, 10.0f));
        auto second = std::make_shared<ProbeWidget>(core::Size::Make(20.0f, 10.0f));
        auto third = std::make_shared<ProbeWidget>(core::Size::Make(10.0f, 10.0f));
        container->addChild(first);
        container->addChild(second);
        container->addChild(third);
        container->setLayout(std::move(layout));

        container->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 200.0f, 40.0f));
        expect(nearlyEqual(first->bounds().x(), 0.0f), "space-between should pin first item to start");
        expect(nearlyEqual(second->bounds().x(), 100.0f), "space-between should distribute middle item");
        expect(nearlyEqual(third->bounds().x(), 190.0f), "space-between should pin last item to end");
    }

    {
        auto container = std::make_shared<ui::Container>();
        auto layout = std::make_unique<ui::FlexLayout>();
        layout->direction = ui::FlexDirection::RowReverse;
        layout->wrap = ui::FlexWrap::Wrap;
        layout->spacing = 10.0f;
        layout->padding = 10.0f;

        auto first = std::make_shared<ProbeWidget>(core::Size::Make(40.0f, 20.0f));
        auto second = std::make_shared<ProbeWidget>(core::Size::Make(40.0f, 20.0f));
        auto third = std::make_shared<ProbeWidget>(core::Size::Make(40.0f, 20.0f));
        container->addChild(first);
        container->addChild(second);
        container->addChild(third);
        container->setLayout(std::move(layout));

        const core::Size measured = container->measure(ui::Constraints::loose(140.0f, 200.0f));
        expect(nearlyEqual(measured.width(), 110.0f), "wrapped reverse row should report widest line width");
        expect(nearlyEqual(measured.height(), 70.0f), "wrapped reverse row should report accumulated line height");

        container->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 140.0f, 120.0f));
        expect(nearlyEqual(third->bounds().x(), 90.0f), "row-reverse should place the visual first item on the right");
        expect(nearlyEqual(second->bounds().x(), 40.0f), "row-reverse should place the second item to the left");
        expect(nearlyEqual(first->bounds().x(), 90.0f), "wrapped item should start a new line on the right edge");
        expect(nearlyEqual(first->bounds().y(), 40.0f), "wrapped item should move to the next line");
    }

    return 0;
}
