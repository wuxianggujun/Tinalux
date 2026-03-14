#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "include/core/SkColor.h"
#include "tinalux/app/Application.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/Layout.h"
#include "tinalux/ui/ListView.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/Theme.h"

namespace {

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
    using namespace tinalux;

    ui::setTheme(ui::Theme::dark());

    auto root = std::make_shared<ui::Panel>();
    auto rootLayout = std::make_unique<ui::VBoxLayout>();
    rootLayout->padding = 12.0f;
    rootLayout->spacing = 8.0f;
    root->setLayout(std::move(rootLayout));

    auto listView = std::make_shared<ui::ListView>();
    listView->setPreferredHeight(96.0f);
    listView->setPadding(8.0f);
    listView->setSpacing(6.0f);

    int clickedIndex = -1;
    std::vector<std::shared_ptr<ui::Button>> buttons;
    for (int index = 0; index < 8; ++index) {
        auto button = std::make_shared<ui::Button>("Item " + std::to_string(index + 1));
        button->onClick([&clickedIndex, index] { clickedIndex = index; });
        buttons.push_back(button);
        listView->addItem(button);
    }

    root->addChild(listView);
    root->measure(ui::Constraints::tight(260.0f, 180.0f));
    root->arrange(SkRect::MakeXYWH(0.0f, 0.0f, 260.0f, 180.0f));

    app::Application app;
    app.setRootWidget(root);

    const SkRect listBounds = listView->globalBounds();
    core::MouseMoveEvent hoverList(listBounds.x() + 24.0, listBounds.y() + 24.0);
    app.handleEvent(hoverList);

    core::MouseScrollEvent scrollDown(0.0, -2.0);
    app.handleEvent(scrollDown);
    expect(listView->scrollOffset() > 0.0f, "scroll wheel should advance list offset");
    expect(listView->maxScrollOffset() > 0.0f, "list should have scrollable overflow");

    std::shared_ptr<ui::Button> targetButton;
    SkRect targetBounds = SkRect::MakeEmpty();
    for (std::size_t index = 0; index < buttons.size(); ++index) {
        const SkRect bounds = buttons[index]->globalBounds();
        if (bounds.centerY() > listBounds.y() && bounds.centerY() < listBounds.bottom()) {
            targetButton = buttons[index];
            targetBounds = bounds;
            break;
        }
    }
    expect(targetButton != nullptr, "scroll should expose at least one visible item");

    core::MouseMoveEvent hoverTarget(targetBounds.centerX(), targetBounds.centerY());
    app.handleEvent(hoverTarget);
    core::MouseButtonEvent press(
        0,
        0,
        targetBounds.centerX(),
        targetBounds.centerY(),
        core::EventType::MouseButtonPress);
    core::MouseButtonEvent release(
        0,
        0,
        targetBounds.centerX(),
        targetBounds.centerY(),
        core::EventType::MouseButtonRelease);
    app.handleEvent(press);
    app.handleEvent(release);

    expect(clickedIndex >= 0, "button inside scrolled list should still receive click");

    return 0;
}
