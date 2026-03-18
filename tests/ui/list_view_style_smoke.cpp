#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "../../src/ui/RuntimeState.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/ListView.h"
#include "tinalux/ui/Theme.h"

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
    return std::abs(lhs - rhs) <= 0.001f;
}

class FixedItem final : public tinalux::ui::Widget {
public:
    FixedItem(float width, float height)
        : size_(tinalux::core::Size::Make(width, height))
    {
    }

    tinalux::core::Size measure(const tinalux::ui::Constraints& constraints) override
    {
        return constraints.constrain(size_);
    }

    void onDraw(tinalux::rendering::Canvas&) override {}

private:
    tinalux::core::Size size_;
};

}  // namespace

int main()
{
    using namespace tinalux;

    ui::RuntimeState runtime;
    runtime.theme = ui::Theme::light();
    runtime.theme.listViewStyle.padding = 14.0f;
    runtime.theme.listViewStyle.spacing = 9.0f;
    ui::ScopedRuntimeState bind(runtime);

    auto first = std::make_shared<FixedItem>(50.0f, 20.0f);
    auto second = std::make_shared<FixedItem>(60.0f, 24.0f);
    auto items = std::make_shared<std::vector<std::shared_ptr<FixedItem>>>();
    items->push_back(first);
    items->push_back(second);

    ui::ListView listView;
    listView.setPreferredHeight(120.0f);
    listView.setItemFactory(
        items->size(),
        [items](std::size_t index) -> std::shared_ptr<ui::Widget> {
            return (*items)[index];
        });

    listView.measure(ui::Constraints::loose(220.0f, 200.0f));
    listView.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 220.0f, 120.0f));
    expect(nearlyEqual(first->bounds().x(), 14.0f), "theme list view padding should affect first child x");
    expect(nearlyEqual(first->bounds().y(), 14.0f), "theme list view padding should affect first child y");
    expect(
        nearlyEqual(second->bounds().y(), 14.0f + first->bounds().height() + 9.0f),
        "theme list view spacing should affect second child y");

    ui::ListViewStyle customStyle = runtime.theme.listViewStyle;
    customStyle.padding = 20.0f;
    customStyle.spacing = 12.0f;
    listView.setStyle(customStyle);
    expect(listView.style() != nullptr, "setStyle should install per-list-view style override");
    listView.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 220.0f, 120.0f));
    expect(nearlyEqual(first->bounds().x(), 20.0f), "custom list view style should override padding");
    expect(
        nearlyEqual(second->bounds().y(), 20.0f + first->bounds().height() + 12.0f),
        "custom list view style should override spacing");

    listView.setPadding(26.0f);
    listView.setSpacing(16.0f);
    listView.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 220.0f, 120.0f));
    expect(nearlyEqual(first->bounds().x(), 26.0f), "list view setter padding should override installed style");
    expect(
        nearlyEqual(second->bounds().y(), 26.0f + first->bounds().height() + 16.0f),
        "list view setter spacing should override installed style");

    listView.clearStyle();
    expect(listView.style() == nullptr, "clearStyle should remove per-list-view style override");
    listView.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 220.0f, 120.0f));
    expect(nearlyEqual(first->bounds().x(), 26.0f), "setter padding should persist after clearing style");
    expect(
        nearlyEqual(second->bounds().y(), 26.0f + first->bounds().height() + 16.0f),
        "setter spacing should persist after clearing style");

    runtime.theme = ui::Theme::dark();
    runtime.theme.listViewStyle.padding = 18.0f;
    runtime.theme.listViewStyle.spacing = 11.0f;

    auto themedFirst = std::make_shared<FixedItem>(40.0f, 18.0f);
    auto themedSecond = std::make_shared<FixedItem>(42.0f, 18.0f);
    auto themedItems = std::make_shared<std::vector<std::shared_ptr<FixedItem>>>();
    themedItems->push_back(themedFirst);
    themedItems->push_back(themedSecond);
    ui::ListView themedListView;
    themedListView.setItemFactory(
        themedItems->size(),
        [themedItems](std::size_t index) -> std::shared_ptr<ui::Widget> {
            return (*themedItems)[index];
        });
    themedListView.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 220.0f, 120.0f));
    expect(nearlyEqual(themedFirst->bounds().x(), 18.0f), "list view should observe runtime theme padding");
    expect(
        nearlyEqual(themedSecond->bounds().y(), 18.0f + themedFirst->bounds().height() + 11.0f),
        "list view should observe runtime theme spacing");

    return 0;
}
