#pragma once

#include <memory>
#include <optional>

#include "tinalux/ui/ListViewStyle.h"
#include "tinalux/ui/ScrollView.h"

namespace tinalux::ui {

class Container;
class VBoxLayout;

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

    core::Size measure(const Constraints& constraints) override;
    void arrange(const core::Rect& bounds) override;

private:
    ListViewStyle resolvedStyle() const;
    void applyResolvedStyle();

    std::shared_ptr<Container> items_;
    VBoxLayout* layout_ = nullptr;
    std::optional<ListViewStyle> customStyle_;
    std::optional<float> spacingOverride_;
    std::optional<float> paddingOverride_;
};

}  // namespace tinalux::ui
