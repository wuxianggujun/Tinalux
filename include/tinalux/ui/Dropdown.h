#pragma once

#include <functional>
#include <string>
#include <vector>

#include "tinalux/ui/Widget.h"
#include "tinalux/core/Geometry.h"

namespace tinalux::ui {

/// 下拉菜单控件
/// 允许用户从列表中选择一个选项
class Dropdown : public Widget {
public:
    /// 构造函数
    /// @param items 选项列表
    explicit Dropdown(std::vector<std::string> items = {});
    
    /// 设置选项列表
    void setItems(const std::vector<std::string>& items);
    const std::vector<std::string>& items() const { return items_; }
    
    /// 设置选中项索引
    /// @param index 索引，-1表示未选中
    void setSelectedIndex(int index);
    int selectedIndex() const { return selectedIndex_; }
    
    /// 获取选中项文本
    /// @return 选中项文本，未选中返回空字符串
    std::string selectedItem() const;
    
    /// 设置选择改变回调
    /// @param handler 回调函数，参数为新选中的索引
    void onSelectionChanged(std::function<void(int)> handler);
    
    /// 设置最大可见项数（超出时显示滚动）
    void setMaxVisibleItems(int count);
    int maxVisibleItems() const { return maxVisibleItems_; }
    
    /// 设置占位符文本（未选中时显示）
    void setPlaceholder(const std::string& placeholder);
    const std::string& placeholder() const { return placeholder_; }
    
    // Widget接口实现
    bool focusable() const override { return true; }
    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;
    bool onEvent(core::Event& event) override;
    Widget* hitTest(float x, float y) override;
    
private:
    void toggleExpanded();
    void selectItem(int index);
    int getItemAtY(float y) const;
    core::Rect getDropdownBounds() const;
    
    std::vector<std::string> items_;
    std::string placeholder_ = "请选择...";
    int selectedIndex_ = -1;
    bool expanded_ = false;
    bool hovered_ = false;
    int hoveredItem_ = -1;
    int maxVisibleItems_ = 5;
    std::function<void(int)> onSelectionChanged_;
    
    static constexpr float kItemHeight = 32.0f;
    static constexpr float kMinWidth = 120.0f;
    static constexpr float kPadding = 8.0f;
    static constexpr float kIconSize = 12.0f;
};

}  // namespace tinalux::ui
