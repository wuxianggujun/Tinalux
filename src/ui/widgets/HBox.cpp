#include "tinalux/ui/HBox.h"

#include "tinalux/ui/Layout.h"

namespace tinalux::ui {

HBox::HBox()
{
    auto layout = std::make_unique<HBoxLayout>();
    layout_ = layout.get();
    setLayout(std::move(layout));
}

void HBox::setSpacing(float spacing)
{
    layout_->spacing = spacing;
    markLayoutDirty();
}

float HBox::spacing() const
{
    return layout_->spacing;
}

void HBox::setPadding(float padding)
{
    layout_->padding = padding;
    markLayoutDirty();
}

float HBox::padding() const
{
    return layout_->padding;
}

} // namespace tinalux::ui
