#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "tinalux/rendering/rendering.h"
#include "tinalux/ui/Widget.h"
#include "tinalux/core/Geometry.h"
#include "tinalux/ui/IconRegistry.h"
#include "tinalux/ui/ResourceLoader.h"

namespace tinalux::ui {

enum class DropdownIndicatorLoadState {
    Idle,
    Loading,
    Ready,
    Failed,
};

/// 下拉菜单控件
/// 允许用户从列表中选择一个选项
class Dropdown : public Widget {
public:
    /// 构造函数
    /// @param items 选项列表
    explicit Dropdown(std::vector<std::string> items = {});
    ~Dropdown() override;
    
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

    void setIndicatorIcon(rendering::Image icon);
    void setIndicatorIcon(IconType type, float sizeHint = 0.0f);
    const rendering::Image& indicatorIcon() const;
    void loadIndicatorIconAsync(const std::string& path);
    const std::string& indicatorIconPath() const;
    bool indicatorIconLoading() const;
    DropdownIndicatorLoadState indicatorIconLoadState() const;
    
    // Widget接口实现
    bool focusable() const override { return true; }
    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;
    bool onEvent(core::Event& event) override;
    Widget* hitTest(float x, float y) override;
    
private:
    void applyRequestedSelection(bool emitCallback);
    void toggleExpanded();
    void selectItem(int index);
    int getItemAtY(float y) const;
    core::Rect getDropdownBounds() const;
    
    std::vector<std::string> items_;
    std::string placeholder_ = "请选择...";
    int requestedSelectedIndex_ = -1;
    int selectedIndex_ = -1;
    bool expanded_ = false;
    bool hovered_ = false;
    int hoveredItem_ = -1;
    rendering::Image indicatorIcon_;
    ResourceHandle<rendering::Image> pendingIndicatorIcon_;
    std::shared_ptr<bool> indicatorIconLoadAlive_ = std::make_shared<bool>(true);
    std::shared_ptr<std::uint64_t> indicatorIconLoadGeneration_ = std::make_shared<std::uint64_t>(0);
    std::string indicatorIconPath_;
    bool indicatorIconLoading_ = false;
    DropdownIndicatorLoadState indicatorIconLoadState_ = DropdownIndicatorLoadState::Idle;
    int maxVisibleItems_ = 5;
    std::function<void(int)> onSelectionChanged_;
};

}  // namespace tinalux::ui
