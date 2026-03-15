#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/Container.h"
#include "tinalux/ui/Layout.h"

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
    return std::abs(lhs - rhs) <= 0.05f;
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

    auto container = std::make_shared<ui::Container>();
    auto responsive = std::make_unique<ui::ResponsiveLayout>();

    auto narrow = std::make_unique<ui::VBoxLayout>();
    narrow->padding = 4.0f;
    narrow->spacing = 6.0f;

    auto medium = std::make_unique<ui::HBoxLayout>();
    medium->padding = 3.0f;
    medium->spacing = 5.0f;

    auto wide = std::make_unique<ui::GridLayout>();
    wide->setColumns(2);
    wide->padding = 2.0f;
    wide->columnGap = 8.0f;
    wide->rowGap = 4.0f;

    responsive->addBreakpoint(0.0f, std::move(narrow));
    responsive->addBreakpoint(80.0f, std::move(medium));
    responsive->addBreakpoint(160.0f, std::move(wide));

    auto first = std::make_shared<ProbeWidget>(core::Size::Make(40.0f, 20.0f));
    auto second = std::make_shared<ProbeWidget>(core::Size::Make(30.0f, 15.0f));
    auto third = std::make_shared<ProbeWidget>(core::Size::Make(20.0f, 10.0f));
    container->addChild(first);
    container->addChild(second);
    container->addChild(third);
    container->setLayout(std::move(responsive));

    const core::Size narrowMeasured = container->measure(ui::Constraints::loose(70.0f, 200.0f));
    expect(nearlyEqual(narrowMeasured.width(), 48.0f), "responsive layout should use vbox under narrow breakpoint");
    expect(nearlyEqual(narrowMeasured.height(), 65.0f), "responsive layout should measure with vbox stacking");

    container->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 70.0f, 120.0f));
    expect(nearlyEqual(first->bounds().x(), 4.0f), "narrow layout should use vbox padding");
    expect(nearlyEqual(second->bounds().y(), 30.0f), "narrow layout should stack vertically");

    const core::Size mediumMeasured = container->measure(ui::Constraints::loose(120.0f, 120.0f));
    expect(nearlyEqual(mediumMeasured.width(), 106.0f), "responsive layout should use hbox at medium breakpoint");
    expect(nearlyEqual(mediumMeasured.height(), 26.0f), "medium breakpoint should report hbox height");

    container->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 120.0f, 60.0f));
    expect(nearlyEqual(first->bounds().x(), 3.0f), "medium layout should use hbox padding");
    expect(nearlyEqual(second->bounds().x(), 48.0f), "medium layout should place items horizontally");
    expect(nearlyEqual(third->bounds().x(), 83.0f), "medium layout should keep horizontal flow");

    const core::Size wideMeasured = container->measure(ui::Constraints::loose(220.0f, 200.0f));
    expect(nearlyEqual(wideMeasured.width(), 82.0f), "wide breakpoint should switch to grid measurement");
    expect(nearlyEqual(wideMeasured.height(), 38.0f), "wide breakpoint should switch to grid height");

    container->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 220.0f, 100.0f));
    expect(nearlyEqual(first->bounds().x(), 2.0f), "wide layout should use grid padding");
    expect(nearlyEqual(second->bounds().x(), 119.0f), "wide layout should place second item in next column");
    expect(nearlyEqual(third->bounds().y(), 57.0f), "wide layout should wrap third item to next row");

    return 0;
}
