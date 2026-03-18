#include <cstdlib>
#include <iostream>
#include <memory>
#include <set>
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

class VariableItem final : public tinalux::ui::Widget {
public:
    explicit VariableItem(float width)
        : width_(width)
    {
    }

    void bindHeight(float height)
    {
        if (height_ == height) {
            return;
        }

        height_ = height;
        markLayoutDirty();
    }

    tinalux::core::Size measure(const tinalux::ui::Constraints& constraints) override
    {
        return constraints.constrain(tinalux::core::Size::Make(width_, height_));
    }

    void onDraw(tinalux::rendering::Canvas&) override {}

private:
    float width_ = 0.0f;
    float height_ = 24.0f;
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

    auto items = std::make_shared<std::vector<std::shared_ptr<FixedItem>>>();
    for (int index = 0; index < 120; ++index) {
        auto item = std::make_shared<FixedItem>(180.0f, 20.0f);
        items->push_back(item);
    }
    listView.setItemFactory(
        items->size(),
        [items](std::size_t index, std::shared_ptr<ui::Widget>) -> std::shared_ptr<ui::Widget> {
            return (*items)[index];
        });

    listView.measure(ui::Constraints::tight(240.0f, 96.0f));
    listView.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 240.0f, 96.0f));

    auto* content = dynamic_cast<ui::Container*>(listView.content());
    expect(content != nullptr, "list view content should remain container-based for virtualization");
    expect(content->children().size() < items->size(), "virtualized list should not attach every item");
    expect(!content->children().empty(), "virtualized list should attach visible items");
    expect(items->front()->parent() == content, "first visible item should stay attached before scrolling");
    expect(items->back()->parent() == nullptr, "far offscreen item should start detached");

    listView.setSelectedIndex(static_cast<int>(items->size()) - 1);

    expect(listView.scrollOffset() > 0.0f, "selecting a far item should scroll it into view");
    expect(content->children().size() < items->size(), "virtualized list should stay sparse after scrolling");
    expect(items->back()->parent() == content, "selected far item should become attached when brought into view");
    expect(items->front()->parent() == nullptr, "top item should detach after scrolling far away");

    {
        ui::ListView dataDrivenListView;
        dataDrivenListView.setPreferredHeight(96.0f);
        dataDrivenListView.setPadding(8.0f);
        dataDrivenListView.setSpacing(4.0f);

        std::set<std::size_t> builtIndices;
        int createdWidgetCount = 0;
        dataDrivenListView.setItemFactory(
            200,
            24.0f,
            [&builtIndices, &createdWidgetCount](std::size_t index, std::shared_ptr<ui::Widget> recycled)
                -> std::shared_ptr<ui::Widget> {
                builtIndices.insert(index);
                if (recycled != nullptr) {
                    return recycled;
                }

                ++createdWidgetCount;
                return std::make_shared<FixedItem>(180.0f, 24.0f);
            });

        dataDrivenListView.measure(ui::Constraints::tight(240.0f, 96.0f));
        dataDrivenListView.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 240.0f, 96.0f));

        auto* dataContent = dynamic_cast<ui::Container*>(dataDrivenListView.content());
        expect(dataContent != nullptr, "data-driven list view should keep container content");
        expect(dataContent->children().size() < 20, "data-driven list should mount only a small visible window");
        expect(builtIndices.size() < 20, "data-driven list should not build every item during first layout");
        expect(createdWidgetCount < 20, "data-driven list should create only the visible recycler window");

        dataDrivenListView.setSelectedIndex(199);
        expect(dataDrivenListView.scrollOffset() > 0.0f, "data-driven list should scroll far selection into view");
        expect(dataDrivenListView.selectedItem() != nullptr, "data-driven list should materialize selected item on demand");
        expect(dataContent->children().size() < 20, "data-driven list should stay sparse after far selection");
        expect(builtIndices.size() < 40, "data-driven list should still build only a narrow window after far selection");
        expect(createdWidgetCount < 20, "data-driven list should recycle slots instead of creating a second window");
    }

    {
        ui::ListView variableHeightListView;
        variableHeightListView.setPreferredHeight(96.0f);
        variableHeightListView.setPadding(8.0f);
        variableHeightListView.setSpacing(4.0f);

        auto heights = std::make_shared<std::vector<float>>();
        heights->reserve(180);
        for (int index = 0; index < 180; ++index) {
            heights->push_back(20.0f + static_cast<float>((index % 4) * 8));
        }

        std::set<std::size_t> builtIndices;
        int createdWidgetCount = 0;
        variableHeightListView.setItemFactory(
            heights->size(),
            [heights, &builtIndices, &createdWidgetCount](std::size_t index, std::shared_ptr<ui::Widget> recycled)
                -> std::shared_ptr<ui::Widget> {
                builtIndices.insert(index);

                auto item = std::dynamic_pointer_cast<VariableItem>(recycled);
                if (item == nullptr) {
                    ++createdWidgetCount;
                    item = std::make_shared<VariableItem>(180.0f);
                }

                item->bindHeight((*heights)[index]);
                return item;
            });

        variableHeightListView.measure(ui::Constraints::tight(240.0f, 96.0f));
        variableHeightListView.arrange(core::Rect::MakeXYWH(0.0f, 0.0f, 240.0f, 96.0f));

        auto* variableContent = dynamic_cast<ui::Container*>(variableHeightListView.content());
        expect(variableContent != nullptr, "variable-height list should keep container content");
        expect(variableContent->children().size() < 20, "variable-height list should mount only the visible window");
        expect(builtIndices.size() < 20, "variable-height list should not build every row during first layout");
        expect(createdWidgetCount < 20, "variable-height list should create only a narrow recycler window");

        variableHeightListView.setSelectedIndex(static_cast<int>(heights->size()) - 1);
        expect(variableHeightListView.scrollOffset() > 0.0f, "variable-height list should scroll far selection into view");
        expect(variableHeightListView.selectedItem() != nullptr, "variable-height list should realize the far selection");
        expect(variableContent->children().size() < 20, "variable-height list should stay sparse after far selection");
        expect(builtIndices.size() < 50, "variable-height list should discover only a small measured window after far selection");
        expect(createdWidgetCount < 20, "variable-height list should reuse recycler slots after far selection");
    }

    return 0;
}
