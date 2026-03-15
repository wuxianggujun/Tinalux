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
    root->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 260.0f, 180.0f));

    app::Application app;
    app.setRootWidget(root);

    const core::Rect listBounds = listView->globalBounds();
    const core::Rect firstButtonBoundsBeforeScroll = buttons.front()->globalBounds();
    const core::Point firstButtonCenterBeforeScroll = buttons.front()->localToGlobal(core::Point::Make(
        buttons.front()->bounds().width() * 0.5f,
        buttons.front()->bounds().height() * 0.5f));
    core::MouseMoveEvent hoverList(listBounds.x() + 24.0, listBounds.y() + 24.0);
    app.handleEvent(hoverList);

    core::MouseScrollEvent scrollDown(0.0, -2.0);
    app.handleEvent(scrollDown);
    expect(listView->scrollOffset() > 0.0f, "scroll wheel should advance list offset");
    expect(listView->maxScrollOffset() > 0.0f, "list should have scrollable overflow");
    const core::Rect firstButtonBoundsAfterScroll = buttons.front()->globalBounds();
    expect(
        std::abs(
            firstButtonBoundsAfterScroll.y()
            - (firstButtonBoundsBeforeScroll.y() - listView->scrollOffset())) <= 0.001f,
        "scroll offset should participate in child global bounds");
    const core::Point firstButtonCenterAfterScroll = buttons.front()->localToGlobal(core::Point::Make(
        buttons.front()->bounds().width() * 0.5f,
        buttons.front()->bounds().height() * 0.5f));
    expect(
        std::abs(
            firstButtonCenterAfterScroll.y()
            - (firstButtonCenterBeforeScroll.y() - listView->scrollOffset())) <= 0.001f,
        "localToGlobal should honor scroll offset");
    const core::Point buttonLocalCenter = buttons.front()->globalToLocal(firstButtonCenterAfterScroll);
    expect(
        std::abs(buttonLocalCenter.x() - buttons.front()->bounds().width() * 0.5f) <= 0.001f
            && std::abs(buttonLocalCenter.y() - buttons.front()->bounds().height() * 0.5f) <= 0.001f,
        "globalToLocal should invert localToGlobal");

    std::shared_ptr<ui::Button> targetButton;
    core::Rect targetBounds = core::Rect::MakeEmpty();
    for (std::size_t index = 0; index < buttons.size(); ++index) {
        const core::Rect bounds = buttons[index]->globalBounds();
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
