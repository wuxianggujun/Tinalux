#include "tinalux/ui/ListView.h"

#include <algorithm>

#include "tinalux/ui/Container.h"
#include "tinalux/ui/Layout.h"
#include "tinalux/ui/Theme.h"

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
    applyResolvedStyle();
}

void ListView::addItem(std::shared_ptr<Widget> item)
{
    applyResolvedStyle();
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
    const float clampedSpacing = std::max(0.0f, spacing);
    if (spacingOverride_ && *spacingOverride_ == clampedSpacing) {
        return;
    }

    spacingOverride_ = clampedSpacing;
    applyResolvedStyle();
}

void ListView::setPadding(float padding)
{
    const float clampedPadding = std::max(0.0f, padding);
    if (paddingOverride_ && *paddingOverride_ == clampedPadding) {
        return;
    }

    paddingOverride_ = clampedPadding;
    applyResolvedStyle();
}

void ListView::setStyle(const ListViewStyle& style)
{
    customStyle_ = style;
    applyResolvedStyle();
}

void ListView::clearStyle()
{
    if (!customStyle_.has_value()) {
        return;
    }

    customStyle_.reset();
    applyResolvedStyle();
}

const ListViewStyle* ListView::style() const
{
    return customStyle_ ? &*customStyle_ : nullptr;
}

core::Size ListView::measure(const Constraints& constraints)
{
    applyResolvedStyle();
    return ScrollView::measure(constraints);
}

void ListView::arrange(const core::Rect& bounds)
{
    applyResolvedStyle();
    ScrollView::arrange(bounds);
}

ListViewStyle ListView::resolvedStyle() const
{
    ListViewStyle style = customStyle_ ? *customStyle_ : resolvedTheme().listViewStyle;
    if (paddingOverride_) {
        style.padding = *paddingOverride_;
    }
    if (spacingOverride_) {
        style.spacing = *spacingOverride_;
    }
    return style;
}

void ListView::applyResolvedStyle()
{
    if (layout_ == nullptr) {
        return;
    }

    const ListViewStyle style = resolvedStyle();
    if (layout_->padding == style.padding && layout_->spacing == style.spacing) {
        return;
    }

    layout_->padding = style.padding;
    layout_->spacing = style.spacing;
    items_->markLayoutDirty();
    markLayoutDirty();
}

}  // namespace tinalux::ui
