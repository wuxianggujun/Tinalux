#include "tinalux/ui/ListView.h"

#include <algorithm>
#include <cmath>

#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
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
    const bool hadSelection = selectedIndex_ != -1;
    while (!items_->children().empty()) {
        items_->removeChild(items_->children().back().get());
    }
    pressedIndex_ = -1;
    selectedIndex_ = -1;
    if (hadSelection && onSelectionChanged_) {
        onSelectionChanged_(selectedIndex_);
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
    markPaintDirty();
}

void ListView::clearStyle()
{
    if (!customStyle_.has_value()) {
        return;
    }

    customStyle_.reset();
    applyResolvedStyle();
    markPaintDirty();
}

const ListViewStyle* ListView::style() const
{
    return customStyle_ ? &*customStyle_ : nullptr;
}

void ListView::setSelectedIndex(int index)
{
    updateSelection(index, true);
}

int ListView::selectedIndex() const
{
    return selectedIndex_;
}

Widget* ListView::selectedItem() const
{
    return itemAtIndex(selectedIndex_);
}

void ListView::onSelectionChanged(std::function<void(int)> handler)
{
    onSelectionChanged_ = std::move(handler);
}

bool ListView::focusable() const
{
    return true;
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
    ensureItemVisible(selectedIndex_);
}

void ListView::onDraw(rendering::Canvas& canvas)
{
    ScrollView::onDraw(canvas);
    if (!canvas) {
        return;
    }

    Widget* selectedItemWidget = itemAtIndex(selectedIndex_);
    if (selectedItemWidget == nullptr) {
        return;
    }

    core::Rect highlightBounds = selectedItemWidget->bounds().makeOffset(0.0f, -scrollOffset_);
    highlightBounds = highlightBounds.makeInset(1.0f, 1.0f);
    if (highlightBounds.isEmpty() || !highlightBounds.intersects(core::Rect::MakeWH(bounds_.width(), bounds_.height()))) {
        return;
    }

    const ListViewStyle style = resolvedStyle();
    const core::Color borderColor = focused() ? style.focusBorderColor : style.selectionBorderColor;
    canvas.save();
    canvas.clipRect(core::Rect::MakeWH(bounds_.width(), bounds_.height()));
    canvas.drawRoundRect(
        highlightBounds,
        style.itemCornerRadius,
        style.itemCornerRadius,
        style.selectionFillColor);
    canvas.drawRoundRect(
        highlightBounds,
        style.itemCornerRadius,
        style.itemCornerRadius,
        borderColor,
        rendering::PaintStyle::Stroke,
        focused() ? 2.0f : 1.5f);
    canvas.restore();
}

bool ListView::onEventCapture(core::Event& event)
{
    switch (event.type()) {
    case core::EventType::MouseButtonPress: {
        const auto& mouseEvent = static_cast<const core::MouseButtonEvent&>(event);
        if (mouseEvent.button != core::mouse::kLeft) {
            return false;
        }

        pressedIndex_ = indexForPoint(globalToLocal(core::Point::Make(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y))));
        return false;
    }
    case core::EventType::MouseButtonRelease: {
        const auto& mouseEvent = static_cast<const core::MouseButtonEvent&>(event);
        if (mouseEvent.button != core::mouse::kLeft) {
            return false;
        }

        const int releaseIndex = indexForPoint(globalToLocal(core::Point::Make(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y))));
        if (pressedIndex_ != -1 && pressedIndex_ == releaseIndex) {
            updateSelection(releaseIndex, true);
        }
        pressedIndex_ = -1;
        return false;
    }
    case core::EventType::KeyPress: {
        const auto& keyEvent = static_cast<const core::KeyEvent&>(event);
        if (items_->children().empty()) {
            return false;
        }

        switch (keyEvent.key) {
        case core::keys::kUp:
            updateSelection(selectedIndex_ <= 0 ? 0 : (selectedIndex_ - 1), true);
            return true;
        case core::keys::kDown:
            updateSelection(
                selectedIndex_ < 0 ? 0 : std::min(selectedIndex_ + 1, static_cast<int>(items_->children().size()) - 1),
                true);
            return true;
        case core::keys::kHome:
            updateSelection(0, true);
            return true;
        case core::keys::kEnd:
            updateSelection(static_cast<int>(items_->children().size()) - 1, true);
            return true;
        default:
            return false;
        }
    }
    default:
        return false;
    }
}

bool ListView::onEvent(core::Event& event)
{
    return ScrollView::onEvent(event);
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

Widget* ListView::itemAtIndex(int index) const
{
    if (index < 0 || index >= static_cast<int>(items_->children().size())) {
        return nullptr;
    }
    return items_->children()[static_cast<std::size_t>(index)].get();
}

int ListView::indexForPoint(core::Point localPoint) const
{
    if (!containsLocalPoint(localPoint.x(), localPoint.y())) {
        return -1;
    }

    const float contentY = localPoint.y() + scrollOffset_;
    const auto& children = items_->children();
    for (std::size_t index = 0; index < children.size(); ++index) {
        const core::Rect childBounds = children[index]->bounds();
        if (childBounds.contains(localPoint.x(), contentY)) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

void ListView::ensureItemVisible(int index)
{
    Widget* item = itemAtIndex(index);
    if (item == nullptr) {
        return;
    }

    const core::Rect itemBounds = item->bounds();
    if (itemBounds.isEmpty() || bounds_.height() <= 0.0f) {
        return;
    }

    float nextOffset = scrollOffset_;
    if (itemBounds.top() < nextOffset) {
        nextOffset = itemBounds.top();
    } else if (itemBounds.bottom() > nextOffset + bounds_.height()) {
        nextOffset = itemBounds.bottom() - bounds_.height();
    }

    nextOffset = std::clamp(nextOffset, 0.0f, maxScrollOffset());
    if (std::abs(nextOffset - scrollOffset_) <= 0.001f) {
        return;
    }

    scrollOffset_ = nextOffset;
    markPaintDirty();
}

void ListView::updateSelection(int index, bool emitCallback)
{
    if (index < -1 || index >= static_cast<int>(items_->children().size())) {
        index = -1;
    }

    if (selectedIndex_ == index) {
        ensureItemVisible(selectedIndex_);
        return;
    }

    selectedIndex_ = index;
    ensureItemVisible(selectedIndex_);
    markPaintDirty();
    if (emitCallback && onSelectionChanged_) {
        onSelectionChanged_(selectedIndex_);
    }
}

}  // namespace tinalux::ui
