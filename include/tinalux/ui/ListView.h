#pragma once

#include <memory>

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

private:
    std::shared_ptr<Container> items_;
    VBoxLayout* layout_ = nullptr;
};

}  // namespace tinalux::ui
