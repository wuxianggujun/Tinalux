#pragma once

#include "tinalux/ui/Container.h"

namespace tinalux::ui {

class VBoxLayout;

class VBox : public Container {
public:
    VBox();

    void setSpacing(float spacing);
    float spacing() const;

    void setPadding(float padding);
    float padding() const;

private:
    VBoxLayout* layout_;
};

} // namespace tinalux::ui
