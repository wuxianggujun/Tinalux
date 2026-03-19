#include "tinalux/ui/Dropdown.h"

#include <algorithm>
#include <cstdint>
#include <utility>

#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Theme.h"

#include "../TextPrimitives.h"

namespace tinalux::ui {

namespace {

constexpr float kBaseDropdownItemHeight = 32.0f;
constexpr float kBaseDropdownMinWidth = 120.0f;
constexpr float kBaseDropdownPadding = 8.0f;
constexpr float kBaseDropdownIconSize = 12.0f;

core::Color brighten(core::Color color, int delta)
{
    const auto clampChannel = [delta](core::Color::Channel channel) -> core::Color::Channel {
        const int value = std::clamp(static_cast<int>(channel) + delta, 0, 255);
        return static_cast<core::Color::Channel>(value);
    };

    return core::colorARGB(
        color.alpha(),
        clampChannel(color.red()),
        clampChannel(color.green()),
        clampChannel(color.blue()));
}

float dropdownItemHeight(const Theme& theme)
{
    return std::max(kBaseDropdownItemHeight, theme.minimumTouchTargetSize);
}

float dropdownPadding(const Theme& theme)
{
    return std::max(kBaseDropdownPadding, theme.spacingScale.sm);
}

float dropdownIconSize(const Theme& theme)
{
    return std::max(kBaseDropdownIconSize, theme.typography.caption.fontSize);
}

core::Rect indicatorBounds(float width, float itemHeight, float padding, float iconSize)
{
    return core::Rect::MakeXYWH(
        width - padding - iconSize,
        (itemHeight - iconSize) * 0.5f,
        iconSize,
        iconSize);
}

void drawChevron(
    rendering::Canvas& canvas,
    core::Rect bounds,
    core::Color color,
    bool expanded)
{
    const float left = bounds.left() + 2.0f;
    const float right = bounds.right() - 2.0f;
    const float centerX = bounds.centerX();
    const float top = bounds.top() + 3.0f;
    const float bottom = bounds.bottom() - 3.0f;

    if (expanded) {
        canvas.drawLine(left, bottom - 1.0f, centerX, top, color, 1.75f, true);
        canvas.drawLine(centerX, top, right, bottom - 1.0f, color, 1.75f, true);
        return;
    }

    canvas.drawLine(left, top, centerX, bottom, color, 1.75f, true);
    canvas.drawLine(centerX, bottom, right, top, color, 1.75f, true);
}

}  // namespace

Dropdown::Dropdown(std::vector<std::string> items)
    : items_(std::move(items))
{
}

Dropdown::~Dropdown()
{
    if (indicatorIconLoadAlive_ != nullptr) {
        *indicatorIconLoadAlive_ = false;
    }
}

void Dropdown::setItems(const std::vector<std::string>& items)
{
    items_ = items;
    if (selectedIndex_ >= static_cast<int>(items_.size())) {
        selectedIndex_ = -1;
    }
    hoveredItem_ = -1;
    markLayoutDirty();
}

void Dropdown::setSelectedIndex(int index)
{
    if (index < -1 || index >= static_cast<int>(items_.size())) {
        index = -1;
    }

    if (selectedIndex_ == index) {
        return;
    }

    selectedIndex_ = index;
    markPaintDirty();
    if (onSelectionChanged_) {
        onSelectionChanged_(selectedIndex_);
    }
}

std::string Dropdown::selectedItem() const
{
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size())) {
        return items_[selectedIndex_];
    }
    return {};
}

void Dropdown::onSelectionChanged(std::function<void(int)> handler)
{
    onSelectionChanged_ = std::move(handler);
}

void Dropdown::setMaxVisibleItems(int count)
{
    const int clamped = std::max(1, count);
    if (maxVisibleItems_ == clamped) {
        return;
    }

    maxVisibleItems_ = clamped;
    hoveredItem_ = -1;
    markLayoutDirty();
}

void Dropdown::setPlaceholder(const std::string& placeholder)
{
    if (placeholder_ == placeholder) {
        return;
    }

    placeholder_ = placeholder;
    markLayoutDirty();
}

void Dropdown::setIndicatorIcon(rendering::Image icon)
{
    ++(*indicatorIconLoadGeneration_);
    pendingIndicatorIcon_ = {};
    indicatorIconPath_.clear();
    indicatorIconLoading_ = false;
    indicatorIcon_ = std::move(icon);
    indicatorIconLoadState_ = indicatorIcon_ ? DropdownIndicatorLoadState::Ready : DropdownIndicatorLoadState::Idle;
    markPaintDirty();
}

void Dropdown::setIndicatorIcon(IconType type, float sizeHint)
{
    setIndicatorIcon(IconRegistry::instance().getIcon(type, sizeHint));
}

const rendering::Image& Dropdown::indicatorIcon() const
{
    return indicatorIcon_;
}

void Dropdown::loadIndicatorIconAsync(const std::string& path)
{
    ++(*indicatorIconLoadGeneration_);
    indicatorIconPath_ = path;
    indicatorIcon_ = {};
    indicatorIconLoading_ = !path.empty();
    indicatorIconLoadState_ = indicatorIconLoading_ ? DropdownIndicatorLoadState::Loading : DropdownIndicatorLoadState::Idle;
    pendingIndicatorIcon_ = {};
    markPaintDirty();
    if (path.empty()) {
        return;
    }

    const std::uint64_t generation = *indicatorIconLoadGeneration_;
    pendingIndicatorIcon_ = ResourceLoader::instance().loadImageAsync(path);
    const std::shared_ptr<bool> alive = indicatorIconLoadAlive_;
    const std::shared_ptr<std::uint64_t> loadGeneration = indicatorIconLoadGeneration_;
    pendingIndicatorIcon_.onReady([this, alive, loadGeneration, generation](const rendering::Image& image) {
        if (alive == nullptr || !*alive || loadGeneration == nullptr || *loadGeneration != generation) {
            return;
        }

        indicatorIconLoading_ = false;
        indicatorIcon_ = image;
        indicatorIconLoadState_ = indicatorIcon_ ? DropdownIndicatorLoadState::Ready : DropdownIndicatorLoadState::Failed;
        markPaintDirty();
    });
}

const std::string& Dropdown::indicatorIconPath() const
{
    return indicatorIconPath_;
}

bool Dropdown::indicatorIconLoading() const
{
    return indicatorIconLoading_;
}

DropdownIndicatorLoadState Dropdown::indicatorIconLoadState() const
{
    return indicatorIconLoadState_;
}

core::Size Dropdown::measure(const Constraints& constraints)
{
    const Theme& theme = resolvedTheme();
    const float itemHeight = dropdownItemHeight(theme);
    const float padding = dropdownPadding(theme);
    const float iconSize = dropdownIconSize(theme);
    float maxTextWidth = kBaseDropdownMinWidth;
    const TextMetrics placeholderMetrics = measureTextMetrics(placeholder_, theme.bodyFontSize());
    maxTextWidth = std::max(maxTextWidth, placeholderMetrics.width);
    for (const std::string& item : items_) {
        const TextMetrics metrics = measureTextMetrics(item, theme.bodyFontSize());
        maxTextWidth = std::max(maxTextWidth, metrics.width);
    }

    const float width = maxTextWidth + padding * 3.0f + iconSize;
    const int visibleCount = std::min(static_cast<int>(items_.size()), maxVisibleItems_);
    const float height = itemHeight + (expanded_ ? visibleCount * itemHeight : 0.0f);
    return constraints.constrain(core::Size::Make(width, height));
}

void Dropdown::onDraw(rendering::Canvas& canvas)
{
    if (!canvas) {
        return;
    }

    const Theme& theme = resolvedTheme();
    const float itemHeight = dropdownItemHeight(theme);
    const float padding = dropdownPadding(theme);
    const float iconSize = dropdownIconSize(theme);
    const core::Rect buttonBounds = core::Rect::MakeXYWH(0.0f, 0.0f, bounds_.width(), itemHeight);
    const core::Color backgroundColor = focused()
        ? theme.colors.primary
        : (hovered_ ? brighten(theme.colors.surface, 20) : theme.colors.surface);
    const core::Color textColor = focused()
        ? theme.colors.onPrimary
        : (selectedItem().empty() ? theme.secondaryTextColor() : theme.textColor());

    canvas.drawRoundRect(
        buttonBounds,
        theme.cornerRadius(),
        theme.cornerRadius(),
        backgroundColor);
    canvas.drawRoundRect(
        buttonBounds,
        theme.cornerRadius(),
        theme.cornerRadius(),
        theme.colors.border,
        rendering::PaintStyle::Stroke,
        1.0f);

    std::string displayText = selectedItem();
    if (displayText.empty()) {
        displayText = placeholder_;
    }

    const TextMetrics textMetrics = measureTextMetrics(displayText, theme.bodyFontSize());
    const float textY = (itemHeight - textMetrics.height) * 0.5f + textMetrics.baseline;
    canvas.drawText(
        displayText,
        padding + textMetrics.drawX,
        textY,
        theme.bodyFontSize(),
        textColor);

    if (!expanded_ && indicatorIcon_) {
        canvas.drawImage(indicatorIcon_, indicatorBounds(bounds_.width(), itemHeight, padding, iconSize));
    } else {
        drawChevron(
            canvas,
            indicatorBounds(bounds_.width(), itemHeight, padding, iconSize),
            textColor,
            expanded_);
    }

    if (!expanded_ || items_.empty()) {
        return;
    }

    const core::Rect dropdownBounds = getDropdownBounds();
    canvas.drawRoundRect(
        dropdownBounds,
        theme.cornerRadius(),
        theme.cornerRadius(),
        theme.colors.surface);
    canvas.drawRoundRect(
        dropdownBounds,
        theme.cornerRadius(),
        theme.cornerRadius(),
        theme.colors.border,
        rendering::PaintStyle::Stroke,
        1.0f);

    const int visibleCount = std::min(static_cast<int>(items_.size()), maxVisibleItems_);
    for (int i = 0; i < visibleCount; ++i) {
        const float itemY = dropdownBounds.y() + static_cast<float>(i) * itemHeight;
        const core::Rect itemRect = core::Rect::MakeXYWH(
            dropdownBounds.x(),
            itemY,
            dropdownBounds.width(),
            itemHeight);

        if (i == hoveredItem_) {
            canvas.drawRect(itemRect, core::colorARGB(56, 255, 255, 255));
        } else if (i == selectedIndex_) {
            canvas.drawRect(
                itemRect,
                core::colorARGB(
                    36,
                    theme.colors.primary.red(),
                    theme.colors.primary.green(),
                    theme.colors.primary.blue()));
        }

        const TextMetrics itemMetrics = measureTextMetrics(
            items_[static_cast<std::size_t>(i)],
            theme.bodyFontSize());
        const float itemTextY = itemRect.y() + (itemHeight - itemMetrics.height) * 0.5f + itemMetrics.baseline;
        canvas.drawText(
            items_[static_cast<std::size_t>(i)],
            itemRect.x() + padding + itemMetrics.drawX,
            itemTextY,
            theme.bodyFontSize(),
            theme.textColor());
    }
}

bool Dropdown::onEvent(core::Event& event)
{
    switch (event.type()) {
    case core::EventType::MouseEnter:
        if (!hovered_) {
            hovered_ = true;
            markPaintDirty();
        }
        return false;
    case core::EventType::MouseLeave:
        if (hovered_ || hoveredItem_ != -1) {
            hovered_ = false;
            hoveredItem_ = -1;
            markPaintDirty();
        }
        return false;
    case core::EventType::MouseMove: {
        const auto& moveEvent = static_cast<const core::MouseMoveEvent&>(event);
        const float itemHeight = dropdownItemHeight(resolvedTheme());
        const core::Point localPoint = globalToLocal(core::Point::Make(
            static_cast<float>(moveEvent.x),
            static_cast<float>(moveEvent.y)));
        const bool insideButton = localPoint.y() >= 0.0f
            && localPoint.y() < itemHeight
            && containsLocalPoint(localPoint.x(), localPoint.y());
        const bool hoveredChanged = hovered_ != insideButton;
        hovered_ = insideButton;

        int newHoveredItem = -1;
        if (expanded_ && getDropdownBounds().contains(localPoint.x(), localPoint.y())) {
            newHoveredItem = getItemAtY(localPoint.y() - itemHeight);
        }

        if (hoveredChanged || newHoveredItem != hoveredItem_) {
            hoveredItem_ = newHoveredItem;
            markPaintDirty();
        }
        return expanded_;
    }
    case core::EventType::MouseButtonPress: {
        const auto& mouseEvent = static_cast<const core::MouseButtonEvent&>(event);
        if (mouseEvent.button != core::mouse::kLeft) {
            return false;
        }

        const float itemHeight = dropdownItemHeight(resolvedTheme());
        const core::Point localPoint = globalToLocal(core::Point::Make(
            static_cast<float>(mouseEvent.x),
            static_cast<float>(mouseEvent.y)));
        if (localPoint.y() >= 0.0f && localPoint.y() < itemHeight
            && containsLocalPoint(localPoint.x(), localPoint.y())) {
            toggleExpanded();
            return true;
        }

        if (expanded_ && getDropdownBounds().contains(localPoint.x(), localPoint.y())) {
            const int itemIndex = getItemAtY(localPoint.y() - itemHeight);
            if (itemIndex >= 0) {
                selectItem(itemIndex);
                return true;
            }
        }

        if (expanded_) {
            expanded_ = false;
            hoveredItem_ = -1;
            markLayoutDirty();
            return false;
        }
        return false;
    }
    case core::EventType::KeyPress: {
        const auto& keyEvent = static_cast<const core::KeyEvent&>(event);
        if (!focused()) {
            return false;
        }

        if (keyEvent.key == core::keys::kEnter
            || keyEvent.key == core::keys::kKpEnter
            || keyEvent.key == core::keys::kSpace) {
            toggleExpanded();
            return true;
        }
        if (keyEvent.key == core::keys::kEscape && expanded_) {
            expanded_ = false;
            hoveredItem_ = -1;
            markLayoutDirty();
            return true;
        }
        if (keyEvent.key == core::keys::kUp && expanded_) {
            if (selectedIndex_ > 0) {
                setSelectedIndex(selectedIndex_ - 1);
            }
            return true;
        }
        if (keyEvent.key == core::keys::kDown && expanded_) {
            if (selectedIndex_ < static_cast<int>(items_.size()) - 1) {
                setSelectedIndex(selectedIndex_ + 1);
            } else if (selectedIndex_ < 0 && !items_.empty()) {
                setSelectedIndex(0);
            }
            return true;
        }
        return false;
    }
    default:
        return false;
    }
}

Widget* Dropdown::hitTest(float x, float y)
{
    return Widget::hitTest(x, y);
}

void Dropdown::toggleExpanded()
{
    expanded_ = !expanded_;
    if (!expanded_) {
        hoveredItem_ = -1;
    }
    markLayoutDirty();
}

void Dropdown::selectItem(int index)
{
    setSelectedIndex(index);
    expanded_ = false;
    hoveredItem_ = -1;
    markLayoutDirty();
}

int Dropdown::getItemAtY(float y) const
{
    if (y < 0.0f) {
        return -1;
    }

    const int index = static_cast<int>(y / dropdownItemHeight(resolvedTheme()));
    const int visibleCount = std::min(static_cast<int>(items_.size()), maxVisibleItems_);
    if (index >= 0 && index < visibleCount) {
        return index;
    }
    return -1;
}

core::Rect Dropdown::getDropdownBounds() const
{
    const float itemHeight = dropdownItemHeight(resolvedTheme());
    const int visibleCount = std::min(static_cast<int>(items_.size()), maxVisibleItems_);
    const float dropdownHeight = static_cast<float>(visibleCount) * itemHeight;
    return core::Rect::MakeXYWH(0.0f, itemHeight, bounds_.width(), dropdownHeight);
}

}  // namespace tinalux::ui
