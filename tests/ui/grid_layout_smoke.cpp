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

    auto explicitContainer = std::make_shared<ui::Container>();
    auto explicitLayout = std::make_unique<ui::GridLayout>();
    explicitLayout->setColumns(2);
    explicitLayout->columnGap = 10.0f;
    explicitLayout->rowGap = 8.0f;
    explicitLayout->padding = 6.0f;

    auto header = std::make_shared<ProbeWidget>(core::Size::Make(40.0f, 20.0f));
    auto meta = std::make_shared<ProbeWidget>(core::Size::Make(30.0f, 30.0f));
    auto summary = std::make_shared<ProbeWidget>(core::Size::Make(70.0f, 25.0f));

    explicitContainer->addChild(header);
    explicitContainer->addChild(meta);
    explicitContainer->addChild(summary);
    explicitLayout->setPosition(header.get(), 0, 0);
    explicitLayout->setPosition(meta.get(), 1, 0);
    explicitLayout->setPosition(summary.get(), 0, 1);
    explicitLayout->setSpan(summary.get(), 2, 1);
    explicitContainer->setLayout(std::move(explicitLayout));

    const core::Size explicitMeasured =
        explicitContainer->measure(ui::Constraints::loose(200.0f, 200.0f));
    expect(nearlyEqual(explicitMeasured.width(), 92.0f), "grid should report expected natural width");
    expect(nearlyEqual(explicitMeasured.height(), 75.0f), "grid should report expected natural height");

    explicitContainer->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 120.0f, 100.0f));
    expect(nearlyEqual(header->bounds().x(), 6.0f), "explicit grid first item x mismatch");
    expect(nearlyEqual(header->bounds().y(), 6.0f), "explicit grid first item y mismatch");
    expect(nearlyEqual(header->bounds().width(), 54.0f), "explicit grid first item width mismatch");
    expect(nearlyEqual(header->bounds().height(), 42.5f), "explicit grid first item height mismatch");
    expect(nearlyEqual(meta->bounds().x(), 70.0f), "explicit grid second item x mismatch");
    expect(nearlyEqual(meta->bounds().width(), 44.0f), "explicit grid second item width mismatch");
    expect(nearlyEqual(summary->bounds().x(), 6.0f), "spanned grid item x mismatch");
    expect(nearlyEqual(summary->bounds().y(), 56.5f), "spanned grid item y mismatch");
    expect(nearlyEqual(summary->bounds().width(), 108.0f), "spanned grid item width mismatch");
    expect(nearlyEqual(summary->bounds().height(), 37.5f), "spanned grid item height mismatch");

    auto autoContainer = std::make_shared<ui::Container>();
    auto autoLayout = std::make_unique<ui::GridLayout>();
    autoLayout->setColumns(2);
    autoLayout->columnGap = 4.0f;
    autoLayout->rowGap = 4.0f;
    autoLayout->padding = 4.0f;

    auto first = std::make_shared<ProbeWidget>(core::Size::Make(20.0f, 10.0f));
    auto second = std::make_shared<ProbeWidget>(core::Size::Make(25.0f, 12.0f));
    auto third = std::make_shared<ProbeWidget>(core::Size::Make(30.0f, 14.0f));
    autoContainer->addChild(first);
    autoContainer->addChild(second);
    autoContainer->addChild(third);
    autoContainer->setLayout(std::move(autoLayout));

    const core::Size autoMeasured = autoContainer->measure(ui::Constraints::loose(200.0f, 200.0f));
    expect(nearlyEqual(autoMeasured.width(), 67.0f), "auto grid should report expected width");
    expect(nearlyEqual(autoMeasured.height(), 38.0f), "auto grid should report expected height");

    autoContainer->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 100.0f, 50.0f));
    expect(nearlyEqual(first->bounds().x(), 4.0f), "auto grid first item x mismatch");
    expect(nearlyEqual(second->bounds().x(), 54.5f), "auto grid second item x mismatch");
    expect(nearlyEqual(third->bounds().x(), 4.0f), "auto grid third item should wrap to next row");
    expect(nearlyEqual(third->bounds().y(), 26.0f), "auto grid third item y mismatch");
    expect(nearlyEqual(third->bounds().width(), 46.5f), "auto grid track width should expand to fill bounds");
    expect(nearlyEqual(third->bounds().height(), 20.0f), "auto grid second row height should expand to fill bounds");

    return 0;
}
