#include "tinalux/ui/VBox.h"

#include "tinalux/ui/Layout.h"

namespace tinalux::ui {

VBox::VBox()
{
    auto layout = std::make_unique<VBoxLayout>();
    layout_ = layout.get();
    setLayout(std::move(layout));
}

void VBox::setSpacing(float spacing)
{
    layout_->spacing = spacing;
    markLayoutDirty();
}

float VBox::spacing() const
{
    return layout_->spacing;
}

void VBox::setPadding(float padding)
{
    layout_->padding = padding;
    markLayoutDirty();
}

float VBox::padding() const
{
    return layout_->padding;
}

} // namespace tinalux::ui
