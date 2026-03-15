#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/Layout.h"
#include "tinalux/ui/ScrollView.h"
#include "tinalux/ui/Widget.h"

namespace {

class FixedWidget final : public tinalux::ui::Widget {
public:
    FixedWidget(float width, float height)
        : preferredSize_(tinalux::core::Size::Make(width, height))
    {
    }

    tinalux::core::Size measure(const tinalux::ui::Constraints& constraints) override
    {
        return constraints.constrain(preferredSize_);
    }

    void onDraw(tinalux::rendering::Canvas&) override {}

private:
    tinalux::core::Size preferredSize_;
};

class CountingWidget final : public tinalux::ui::Widget {
public:
    CountingWidget(int* measureCount, float width, float height)
        : measureCount_(measureCount)
        , preferredSize_(tinalux::core::Size::Make(width, height))
    {
    }

    tinalux::core::Size measure(const tinalux::ui::Constraints& constraints) override
    {
        ++(*measureCount_);
        return constraints.constrain(preferredSize_);
    }

    bool focusable() const override
    {
        return true;
    }

    void onDraw(tinalux::rendering::Canvas&) override {}

private:
    int* measureCount_ = nullptr;
    tinalux::core::Size preferredSize_;
};

bool nearlyEqual(float lhs, float rhs)
{
    return std::abs(lhs - rhs) <= 0.001f;
}

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

}  // namespace

int main()
{
    using tinalux::ui::Constraints;
    using tinalux::ui::HBoxLayout;
    using tinalux::ui::VBoxLayout;
    using WidgetPtr = std::shared_ptr<tinalux::ui::Widget>;

    const Constraints tight = Constraints::tight(120.0f, 80.0f);
    const tinalux::core::Size constrained = tight.constrain(tinalux::core::Size::Make(16.0f, 200.0f));
    expect(tight.isTight(), "tight constraints should report tight");
    expect(nearlyEqual(constrained.width(), 120.0f), "tight width mismatch");
    expect(nearlyEqual(constrained.height(), 80.0f), "tight height mismatch");

    std::vector<WidgetPtr> verticalChildren {
        std::make_shared<FixedWidget>(40.0f, 20.0f),
        std::make_shared<FixedWidget>(50.0f, 30.0f),
    };

    VBoxLayout vbox;
    vbox.padding = 8.0f;
    vbox.spacing = 10.0f;

    const tinalux::core::Size vboxSize = vbox.measure(Constraints::loose(200.0f, 200.0f), verticalChildren);
    expect(nearlyEqual(vboxSize.width(), 66.0f), "VBox measured width mismatch");
    expect(nearlyEqual(vboxSize.height(), 76.0f), "VBox measured height mismatch");

    vbox.arrange(tinalux::core::Rect::MakeWH(120.0f, 100.0f), verticalChildren);
    expect(nearlyEqual(verticalChildren[0]->bounds().x(), 8.0f), "VBox child 0 x mismatch");
    expect(nearlyEqual(verticalChildren[0]->bounds().y(), 8.0f), "VBox child 0 y mismatch");
    expect(nearlyEqual(verticalChildren[1]->bounds().x(), 8.0f), "VBox child 1 x mismatch");
    expect(nearlyEqual(verticalChildren[1]->bounds().y(), 38.0f), "VBox child 1 y mismatch");

    std::vector<WidgetPtr> horizontalChildren {
        std::make_shared<FixedWidget>(15.0f, 10.0f),
        std::make_shared<FixedWidget>(25.0f, 14.0f),
        std::make_shared<FixedWidget>(18.0f, 12.0f),
    };

    HBoxLayout hbox;
    hbox.padding = 6.0f;
    hbox.spacing = 4.0f;

    const tinalux::core::Size hboxSize = hbox.measure(Constraints::loose(200.0f, 200.0f), horizontalChildren);
    expect(nearlyEqual(hboxSize.width(), 78.0f), "HBox measured width mismatch");
    expect(nearlyEqual(hboxSize.height(), 26.0f), "HBox measured height mismatch");

    hbox.arrange(tinalux::core::Rect::MakeWH(120.0f, 60.0f), horizontalChildren);
    expect(nearlyEqual(horizontalChildren[0]->bounds().x(), 6.0f), "HBox child 0 x mismatch");
    expect(nearlyEqual(horizontalChildren[1]->bounds().x(), 25.0f), "HBox child 1 x mismatch");
    expect(nearlyEqual(horizontalChildren[2]->bounds().x(), 54.0f), "HBox child 2 x mismatch");
    expect(nearlyEqual(horizontalChildren[2]->bounds().y(), 6.0f), "HBox child y mismatch");

    int cachedVBoxMeasureCount = 0;
    std::vector<WidgetPtr> cachedVerticalChildren {
        std::make_shared<CountingWidget>(&cachedVBoxMeasureCount, 40.0f, 20.0f),
        std::make_shared<CountingWidget>(&cachedVBoxMeasureCount, 50.0f, 30.0f),
    };

    VBoxLayout cachedVBox;
    cachedVBox.padding = 8.0f;
    cachedVBox.spacing = 10.0f;

    const Constraints cachedConstraints = Constraints::loose(200.0f, 200.0f);
    cachedVBox.measure(cachedConstraints, cachedVerticalChildren);
    expect(cachedVBoxMeasureCount == 2, "VBox first measure should visit children once");
    cachedVBox.measure(cachedConstraints, cachedVerticalChildren);
    expect(cachedVBoxMeasureCount == 2, "VBox repeated measure should reuse cached size");
    cachedVerticalChildren[0]->setFocused(true);
    cachedVBox.measure(cachedConstraints, cachedVerticalChildren);
    expect(cachedVBoxMeasureCount == 2, "paint-only dirty should not invalidate VBox layout cache");
    cachedVBox.arrange(tinalux::core::Rect::MakeWH(200.0f, 100.0f), cachedVerticalChildren);
    expect(cachedVBoxMeasureCount == 2, "VBox arrange should reuse measured child sizes");
    cachedVerticalChildren[0]->markLayoutDirty();
    cachedVBox.measure(cachedConstraints, cachedVerticalChildren);
    expect(cachedVBoxMeasureCount == 4, "layout dirty child should invalidate VBox cache");

    int scrollMeasureCount = 0;
    auto scrollContent = std::make_shared<CountingWidget>(&scrollMeasureCount, 180.0f, 320.0f);
    tinalux::ui::ScrollView scrollView;
    scrollView.setContent(scrollContent);
    scrollView.setPreferredHeight(96.0f);
    scrollView.measure(Constraints::loose(200.0f, 200.0f));
    expect(scrollMeasureCount == 1, "ScrollView measure should measure content once");
    scrollView.arrange(tinalux::core::Rect::MakeWH(200.0f, 96.0f));
    expect(scrollMeasureCount == 1, "ScrollView arrange should reuse cached content size");
    scrollContent->setFocused(true);
    scrollView.measure(Constraints::loose(200.0f, 200.0f));
    expect(scrollMeasureCount == 1, "paint-only dirty should not invalidate ScrollView cache");
    scrollContent->markLayoutDirty();
    scrollView.measure(Constraints::loose(200.0f, 200.0f));
    expect(scrollMeasureCount == 2, "layout dirty content should invalidate ScrollView cache");

    return 0;
}
