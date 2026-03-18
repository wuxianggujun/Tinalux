#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "tinalux/ui/ListViewStyle.h"
#include "tinalux/ui/ScrollView.h"

namespace tinalux::ui {

class Container;

class ListView final : public ScrollView {
public:
    ListView();

    void addItem(std::shared_ptr<Widget> item);
    void clearItems();
    void setSpacing(float spacing);
    void setPadding(float padding);
    void setStyle(const ListViewStyle& style);
    void clearStyle();
    const ListViewStyle* style() const;
    void setSelectedIndex(int index);
    int selectedIndex() const;
    Widget* selectedItem() const;
    void onSelectionChanged(std::function<void(int)> handler);

    bool focusable() const override;
    core::Size measure(const Constraints& constraints) override;
    void arrange(const core::Rect& bounds) override;
    void onDraw(rendering::Canvas& canvas) override;
    bool onEventCapture(core::Event& event) override;
    bool onEvent(core::Event& event) override;

private:
    ListViewStyle resolvedStyle() const;
    void applyResolvedStyle();
    void invalidateItemLayoutCache();
    void ensureItemLayoutCache(float viewportWidth);
    void syncVisibleItems();
    void applyVisibleItemLayout();
    std::size_t firstItemIntersecting(float contentY) const;
    std::size_t firstItemStartingAfter(float contentY) const;
    Widget* itemAtIndex(int index) const;
    int indexForPoint(core::Point localPoint) const;
    void ensureItemVisible(int index);
    void updateSelection(int index, bool emitCallback);

    std::shared_ptr<Container> items_;
    std::vector<std::shared_ptr<Widget>> itemStorage_;
    std::vector<core::Rect> itemBounds_;
    std::vector<std::uint64_t> itemLayoutVersions_;
    core::Size measuredContentSize_ = core::Size::Make(0.0f, 0.0f);
    float cachedViewportWidth_ = -1.0f;
    float appliedPadding_ = -1.0f;
    float appliedSpacing_ = -1.0f;
    bool itemLayoutCacheValid_ = false;
    std::optional<ListViewStyle> customStyle_;
    std::optional<float> spacingOverride_;
    std::optional<float> paddingOverride_;
    std::function<void(int)> onSelectionChanged_;
    int selectedIndex_ = -1;
    int pressedIndex_ = -1;
};

}  // namespace tinalux::ui
