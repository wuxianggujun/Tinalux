#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

#include "../../src/ui/RuntimeState.h"
#include "tinalux/app/Application.h"
#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/ListView.h"
#include "tinalux/ui/Panel.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void clickCenter(tinalux::app::Application& app, const tinalux::ui::Widget& widget)
{
    const tinalux::core::Rect bounds = widget.globalBounds();
    tinalux::core::MouseMoveEvent hover(bounds.centerX(), bounds.centerY());
    tinalux::core::MouseButtonEvent press(
        tinalux::core::mouse::kLeft,
        0,
        bounds.centerX(),
        bounds.centerY(),
        tinalux::core::EventType::MouseButtonPress);
    tinalux::core::MouseButtonEvent release(
        tinalux::core::mouse::kLeft,
        0,
        bounds.centerX(),
        bounds.centerY(),
        tinalux::core::EventType::MouseButtonRelease);
    app.handleEvent(hover);
    app.handleEvent(press);
    app.handleEvent(release);
}

void pressKey(tinalux::app::Application& app, int key)
{
    tinalux::core::KeyEvent keyEvent(key, 0, 0, tinalux::core::EventType::KeyPress);
    app.handleEvent(keyEvent);
}

}  // namespace

int main()
{
    using namespace tinalux;
    ui::RuntimeState runtime;
    ui::ScopedRuntimeState bind(runtime);

    auto root = std::make_shared<ui::Panel>();
    auto listView = std::make_shared<ui::ListView>();
    listView->setPreferredHeight(96.0f);
    listView->setPadding(8.0f);
    listView->setSpacing(6.0f);

    int clickedIndex = -1;
    std::vector<int> selectionChanges;
    auto items = std::make_shared<std::vector<std::shared_ptr<ui::Button>>>();
    for (int index = 0; index < 6; ++index) {
        auto button = std::make_shared<ui::Button>("Item " + std::to_string(index + 1));
        button->onClick([&clickedIndex, index] { clickedIndex = index; });
        items->push_back(button);
    }
    listView->setItemFactory(
        items->size(),
        [items](std::size_t index, std::shared_ptr<ui::Widget>) -> std::shared_ptr<ui::Widget> {
            return (*items)[index];
        });
    listView->onSelectionChanged([&selectionChanges](int index) { selectionChanges.push_back(index); });

    root->addChild(listView);
    root->measure(ui::Constraints::tight(320.0f, 180.0f));
    root->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 320.0f, 180.0f));

    app::Application app;
    app.setRootWidget(root);

    clickCenter(app, *(*items)[1]);
    expect(clickedIndex == 1, "button click inside list view should remain functional");
    expect(listView->selectedIndex() == 1, "mouse click should update list view selection");
    expect(listView->selectedItem() == (*items)[1].get(), "selectedItem should expose the selected widget");
    expect(selectionChanges.size() == 1 && selectionChanges.back() == 1, "mouse selection should emit callback");

    pressKey(app, core::keys::kDown);
    expect(listView->selectedIndex() == 2, "down key should advance selection even when child button has focus");

    pressKey(app, core::keys::kEnd);
    expect(listView->selectedIndex() == 5, "end key should jump to the last item");
    expect(listView->scrollOffset() > 0.0f, "keyboard selection should keep the selected item visible");

    pressKey(app, core::keys::kHome);
    expect(listView->selectedIndex() == 0, "home key should jump back to the first item");

    const std::size_t callbackCountBeforeRedundantSelect = selectionChanges.size();
    listView->setSelectedIndex(0);
    expect(
        selectionChanges.size() == callbackCountBeforeRedundantSelect,
        "setting the same selection should not emit duplicate callbacks");

    listView->clearSource();
    expect(listView->selectedIndex() == -1, "clearing items should clear the selection");
    expect(!selectionChanges.empty() && selectionChanges.back() == -1, "clearing items should emit cleared selection");

    return 0;
}
