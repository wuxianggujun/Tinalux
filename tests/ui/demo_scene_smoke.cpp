#include <cstdlib>
#include <iostream>
#include <memory>

#include "../../src/ui/RuntimeState.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/Container.h"
#include "tinalux/ui/DemoScene.h"
#include "tinalux/ui/ListView.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/TextInput.h"
#include "tinalux/ui/Theme.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

std::size_t listItemCount(const std::shared_ptr<tinalux::ui::ListView>& list)
{
    expect(list != nullptr, "list view should exist");
    auto* content = dynamic_cast<tinalux::ui::Container*>(list->content());
    expect(content != nullptr, "list view content should be a container");
    return content->children().size();
}

}  // namespace

int main()
{
    using namespace tinalux;

    ui::RuntimeState runtime;
    runtime.theme = ui::Theme::dark();
    ui::ScopedRuntimeState bind(runtime);

    auto rootWidget = ui::createDemoScene(runtime.theme, runtime.animationScheduler);
    auto root = std::dynamic_pointer_cast<ui::Container>(rootWidget);
    expect(root != nullptr, "demo scene should return a container root");

    const auto& children = root->children();
    expect(children.size() >= 4, "demo scene should expose a workspace container at root level");

    auto workspace = std::dynamic_pointer_cast<ui::Container>(children.back());
    expect(workspace != nullptr, "demo scene should expose a responsive workspace container");
    expect(workspace->children().size() == 2, "workspace should contain account and activity columns");

    auto accountColumn = std::dynamic_pointer_cast<ui::Container>(workspace->children()[0]);
    auto activityCard = std::dynamic_pointer_cast<ui::Panel>(workspace->children()[1]);
    expect(accountColumn != nullptr, "workspace should include an account column");
    expect(activityCard != nullptr, "workspace should include an activity card");
    expect(activityCard->children().size() == 4, "activity card should include title, summary, search, and list");

    auto searchInput = std::dynamic_pointer_cast<ui::TextInput>(activityCard->children()[2]);
    auto activityList = std::dynamic_pointer_cast<ui::ListView>(activityCard->children()[3]);
    expect(searchInput != nullptr, "demo scene should include a search text input");
    expect(activityList != nullptr, "demo scene should include an activity list view");

    root->measure(ui::Constraints::tight(720.0f, 960.0f));
    root->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 720.0f, 960.0f));
    expect(
        workspace->children()[1]->bounds().y() > workspace->children()[0]->bounds().y(),
        "narrow demo scene should stack activity card below account column");

    root->measure(ui::Constraints::tight(1280.0f, 960.0f));
    root->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 1280.0f, 960.0f));
    expect(
        workspace->children()[1]->bounds().x() > workspace->children()[0]->bounds().x(),
        "wide demo scene should place activity card beside account column");

    expect(listItemCount(activityList) == 12, "demo scene should start with all activity sessions visible");

    searchInput->setText("android");
    expect(listItemCount(activityList) == 2, "search input should filter activity list entries");

    searchInput->setText("no-such-device");
    expect(listItemCount(activityList) == 1, "no-match search should show one empty-state card");

    searchInput->setText("");
    expect(listItemCount(activityList) == 12, "clearing search text should restore full activity list");

    return 0;
}
