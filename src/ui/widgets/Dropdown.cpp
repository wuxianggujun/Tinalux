#include "tinalux/ui/Dropdown.h"

#include <algorithm>
#include <cmath>

#include "tinalux/core/KeyCodes.h"
#include "tinalux/core/events/Event.h"
#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Theme.h"

#include "../TextPrimitives.h"

namespace tinalux::ui {

Dropdown::Dropdown(std::vector<std::string> items)
    : items_(std::move(items))
{
}

void Dropdown::setItems(const std::vector<std::string>& items)
{
    items_ = items;
    if (selectedIndex_ >= static_cast<int>(items_.size())) {
        selectedIndex_ = -1;
    }
    markLayoutDirty();
}

void Dropdown::setSelectedIndex(int index)
{
    if (index < -1 || index >= static_cast<int>(items_.size())) {
        index = -1;
    }
    
    if (selectedIndex_ != index) {
        selectedIndex_ = index;
        markDirty();
        
        if (onSelectionChanged_) {
            onSelectionChanged_(selectedIndex_);
        }
    }
}

std::string Dropdown::selectedItem() const
{
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size())) {
        return items_[selectedIndex_];
    }
    return "";
}

void Dropdown::onSelectionChanged(std::function<void(int)> handler)
{
    onSelectionChanged_ = std::move(handler);
}

void Dropdown::setMaxVisibleItems(int count)
{
    if (maxVisibleItems_ != count) {
        maxVisibleItems_ = std::max(1, count);
        markLayoutDirty();
    }
}

void Dropdown::setPlaceholder(const std::string& placeholder)
{
    if (placeholder_ != placeholder) {
        placeholder_ = placeholder;
        markDirty();
    }
}

core::Size Dropdown::measure(const Constraints& constraints)
{
    const auto& theme = currentTheme();
    
    // 计算最大文本宽度
    float maxTextWidth = kMinWidth;
    for (const auto& item : items_) {
        auto metrics = measureTextMetrics(item, theme.fontSize);
        maxTextWidth = std::max(maxTextWidth, metrics.width);
    }
    
    // 加上padding和图标空间
    float width = maxTextWidth + kPadding * 2 + kIconSize + kPadding;
    float height = kItemHeight;
    
    return constraints.constrain(core::Size::Make(width, height));
}

void Dropdown::onDraw(rendering::Canvas& canvas)
{
    const auto& theme = currentTheme();
    const float w = bounds().width();
    const float h = bounds().height();
    
    // 绘制主按钮背景
    core::Color bgColor = theme.surface;
    if (focused()) {
        bgColor = theme.primary;
    } else if (hovered_) {
        bgColor = core::colorARGB(255, 
            core::colorGetR(theme.surface) + 20,
            core::colorGetG(theme.surface) + 20,
            core::colorGetB(theme.surface) + 20);
    }
    
    canvas.drawRoundRect(
        core::Rect::MakeWH(w, h),
        theme.cornerRadius,
        bgColor
    );
    
    // 绘制边框
    canvas.drawRoundRectStroke(
        core::Rect::MakeWH(w, h),
        theme.cornerRadius,
        1.0f,
        theme.border
    );
    
    // 绘制选中项文本或占位符
    std::string displayText = selectedItem();
    if (displayText.empty()) {
        displayText = placeholder_;
    }
    
    core::Color textColor = selectedItem().empty() ? theme.textSecondary : theme.text;
    if (focused()) {
        textColor = theme.onPrimary;
    }
    
    auto metrics = measureTextMetrics(displayText, theme.fontSize);
    float textX = kPadding;
    float textY = (h - metrics.height) / 2 + metrics.ascent;
    
    canvas.drawText(
        displayText,
        core::Point::Make(textX, textY),
        theme.fontSize,
        textColor
    );
    
    // 绘制下拉箭头
    float arrowX = w - kPadding - kIconSize;
    float arrowY = (h - kIconSize) / 2;
    
    // 简单的三角形箭头
    float arrowSize = kIconSize / 2;
    float arrowCenterX = arrowX + kIconSize / 2;
    float arrowCenterY = arrowY + kIconSize / 2;
    
    if (expanded_) {
        // 向上箭头
        canvas.drawTriangle(
            core::Point::Make(arrowCenterX - arrowSize, arrowCenterY + arrowSize / 2),
            core::Point::Make(arrowCenterX + arrowSize, arrowCenterY + arrowSize / 2),
            core::Point::Make(arrowCenterX, arrowCenterY - arrowSize / 2),
            textColor
        );
    } else {
        // 向下箭头
        canvas.drawTriangle(
            core::Point::Make(arrowCenterX - arrowSize, arrowCenterY - arrowSize / 2),
            core::Point::Make(arrowCenterX + arrowSize, arrowCenterY - arrowSize / 2),
            core::Point::Make(arrowCenterX, arrowCenterY + arrowSize / 2),
            textColor
        );
    }
    
    // 绘制下拉列表
    if (expanded_ && !items_.empty()) {
        auto dropdownBounds = getDropdownBounds();
        
        // 绘制下拉列表背景
        canvas.drawRoundRect(
            dropdownBounds,
            theme.cornerRadius,
            theme.surface
        );
        
        // 绘制边框
        canvas.drawRoundRectStroke(
            dropdownBounds,
            theme.cornerRadius,
            1.0f,
            theme.border
        );
        
        // 绘制每个选项
        int visibleCount = std::min(static_cast<int>(items_.size()), maxVisibleItems_);
        for (int i = 0; i < visibleCount; ++i) {
            float itemY = dropdownBounds.top() + i * kItemHeight;
            core::Rect itemRect = core::Rect::MakeXYWH(
                dropdownBounds.left(),
                itemY,
                dropdownBounds.width(),
                kItemHeight
            );
            
            // 绘制选项背景（悬停或选中）
            if (i == hoveredItem_) {
                canvas.drawRect(itemRect, core::colorARGB(50, 255, 255, 255));
            } else if (i == selectedIndex_) {
                canvas.drawRect(itemRect, core::colorARGB(30, 
                    core::colorGetR(theme.primary),
                    core::colorGetG(theme.primary),
                    core::colorGetB(theme.primary)));
            }
            
            // 绘制选项文本
            auto itemMetrics = measureTextMetrics(items_[i], theme.fontSize);
            float itemTextX = itemRect.left() + kPadding;
            float itemTextY = itemRect.top() + (kItemHeight - itemMetrics.height) / 2 + itemMetrics.ascent;
            
            canvas.drawText(
                items_[i],
                core::Point::Make(itemTextX, itemTextY),
                theme.fontSize,
                theme.text
            );
        }
    }
}

bool Dropdown::onEvent(core::Event& event)
{
    if (auto* mouseBtn = dynamic_cast<core::MouseButtonEvent*>(&event)) {
        if (mouseBtn->type() == core::EventType::MouseButtonPress) {
            if (expanded_) {
                // 检查是否点击了下拉列表中的项
                auto dropdownBounds = getDropdownBounds();
                float localY = mouseBtn->y() - globalBounds().top() - bounds().height();
                int itemIndex = getItemAtY(localY);
                
                if (itemIndex >= 0 && itemIndex < static_cast<int>(items_.size())) {
                    selectItem(itemIndex);
                }
                expanded_ = false;
            } else {
                toggleExpanded();
            }
            markDirty();
            event.handled = true;
            return true;
        }
    }
    
    if (auto* mouseMove = dynamic_cast<core::MouseMoveEvent*>(&event)) {
        if (expanded_) {
            auto dropdownBounds = getDropdownBounds();
            float localY = mouseMove->y() - globalBounds().top() - bounds().height();
            int newHoveredItem = getItemAtY(localY);
            
            if (newHoveredItem != hoveredItem_) {
                hoveredItem_ = newHoveredItem;
                markDirty();
            }
        }
    }
    
    if (auto* key = dynamic_cast<core::KeyEvent*>(&event)) {
        if (key->type() == core::EventType::KeyPress) {
            if (key->key() == core::Key::Enter || key->key() == core::Key::Space) {
                toggleExpanded();
                markDirty();
                event.handled = true;
                return true;
            } else if (key->key() == core::Key::Escape && expanded_) {
                expanded_ = false;
                markDirty();
                event.handled = true;
                return true;
            } else if (key->key() == core::Key::Up && expanded_ && selectedIndex_ > 0) {
                selectItem(selectedIndex_ - 1);
                event.handled = true;
                return true;
            } else if (key->key() == core::Key::Down && expanded_ && 
                       selectedIndex_ < static_cast<int>(items_.size()) - 1) {
                selectItem(selectedIndex_ + 1);
                event.handled = true;
                return true;
            }
        }
    }
    
    return Widget::onEvent(event);
}

Widget* Dropdown::hitTest(float x, float y)
{
    // 检查主按钮区域
    if (x >= 0 && x < bounds().width() && y >= 0 && y < bounds().height()) {
        return this;
    }
    
    // 检查下拉列表区域
    if (expanded_) {
        auto dropdownBounds = getDropdownBounds();
        float globalX = x + globalBounds().left();
        float globalY = y + globalBounds().top();
        
        if (dropdownBounds.contains(globalX, globalY)) {
            return this;
        }
    }
    
    return nullptr;
}

void Dropdown::toggleExpanded()
{
    expanded_ = !expanded_;
    if (!expanded_) {
        hoveredItem_ = -1;
    }
}

void Dropdown::selectItem(int index)
{
    setSelectedIndex(index);
    expanded_ = false;
    hoveredItem_ = -1;
}

int Dropdown::getItemAtY(float y) const
{
    if (y < 0) return -1;
    
    int index = static_cast<int>(y / kItemHeight);
    int visibleCount = std::min(static_cast<int>(items_.size()), maxVisibleItems_);
    
    if (index >= 0 && index < visibleCount) {
        return index;
    }
    return -1;
}

core::Rect Dropdown::getDropdownBounds() const
{
    int visibleCount = std::min(static_cast<int>(items_.size()), maxVisibleItems_);
    float dropdownHeight = visibleCount * kItemHeight;
    
    auto gb = globalBounds();
    return core::Rect::MakeXYWH(
        gb.left(),
        gb.top() + bounds().height(),
        bounds().width(),
        dropdownHeight
    );
}

}  // namespace tinalux::ui
