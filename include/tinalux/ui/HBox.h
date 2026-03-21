#pragma once

#include "tinalux/ui/Container.h"

namespace tinalux::ui {

class HBoxLayout;

class HBox : public Container {
public:
    HBox();

    void setSpacing(float spacing);
    float spacing() const;

    void setPadding(float padding);
    float padding() const;

private:
    HBoxLayout* layout_;
};

} // namespace tinalux::ui
