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

    auto rowContainer = std::make_shared<ui::Container>();
    auto rowLayout = std::make_unique<ui::FlexLayout>();
    rowLayout->direction = ui::FlexDirection::Row;
    rowLayout->spacing = 10.0f;
    rowLayout->padding = 10.0f;
    rowLayout->alignItems = ui::AlignItems::Stretch;

    auto first = std::make_shared<ProbeWidget>(core::Size::Make(50.0f, 20.0f));
    auto second = std::make_shared<ProbeWidget>(core::Size::Make(30.0f, 30.0f));
    rowContainer->addChild(first);
    rowContainer->addChild(second);
    rowLayout->setFlex(second.get(), 1.0f, 1.0f);
    rowContainer->setLayout(std::move(rowLayout));

    const core::Size rowMeasured = rowContainer->measure(ui::Constraints::loose(300.0f, 120.0f));
    expect(nearlyEqual(rowMeasured.width(), 110.0f), "flex row should report natural width before growth");
    expect(nearlyEqual(rowMeasured.height(), 50.0f), "flex row should report natural cross size");

    rowContainer->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 300.0f, 100.0f));
    expect(nearlyEqual(first->bounds().x(), 10.0f), "first row item should start after padding");
    expect(nearlyEqual(first->bounds().width(), 50.0f), "first row item should keep intrinsic width");
    expect(nearlyEqual(first->bounds().height(), 80.0f), "stretch alignment should expand first item height");
    expect(nearlyEqual(second->bounds().x(), 70.0f), "second row item should follow spacing after first");
    expect(nearlyEqual(second->bounds().width(), 220.0f), "grow item should consume remaining row space");
    expect(nearlyEqual(second->bounds().height(), 80.0f), "stretch alignment should expand second item height");

    auto centerContainer = std::make_shared<ui::Container>();
    auto centerLayout = std::make_unique<ui::FlexLayout>();
    centerLayout->direction = ui::FlexDirection::Row;
    centerLayout->justifyContent = ui::JustifyContent::Center;
    centerLayout->alignItems = ui::AlignItems::Center;
    centerLayout->spacing = 10.0f;

    auto centerA = std::make_shared<ProbeWidget>(core::Size::Make(40.0f, 20.0f));
    auto centerB = std::make_shared<ProbeWidget>(core::Size::Make(60.0f, 30.0f));
    centerContainer->addChild(centerA);
    centerContainer->addChild(centerB);
    centerContainer->setLayout(std::move(centerLayout));
    centerContainer->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 200.0f, 100.0f));
    expect(nearlyEqual(centerA->bounds().x(), 45.0f), "center justification should offset first item");
    expect(nearlyEqual(centerA->bounds().y(), 40.0f), "center alignment should vertically center first item");
    expect(nearlyEqual(centerB->bounds().x(), 95.0f), "center justification should preserve gap");
    expect(nearlyEqual(centerB->bounds().y(), 35.0f), "center alignment should vertically center second item");

    auto columnContainer = std::make_shared<ui::Container>();
    auto columnLayout = std::make_unique<ui::FlexLayout>();
    columnLayout->direction = ui::FlexDirection::Column;
    columnLayout->justifyContent = ui::JustifyContent::SpaceBetween;
    columnLayout->alignItems = ui::AlignItems::End;
    columnLayout->padding = 8.0f;

    auto columnA = std::make_shared<ProbeWidget>(core::Size::Make(40.0f, 20.0f));
    auto columnB = std::make_shared<ProbeWidget>(core::Size::Make(30.0f, 30.0f));
    columnContainer->addChild(columnA);
    columnContainer->addChild(columnB);
    columnContainer->setLayout(std::move(columnLayout));
    columnContainer->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 120.0f, 140.0f));
    expect(nearlyEqual(columnA->bounds().y(), 8.0f), "column first item should start at top padding");
    expect(nearlyEqual(columnB->bounds().y(), 102.0f), "space-between should push last column item to bottom");
    expect(nearlyEqual(columnA->bounds().x(), 72.0f), "end alignment should right-align wider first item");
    expect(nearlyEqual(columnB->bounds().x(), 82.0f), "end alignment should right-align narrower second item");

    return 0;
}
