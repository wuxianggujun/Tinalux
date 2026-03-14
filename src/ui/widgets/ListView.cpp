#include "tinalux/ui/ListView.h"

#include "tinalux/ui/Container.h"
#include "tinalux/ui/Layout.h"

namespace tinalux::ui {

ListView::ListView()
    : items_(std::make_shared<Container>())
{
    auto layout = std::make_unique<VBoxLayout>();
    layout->padding = 8.0f;
    layout->spacing = 8.0f;
    layout_ = layout.get();
    items_->setLayout(std::move(layout));
    setContent(items_);
}

void ListView::addItem(std::shared_ptr<Widget> item)
{
    items_->addChild(std::move(item));
}

void ListView::clearItems()
{
    while (!items_->children().empty()) {
        items_->removeChild(items_->children().back().get());
    }
}

void ListView::setSpacing(float spacing)
{
    if (layout_ == nullptr || layout_->spacing == spacing) {
        return;
    }

    layout_->spacing = spacing;
    items_->markDirty();
    markDirty();
}

void ListView::setPadding(float padding)
{
    if (layout_ == nullptr || layout_->padding == padding) {
        return;
    }

    layout_->padding = padding;
    items_->markDirty();
    markDirty();
}

}  // namespace tinalux::ui
