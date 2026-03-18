#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

#include "../../src/ui/RuntimeState.h"
#include "tinalux/ui/Constraints.h"
#include "tinalux/ui/Container.h"
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
    ui::ScopedRuntimeState bind(runtime);

    ui::ListView listView;
    listView.setPreferredHeight(96.0f);
    listView.setPadding(8.0f);
    listView.setSpacing(4.0f);

    std::vector<std::shared_ptr<FixedItem>> items;
    for (int index = 0; index < 120; ++index) {
        auto item = std::make_shared<FixedItem>(180.0f, 20.0f);
        items.push_back(item);
        listView.addItem(item);
    }

    listView.measure(ui::Constraints::tight(240.0f, 96.0f));
    listView.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 240.0f, 96.0f));

    auto* content = dynamic_cast<ui::Container*>(listView.content());
    expect(content != nullptr, "list view content should remain container-based for virtualization");
    expect(content->children().size() < items.size(), "virtualized list should not attach every item");
    expect(!content->children().empty(), "virtualized list should attach visible items");
    expect(items.front()->parent() == content, "first visible item should stay attached before scrolling");
    expect(items.back()->parent() == nullptr, "far offscreen item should start detached");

    listView.setSelectedIndex(static_cast<int>(items.size()) - 1);

    expect(listView.scrollOffset() > 0.0f, "selecting a far item should scroll it into view");
    expect(content->children().size() < items.size(), "virtualized list should stay sparse after scrolling");
    expect(items.back()->parent() == content, "selected far item should become attached when brought into view");
    expect(items.front()->parent() == nullptr, "top item should detach after scrolling far away");

    return 0;
}
