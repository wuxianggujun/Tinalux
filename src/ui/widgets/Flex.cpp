#include "tinalux/ui/Flex.h"

namespace tinalux::ui {

Flex::Flex()
{
    auto layout = std::make_unique<FlexLayout>();
    layout_ = layout.get();
    setLayout(std::move(layout));
}

void Flex::setDirection(FlexDirection direction) { layout_->direction = direction; markLayoutDirty(); }
FlexDirection Flex::direction() const { return layout_->direction; }

void Flex::setJustifyContent(JustifyContent justify) { layout_->justifyContent = justify; markLayoutDirty(); }
JustifyContent Flex::justifyContent() const { return layout_->justifyContent; }

void Flex::setAlignItems(AlignItems align) { layout_->alignItems = align; markLayoutDirty(); }
AlignItems Flex::alignItems() const { return layout_->alignItems; }

void Flex::setWrap(FlexWrap wrap) { layout_->wrap = wrap; markLayoutDirty(); }
FlexWrap Flex::wrap() const { return layout_->wrap; }

void Flex::setSpacing(float spacing) { layout_->spacing = spacing; markLayoutDirty(); }
float Flex::spacing() const { return layout_->spacing; }

void Flex::setPadding(float padding) { layout_->padding = padding; markLayoutDirty(); }
float Flex::padding() const { return layout_->padding; }

void Flex::setFlex(Widget* child, float grow, float shrink) { layout_->setFlex(child, grow, shrink); }
void Flex::clearFlex(Widget* child) { layout_->clearFlex(child); }

} // namespace tinalux::ui
