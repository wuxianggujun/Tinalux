#include "tinalux/ui/ListView.h"

#include <algorithm>
#include <cmath>
#include <limits>
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

void ListView::clearSource()
{
    const bool hadSelection = selectedIndex_ != -1;
    items_->clearChildren();
    realizedItems_.clear();
    recycledItems_.clear();
    itemBounds_.clear();
    itemHeights_.clear();
    itemWidths_.clear();
    itemLayoutVersions_.clear();
    measuredContentSize_ = core::Size::Make(0.0f, 0.0f);
    cachedViewportWidth_ = -1.0f;
    uniformItemHeight_ = 0.0f;
    sourceItemCount_ = 0;
    itemLayoutCacheValid_ = false;
    itemFactory_ = {};
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

void ListView::setItemFactory(
    std::size_t itemCountValue,
    ItemFactory factory)
{
    setItemFactory(itemCountValue, 0.0f, std::move(factory));
}

void ListView::setItemFactory(
    std::size_t itemCountValue,
    float itemHeight,
    ItemFactory factory)
{
    const float clampedItemHeight = std::max(0.0f, itemHeight);
    const bool hadSelection = selectedIndex_ != -1;

    items_->clearChildren();
    realizedItems_.clear();
    recycledItems_.clear();
    itemBounds_.clear();
    itemHeights_.clear();
    itemWidths_.clear();
    itemLayoutVersions_.clear();
    measuredContentSize_ = core::Size::Make(0.0f, 0.0f);
    cachedViewportWidth_ = -1.0f;
    itemLayoutCacheValid_ = false;
    scrollOffset_ = 0.0f;
    contentHeight_ = 0.0f;
    contentMeasureCacheValid_ = false;
    pressedIndex_ = -1;

    itemFactory_ = std::move(factory);
    sourceItemCount_ = itemFactory_ != nullptr
        && itemCountValue > 0
        ? itemCountValue
        : 0;
    uniformItemHeight_ = sourceItemCount_ > 0 && clampedItemHeight > 0.0f
        ? clampedItemHeight
        : 0.0f;
    if (sourceItemCount_ == 0) {
        itemFactory_ = {};
    }

    const int previousSelection = selectedIndex_;
    if (previousSelection >= static_cast<int>(itemCount())) {
        selectedIndex_ = -1;
    }

    markLayoutDirty();
    if (hadSelection && selectedIndex_ == -1 && onSelectionChanged_) {
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
    const float viewportHeightHint = preferredHeight_ > 0.0f
        ? std::min(preferredHeight_, constraints.maxHeight)
        : constraints.maxHeight;
    ensureItemLayoutCache(constraints.maxWidth, viewportHeightHint);

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
    ensureItemLayoutCache(bounds.width(), bounds.height());
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
    itemHeights_.clear();
    itemWidths_.clear();
    itemLayoutVersions_.clear();
    measuredContentSize_ = core::Size::Make(0.0f, 0.0f);
    cachedViewportWidth_ = -1.0f;
    itemLayoutCacheValid_ = false;
}

float ListView::fallbackItemHeight(const ListViewStyle& style) const
{
    float measuredHeightSum = 0.0f;
    std::size_t measuredHeightCount = 0;
    for (const float height : itemHeights_) {
        if (height <= 0.0f) {
            continue;
        }

        measuredHeightSum += height;
        ++measuredHeightCount;
    }

    if (measuredHeightCount > 0) {
        return measuredHeightSum / static_cast<float>(measuredHeightCount);
    }

    const Theme& theme = resolvedTheme();
    return std::max(
        24.0f,
        theme.typography.body1.fontSize * theme.typography.body1.lineHeight + style.padding * 2.0f);
}

float ListView::resolveViewportHeight(float viewportHeightHint) const
{
    if (std::isfinite(viewportHeightHint) && viewportHeightHint > 0.0f) {
        return viewportHeightHint;
    }

    if (preferredHeight_ > 0.0f) {
        return preferredHeight_;
    }

    if (bounds_.height() > 0.0f) {
        return bounds_.height();
    }

    return fallbackItemHeight(resolvedStyle()) * 8.0f;
}

void ListView::rebuildMeasuredContentFrom(std::size_t startIndex, float innerWidth, const ListViewStyle& style)
{
    const std::size_t totalItems = itemCount();
    if (itemBounds_.size() != totalItems) {
        itemBounds_.assign(totalItems, core::Rect::MakeEmpty());
        startIndex = 0;
    } else {
        startIndex = std::min(startIndex, totalItems);
    }

    float cursorY = style.padding;
    if (startIndex > 0) {
        cursorY = itemBounds_[startIndex - 1].bottom() + style.spacing;
    }

    float widestChild = 0.0f;
    for (const auto& cachedBounds : itemBounds_) {
        widestChild = std::max(widestChild, cachedBounds.width());
    }
    const float estimatedHeight = fallbackItemHeight(style);
    const float defaultWidth = std::isfinite(innerWidth) ? innerWidth : 0.0f;
    for (std::size_t index = startIndex; index < sourceItemCount_; ++index) {
        const float itemHeight = index < itemHeights_.size() && itemHeights_[index] > 0.0f
            ? itemHeights_[index]
            : estimatedHeight;
        const float itemWidth = index < itemWidths_.size() && itemWidths_[index] > 0.0f
            ? itemWidths_[index]
            : defaultWidth;
        widestChild = std::max(widestChild, itemWidth);
        itemBounds_[index] = core::Rect::MakeXYWH(
            style.padding,
            cursorY,
            itemWidth,
            itemHeight);
        cursorY += itemHeight;
        if (index + 1 < sourceItemCount_) {
            cursorY += style.spacing;
        }
    }

    measuredContentSize_ = core::Size::Make(
        widestChild + style.padding * 2.0f,
        totalItems == 0
            ? style.padding * 2.0f
            : itemBounds_.back().bottom() + style.padding);
}

std::vector<std::size_t> ListView::collectActiveItemIndices(float viewportHeight) const
{
    std::vector<std::size_t> activeIndices;
    if (itemCount() == 0) {
        return activeIndices;
    }

    const float visibleTop = scrollOffset_;
    const float visibleBottom = visibleTop + std::max(0.0f, viewportHeight);
    std::size_t start = firstItemIntersecting(visibleTop);
    std::size_t end = std::min(itemCount(), firstItemStartingAfter(visibleBottom));
    if (end <= start) {
        end = std::min(itemCount(), start + 1);
    }

    if (start > 0) {
        start -= std::min(start, kVisibleOverscanItems);
    }
    end = std::min(itemCount(), end + kVisibleOverscanItems);

    activeIndices.reserve((end - start) + realizedItems_.size() + 1);
    for (std::size_t index = start; index < end; ++index) {
        activeIndices.push_back(index);
    }
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(itemCount())) {
        activeIndices.push_back(static_cast<std::size_t>(selectedIndex_));
    }
    for (const auto& [index, item] : realizedItems_) {
        if (item != nullptr && item->focused()) {
            activeIndices.push_back(index);
        }
    }

    std::sort(activeIndices.begin(), activeIndices.end());
    activeIndices.erase(std::unique(activeIndices.begin(), activeIndices.end()), activeIndices.end());
    return activeIndices;
}

bool ListView::measureItemMetrics(std::size_t index, float innerWidth)
{
    if (index >= itemCount()) {
        return false;
    }

    const auto item = realizeItem(index);
    if (item == nullptr) {
        const bool hadCachedMetrics = itemHeights_[index] > 0.0f
            || itemWidths_[index] > 0.0f
            || itemLayoutVersions_[index] != 0;
        itemHeights_[index] = 0.0f;
        itemWidths_[index] = 0.0f;
        itemLayoutVersions_[index] = 0;
        return hadCachedMetrics;
    }

    const std::uint64_t layoutVersion = item->layoutVersion();
    if (itemHeights_[index] > 0.0f && itemLayoutVersions_[index] == layoutVersion) {
        return false;
    }

    const core::Size itemSize = item->measure({
        .minWidth = 0.0f,
        .maxWidth = innerWidth,
        .minHeight = 0.0f,
        .maxHeight = std::numeric_limits<float>::infinity(),
    });
    const float itemWidth = std::isfinite(innerWidth)
        ? std::min(itemSize.width(), innerWidth)
        : itemSize.width();
    const float itemHeight = std::max(0.0f, itemSize.height());

    const bool metricsChanged = !nearlyEqual(itemWidths_[index], itemWidth)
        || !nearlyEqual(itemHeights_[index], itemHeight)
        || itemLayoutVersions_[index] != layoutVersion;
    itemWidths_[index] = itemWidth;
    itemHeights_[index] = itemHeight;
    itemLayoutVersions_[index] = layoutVersion;
    return metricsChanged;
}

void ListView::ensureItemLayoutCache(float viewportWidth, float viewportHeight)
{
    const ListViewStyle style = resolvedStyle();
    const float normalizedViewportWidth = std::isfinite(viewportWidth)
        ? std::max(0.0f, viewportWidth)
        : std::numeric_limits<float>::infinity();
    const float innerWidth = std::isfinite(normalizedViewportWidth)
        ? std::max(0.0f, normalizedViewportWidth - style.padding * 2.0f)
        : std::numeric_limits<float>::infinity();

    const bool widthChanged = !itemLayoutCacheValid_
        || itemBounds_.size() != itemCount()
        || ((std::isfinite(normalizedViewportWidth) && !nearlyEqual(cachedViewportWidth_, normalizedViewportWidth))
            || (!std::isfinite(normalizedViewportWidth) && std::isfinite(cachedViewportWidth_)));

    if (usesUniformItemLayout()) {
        itemBounds_.clear();
        itemHeights_.clear();
        itemWidths_.clear();
        itemLayoutVersions_.clear();
        itemBounds_.reserve(itemCount());

        float cursorY = style.padding;
        const float itemWidth = std::isfinite(innerWidth) ? innerWidth : 0.0f;
        for (std::size_t index = 0; index < sourceItemCount_; ++index) {
            itemBounds_.push_back(core::Rect::MakeXYWH(
                style.padding,
                cursorY,
                itemWidth,
                uniformItemHeight_));
            cursorY += uniformItemHeight_;
            if (index + 1 < sourceItemCount_) {
                cursorY += style.spacing;
            }
        }

        measuredContentSize_ = core::Size::Make(
            itemWidth + style.padding * 2.0f,
            itemCount() == 0 ? style.padding * 2.0f : cursorY + style.padding);
        cachedViewportWidth_ = normalizedViewportWidth;
        itemLayoutCacheValid_ = true;
        return;
    }

    if (widthChanged
        || itemHeights_.size() != itemCount()
        || itemWidths_.size() != itemCount()
        || itemLayoutVersions_.size() != itemCount()) {
        itemHeights_.assign(itemCount(), 0.0f);
        itemWidths_.assign(itemCount(), 0.0f);
        itemLayoutVersions_.assign(itemCount(), 0);
    }

    bool metricsChanged = false;
    std::size_t firstChangedIndex = widthChanged ? 0 : itemCount();
    for (const auto& [index, item] : realizedItems_) {
        if (item == nullptr || index >= itemCount()) {
            continue;
        }

        if (itemLayoutVersions_[index] != 0 && itemLayoutVersions_[index] != item->layoutVersion()) {
            itemHeights_[index] = 0.0f;
            itemWidths_[index] = 0.0f;
            itemLayoutVersions_[index] = 0;
            metricsChanged = true;
            firstChangedIndex = std::min(firstChangedIndex, index);
        }
    }

    const bool needsProvisionalLayout = widthChanged
        || !itemLayoutCacheValid_
        || itemBounds_.size() != itemCount();
    if (needsProvisionalLayout) {
        rebuildMeasuredContentFrom(0, innerWidth, style);
    }

    const float effectiveViewportHeight = resolveViewportHeight(viewportHeight);
    const bool requireExactLayout = !std::isfinite(viewportHeight)
        && preferredHeight_ <= 0.0f
        && bounds_.height() <= 0.0f;
    std::vector<std::size_t> measureIndices;
    if (requireExactLayout) {
        measureIndices.reserve(itemCount());
        for (std::size_t index = 0; index < itemCount(); ++index) {
            measureIndices.push_back(index);
        }
    } else {
        measureIndices = collectActiveItemIndices(effectiveViewportHeight);
    }

    for (const std::size_t index : measureIndices) {
        if (!measureItemMetrics(index, innerWidth)) {
            continue;
        }

        metricsChanged = true;
        firstChangedIndex = std::min(firstChangedIndex, index);
    }

    if (metricsChanged) {
        rebuildMeasuredContentFrom(firstChangedIndex, innerWidth, style);
    }

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
    const std::vector<std::size_t> activeIndices =
        collectActiveItemIndices(resolveViewportHeight(bounds_.height()));

    if (itemCount() > 0) {
        recycleInactiveItems(activeIndices);

        visibleItems.reserve(activeIndices.size());
        visibleBounds.reserve(activeIndices.size());
        for (const std::size_t index : activeIndices) {
            const auto item = realizeItem(index);
            if (item == nullptr) {
                continue;
            }
            visibleItems.push_back(item);
            visibleBounds.push_back(itemBounds_[index]);
        }
    } else {
        recycleInactiveItems(activeIndices);
    }

    if (auto* virtualContent = dynamic_cast<VirtualListContent*>(items_.get()); virtualContent != nullptr) {
        virtualContent->setVisibleChildren(visibleItems, visibleBounds);
    }
}

std::size_t ListView::itemCount() const
{
    return sourceItemCount_;
}

bool ListView::usesUniformItemLayout() const
{
    return itemFactory_ != nullptr && uniformItemHeight_ > 0.0f;
}

void ListView::recycleInactiveItems(const std::vector<std::size_t>& activeIndices)
{
    for (auto it = realizedItems_.begin(); it != realizedItems_.end();) {
        if (std::binary_search(activeIndices.begin(), activeIndices.end(), it->first)) {
            ++it;
            continue;
        }

        if (it->second != nullptr) {
            recycledItems_.push_back(std::move(it->second));
        }
        it = realizedItems_.erase(it);
    }
}

std::shared_ptr<Widget> ListView::realizeItem(std::size_t index) const
{
    if (index >= sourceItemCount_ || itemFactory_ == nullptr) {
        return {};
    }

    if (const auto it = realizedItems_.find(index); it != realizedItems_.end()) {
        return it->second;
    }

    std::shared_ptr<Widget> recycledItem;
    while (!recycledItems_.empty() && recycledItem == nullptr) {
        recycledItem = std::move(recycledItems_.back());
        recycledItems_.pop_back();
    }

    auto item = itemFactory_(index, std::move(recycledItem));
    if (item != nullptr) {
        realizedItems_.emplace(index, item);
    }
    return item;
}

Widget* ListView::itemAtIndex(int index) const
{
    if (index < 0 || index >= static_cast<int>(itemCount())) {
        return nullptr;
    }
    return realizeItem(static_cast<std::size_t>(index)).get();
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

float ListView::desiredScrollOffsetForItem(int index) const
{
    if (index < 0 || index >= static_cast<int>(itemBounds_.size())) {
        return scrollOffset_;
    }

    const core::Rect itemBounds = itemBounds_[static_cast<std::size_t>(index)];
    if (itemBounds.isEmpty() || bounds_.height() <= 0.0f) {
        return scrollOffset_;
    }

    float nextOffset = scrollOffset_;
    if (itemBounds.top() < nextOffset) {
        nextOffset = itemBounds.top();
    } else if (itemBounds.bottom() > nextOffset + bounds_.height()) {
        nextOffset = itemBounds.bottom() - bounds_.height();
    }

    return std::clamp(nextOffset, 0.0f, maxScrollOffset());
}

void ListView::ensureItemVisible(int index)
{
    float nextOffset = desiredScrollOffsetForItem(index);
    if (std::abs(nextOffset - scrollOffset_) <= 0.001f) {
        return;
    }

    scrollOffset_ = nextOffset;
    markPaintDirty();
    applyVisibleItemLayout();

    nextOffset = desiredScrollOffsetForItem(index);
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
    ensureItemLayoutCache(bounds_.width(), bounds_.height());
    contentHeight_ = measuredContentSize_.height();
    clampScrollOffset();
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
