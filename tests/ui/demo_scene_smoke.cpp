#include <cstdlib>
#include <iostream>
#include <memory>

#include "../../src/ui/RuntimeState.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/Button.h"
#include "tinalux/ui/Container.h"
#include "tinalux/ui/DemoScene.h"
#include "tinalux/ui/Label.h"
#include "tinalux/ui/Panel.h"
#include "tinalux/ui/ParagraphLabel.h"
#include "tinalux/ui/Theme.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

std::size_t countButtons(const std::shared_ptr<tinalux::ui::Container>& container)
{
    expect(container != nullptr, "container should exist");
    std::size_t count = 0;
    for (const auto& child : container->children()) {
        if (std::dynamic_pointer_cast<tinalux::ui::Button>(child) != nullptr) {
            ++count;
        }
    }
    return count;
}

std::size_t countLabels(const std::shared_ptr<tinalux::ui::Container>& container)
{
    expect(container != nullptr, "container should exist");
    std::size_t count = 0;
    for (const auto& child : container->children()) {
        if (std::dynamic_pointer_cast<tinalux::ui::Label>(child) != nullptr) {
            ++count;
        }
    }
    return count;
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
    expect(children.size() >= 4, "demo scene should keep a compact root with a showcase shell");

    auto shell = std::dynamic_pointer_cast<ui::Container>(children.back());
    expect(shell != nullptr, "demo scene should expose a showcase shell container");
    expect(shell->children().size() == 2, "showcase shell should contain navigation and content panels");

    auto navPanel = std::dynamic_pointer_cast<ui::Panel>(shell->children()[0]);
    auto contentPanel = std::dynamic_pointer_cast<ui::Panel>(shell->children()[1]);
    expect(navPanel != nullptr, "showcase shell should include a navigation panel");
    expect(contentPanel != nullptr, "showcase shell should include a content panel");
    expect(navPanel->children().size() == 4, "navigation panel should include title, hint, buttons, and footnote");
    expect(contentPanel->children().size() == 4, "content panel should include eyebrow, title, summary, and page host");

    auto navButtons = std::dynamic_pointer_cast<ui::Container>(navPanel->children()[2]);
    auto pageSummary = std::dynamic_pointer_cast<ui::ParagraphLabel>(contentPanel->children()[2]);
    auto pageHost = std::dynamic_pointer_cast<ui::Container>(contentPanel->children()[3]);
    expect(navButtons != nullptr, "navigation panel should expose a button column");
    expect(pageSummary != nullptr, "content panel should expose a page summary");
    expect(pageHost != nullptr, "content panel should expose a page host container");
    expect(countButtons(navButtons) == 9, "showcase navigation should expose nine page buttons");
    expect(countLabels(navButtons) >= 7, "showcase navigation should expose category labels");
    expect(pageHost->children().size() == 1, "page host should attach exactly one active showcase page");
    expect(
        countButtons(navButtons) > 0,
        "showcase navigation should build button-based routing");
    expect(
        std::dynamic_pointer_cast<ui::Container>(pageHost->children()[0]) != nullptr,
        "active showcase page should be a container-based page root");

    root->measure(ui::Constraints::tight(720.0f, 960.0f));
    root->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 720.0f, 960.0f));
    expect(
        shell->children()[1]->bounds().y() > shell->children()[0]->bounds().y(),
        "narrow demo scene should stack content below navigation");

    root->measure(ui::Constraints::tight(1280.0f, 960.0f));
    root->arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 1280.0f, 960.0f));
    expect(
        shell->children()[1]->bounds().x() > shell->children()[0]->bounds().x(),
        "wide demo scene should place content beside navigation");

    return 0;
}
