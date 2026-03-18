#include "tinalux/ui/ListView.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <vector>

#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Container.h"
#include "tinalux/ui/Theme.h"

namespace tinalux::ui {

namespace {

constexpr float kGeometryTolerance = 0.001f;
constexpr std::size_t kVisibleOverscanItems = 3;

bool nearlyEqual(float lhs, float rhs)
{
    return std::abs(lhs - rhs) <= kGeometryTolerance;
}

class VirtualListContent final : public Container {
public:
    core::Size measure(const Constraints& constraints) override
    {
        return constraints.constrain(measuredSize_);
    }

    void arrange(const core::Rect& bounds) override
    {
        Widget::arrange(bounds);
        for (std::size_t index = 0; index < children_.size() && index < childBounds_.size(); ++index) {
            children_[index]->arrange(childBounds_[index]);
        }
    }

    void setMeasuredSize(core::Size size)
    {
        measuredSize_ = size;
    }

    void setVisibleChildren(
        const std::vector<std::shared_ptr<Widget>>& children,
        const std::vector<core::Rect>& bounds)
    {
        if (children.size() != bounds.size()) {
            return;
        }

        bool sameChildren = children_.size() == children.size();
        if (sameChildren) {
            for (std::size_t index = 0; index < children.size(); ++index) {
                if (children_[index].get() != children[index].get()) {
                    sameChildren = false;
                    break;
                }
            }
        }

        childBounds_.clear();
        childBounds_.reserve(bounds.size());
        for (const auto& bound : bounds) {
            childBounds_.push_back(bound);
        }

        if (!sameChildren) {
            replaceChildrenDirect(std::vector<std::shared_ptr<Widget>>(children.begin(), children.end()));
        }
    }

private:
    core::Size measuredSize_ = core::Size::Make(0.0f, 0.0f);
    std::vector<core::Rect> childBounds_;
};

}  // namespace

ListView::ListView()
    : items_(std::make_shared<VirtualListContent>())
{
    setContent(items_);
    applyResolvedStyle();
}

void ListView::addItem(std::shared_ptr<Widget> item)
{
    if (item == nullptr) {
        return;
    }

    if (usingUniformDataSource_) {
        clearItems();
    }

    itemStorage_.push_back(std::move(item));
    invalidateItemLayoutCache();
    markLayoutDirty();
}

void ListView::clearItems()
{
    const bool hadSelection = selectedIndex_ != -1;
    items_->clearChildren();
    itemStorage_.clear();
    dataSourceCache_.clear();
    itemBounds_.clear();
    itemLayoutVersions_.clear();
    measuredContentSize_ = core::Size::Make(0.0f, 0.0f);
    cachedViewportWidth_ = -1.0f;
    uniformItemHeight_ = 0.0f;
    uniformItemCount_ = 0;
    itemLayoutCacheValid_ = false;
    usingUniformDataSource_ = false;
    uniformItemFactory_ = {};
    scrollOffset_ = 0.0f;
    contentHeight_ = 0.0f;
    contentMeasureCacheValid_ = false;
    pressedIndex_ = -1;
    selectedIndex_ = -1;
    markLayoutDirty();
    if (hadSelection && onSelectionChanged_) {
        onSelectionChanged_(selectedIndex_);
    }
}

void ListView::setUniformDataSource(
    std::size_t itemCountValue,
    float itemHeight,
    UniformItemFactory factory)
{
    const float clampedItemHeight = std::max(0.0f, itemHeight);
    const bool hadSelection = selectedIndex_ != -1;

    items_->clearChildren();
    itemStorage_.clear();
    dataSourceCache_.clear();
    itemBounds_.clear();
    itemLayoutVersions_.clear();
    measuredContentSize_ = core::Size::Make(0.0f, 0.0f);
    cachedViewportWidth_ = -1.0f;
    itemLayoutCacheValid_ = false;
    scrollOffset_ = 0.0f;
    contentHeight_ = 0.0f;
    contentMeasureCacheValid_ = false;
    pressedIndex_ = -1;

    uniformItemFactory_ = std::move(factory);
    usingUniformDataSource_ = uniformItemFactory_ != nullptr
        && itemCountValue > 0
        && clampedItemHeight > 0.0f;
    uniformItemCount_ = usingUniformDataSource_ ? itemCountValue : 0;
    uniformItemHeight_ = usingUniformDataSource_ ? clampedItemHeight : 0.0f;

    const int previousSelection = selectedIndex_;
    if (previousSelection >= static_cast<int>(itemCount())) {
        selectedIndex_ = -1;
    }

    markLayoutDirty();
    if (hadSelection && selectedIndex_ == -1 && onSelectionChanged_) {
        onSelectionChanged_(selectedIndex_);
    }
}

void ListView::clearDataSource()
{
    if (!usingUniformDataSource_) {
        return;
    }
    clearItems();
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
    ensureItemLayoutCache(constraints.maxWidth);

    const float desiredHeight = preferredHeight_ > 0.0f
        ? std::min(preferredHeight_, measuredContentSize_.height())
        : measuredContentSize_.height();
    contentHeight_ = measuredContentSize_.height();
    return constraints.constrain(core::Size::Make(measuredContentSize_.width(), desiredHeight));
}

void ListView::arrange(const core::Rect& bounds)
{
    applyResolvedStyle();
    Widget::arrange(bounds);
    ensureItemLayoutCache(bounds.width());
    contentHeight_ = measuredContentSize_.height();
    clampScrollOffset();
    ensureItemVisible(selectedIndex_);
    applyVisibleItemLayout();
}

void ListView::onDraw(rendering::Canvas& canvas)
{
    ScrollView::onDraw(canvas);
    if (!canvas) {
        return;
    }

    if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(itemBounds_.size())) {
        return;
    }

    core::Rect highlightBounds = itemBounds_[static_cast<std::size_t>(selectedIndex_)].makeOffset(0.0f, -scrollOffset_);
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
        if (itemCount() == 0) {
            return false;
        }

        switch (keyEvent.key) {
        case core::keys::kUp:
            updateSelection(selectedIndex_ <= 0 ? 0 : (selectedIndex_ - 1), true);
            return true;
        case core::keys::kDown:
            updateSelection(
                selectedIndex_ < 0 ? 0 : std::min(selectedIndex_ + 1, static_cast<int>(itemCount()) - 1),
                true);
            return true;
        case core::keys::kHome:
            updateSelection(0, true);
            return true;
        case core::keys::kEnd:
            updateSelection(static_cast<int>(itemCount()) - 1, true);
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
    const bool handled = ScrollView::onEvent(event);
    if (handled && event.type() == core::EventType::MouseScroll) {
        applyVisibleItemLayout();
    }
    return handled;
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
    const ListViewStyle style = resolvedStyle();
    if (nearlyEqual(appliedPadding_, style.padding) && nearlyEqual(appliedSpacing_, style.spacing)) {
        return;
    }

    appliedPadding_ = style.padding;
    appliedSpacing_ = style.spacing;
    invalidateItemLayoutCache();
    markLayoutDirty();
}

void ListView::invalidateItemLayoutCache()
{
    itemBounds_.clear();
    itemLayoutVersions_.clear();
    measuredContentSize_ = core::Size::Make(0.0f, 0.0f);
    cachedViewportWidth_ = -1.0f;
    itemLayoutCacheValid_ = false;
}

void ListView::ensureItemLayoutCache(float viewportWidth)
{
    const ListViewStyle style = resolvedStyle();
    const float normalizedViewportWidth = std::isfinite(viewportWidth)
        ? std::max(0.0f, viewportWidth)
        : std::numeric_limits<float>::infinity();

    bool canReuseCache = itemLayoutCacheValid_
        && itemBounds_.size() == itemCount()
        && ((std::isfinite(normalizedViewportWidth) && nearlyEqual(cachedViewportWidth_, normalizedViewportWidth))
            || (!std::isfinite(normalizedViewportWidth) && !std::isfinite(cachedViewportWidth_)));

    if (canReuseCache && !usingUniformDataSource_) {
        if (itemLayoutVersions_.size() != itemStorage_.size()) {
            canReuseCache = false;
        } else {
            for (std::size_t index = 0; index < itemStorage_.size(); ++index) {
                if (itemLayoutVersions_[index] != itemStorage_[index]->layoutVersion()) {
                    canReuseCache = false;
                    break;
                }
            }
        }
    }

    if (canReuseCache) {
        return;
    }

    const float innerWidth = std::isfinite(normalizedViewportWidth)
        ? std::max(0.0f, normalizedViewportWidth - style.padding * 2.0f)
        : std::numeric_limits<float>::infinity();

    itemBounds_.clear();
    itemLayoutVersions_.clear();
    itemBounds_.reserve(itemCount());
    itemLayoutVersions_.reserve(itemStorage_.size());

    float cursorY = style.padding;
    float widestChild = 0.0f;
    if (usingUniformDataSource_) {
        const float itemWidth = std::isfinite(innerWidth) ? innerWidth : 0.0f;
        widestChild = itemWidth;
        for (std::size_t index = 0; index < uniformItemCount_; ++index) {
            itemBounds_.push_back(core::Rect::MakeXYWH(
                style.padding,
                cursorY,
                itemWidth,
                uniformItemHeight_));
            cursorY += uniformItemHeight_;
            if (index + 1 < uniformItemCount_) {
                cursorY += style.spacing;
            }
        }
    } else {
        for (std::size_t index = 0; index < itemStorage_.size(); ++index) {
            const core::Size itemSize = itemStorage_[index]->measure({
                .minWidth = 0.0f,
                .maxWidth = innerWidth,
                .minHeight = 0.0f,
                .maxHeight = std::numeric_limits<float>::infinity(),
            });
            const float itemWidth = std::isfinite(innerWidth)
                ? std::min(itemSize.width(), innerWidth)
                : itemSize.width();
            widestChild = std::max(widestChild, itemWidth);
            itemBounds_.push_back(core::Rect::MakeXYWH(
                style.padding,
                cursorY,
                itemWidth,
                itemSize.height()));
            itemLayoutVersions_.push_back(itemStorage_[index]->layoutVersion());
            cursorY += itemSize.height();
            if (index + 1 < itemStorage_.size()) {
                cursorY += style.spacing;
            }
        }
    }

    measuredContentSize_ = core::Size::Make(
        widestChild + style.padding * 2.0f,
        itemCount() == 0 ? style.padding * 2.0f : cursorY + style.padding);
    cachedViewportWidth_ = normalizedViewportWidth;
    itemLayoutCacheValid_ = true;
}

std::size_t ListView::firstItemIntersecting(float contentY) const
{
    std::size_t low = 0;
    std::size_t high = itemBounds_.size();
    while (low < high) {
        const std::size_t index = low + (high - low) / 2;
        if (itemBounds_[index].bottom() <= contentY) {
            low = index + 1;
        } else {
            high = index;
        }
    }
    return low;
}

std::size_t ListView::firstItemStartingAfter(float contentY) const
{
    std::size_t low = 0;
    std::size_t high = itemBounds_.size();
    while (low < high) {
        const std::size_t index = low + (high - low) / 2;
        if (itemBounds_[index].top() < contentY) {
            low = index + 1;
        } else {
            high = index;
        }
    }
    return low;
}

void ListView::syncVisibleItems()
{
    std::vector<std::shared_ptr<Widget>> visibleItems;
    std::vector<core::Rect> visibleBounds;

    if (itemCount() > 0) {
        const float visibleTop = scrollOffset_;
        const float visibleBottom = scrollOffset_ + bounds_.height();
        std::size_t start = firstItemIntersecting(visibleTop);
        std::size_t end = std::min(itemCount(), firstItemStartingAfter(visibleBottom));

        if (start > 0) {
            start -= std::min(start, kVisibleOverscanItems);
        }
        end = std::min(itemCount(), end + kVisibleOverscanItems);

        std::set<std::size_t> activeIndices;
        for (std::size_t index = start; index < end; ++index) {
            activeIndices.insert(index);
        }
        if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(itemCount())) {
            activeIndices.insert(static_cast<std::size_t>(selectedIndex_));
        }
        if (usingUniformDataSource_) {
            for (const auto& [index, item] : dataSourceCache_) {
                if (item != nullptr && item->focused()) {
                    activeIndices.insert(index);
                }
            }
        } else {
            for (std::size_t index = 0; index < itemStorage_.size(); ++index) {
                if (itemStorage_[index]->focused()) {
                    activeIndices.insert(index);
                }
            }
        }

        visibleItems.reserve(activeIndices.size());
        visibleBounds.reserve(activeIndices.size());
        for (const std::size_t index : activeIndices) {
            const auto item = materializeItem(index);
            if (item == nullptr) {
                continue;
            }
            visibleItems.push_back(item);
            visibleBounds.push_back(itemBounds_[index]);
        }

        if (usingUniformDataSource_) {
            for (auto it = dataSourceCache_.begin(); it != dataSourceCache_.end();) {
                if (activeIndices.contains(it->first) || (it->second != nullptr && it->second->focused())) {
                    ++it;
                    continue;
                }
                it = dataSourceCache_.erase(it);
            }
        }
    }

    if (auto* virtualContent = dynamic_cast<VirtualListContent*>(items_.get()); virtualContent != nullptr) {
        virtualContent->setVisibleChildren(visibleItems, visibleBounds);
    }
}

std::size_t ListView::itemCount() const
{
    return usingUniformDataSource_ ? uniformItemCount_ : itemStorage_.size();
}

std::shared_ptr<Widget> ListView::materializeItem(std::size_t index) const
{
    if (usingUniformDataSource_) {
        if (index >= uniformItemCount_ || uniformItemFactory_ == nullptr) {
            return {};
        }

        if (const auto it = dataSourceCache_.find(index); it != dataSourceCache_.end()) {
            return it->second;
        }

        auto item = uniformItemFactory_(index);
        if (item != nullptr) {
            dataSourceCache_.emplace(index, item);
        }
        return item;
    }

    if (index >= itemStorage_.size()) {
        return {};
    }
    return itemStorage_[index];
}

Widget* ListView::itemAtIndex(int index) const
{
    if (index < 0 || index >= static_cast<int>(itemCount())) {
        return nullptr;
    }
    return materializeItem(static_cast<std::size_t>(index)).get();
}

int ListView::indexForPoint(core::Point localPoint) const
{
    if (!containsLocalPoint(localPoint.x(), localPoint.y())) {
        return -1;
    }

    const float contentY = localPoint.y() + scrollOffset_;
    const std::size_t index = firstItemIntersecting(contentY);
    if (index >= itemBounds_.size()) {
        return -1;
    }
    return itemBounds_[index].contains(localPoint.x(), contentY)
        ? static_cast<int>(index)
        : -1;
}

void ListView::ensureItemVisible(int index)
{
    if (index < 0 || index >= static_cast<int>(itemBounds_.size())) {
        return;
    }

    const core::Rect itemBounds = itemBounds_[static_cast<std::size_t>(index)];
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
    applyVisibleItemLayout();
}

void ListView::updateSelection(int index, bool emitCallback)
{
    if (index < -1 || index >= static_cast<int>(itemCount())) {
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

void ListView::applyVisibleItemLayout()
{
    if (auto* virtualContent = dynamic_cast<VirtualListContent*>(items_.get()); virtualContent != nullptr) {
        virtualContent->setMeasuredSize(core::Size::Make(
            std::max(bounds_.width(), measuredContentSize_.width()),
            contentHeight_));
    }
    syncVisibleItems();
    items_->arrange(core::Rect::MakeXYWH(
        0.0f,
        0.0f,
        std::max(bounds_.width(), measuredContentSize_.width()),
        contentHeight_));
}

}  // namespace tinalux::ui
