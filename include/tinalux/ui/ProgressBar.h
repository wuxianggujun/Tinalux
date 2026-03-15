#pragma once

#include "tinalux/ui/Widget.h"
#include "tinalux/core/Geometry.h"

namespace tinalux::ui {

/// 进度条控件
/// 支持确定进度和不确定进度两种模式
class ProgressBar : public Widget {
public:
    /// 构造函数
    /// @param min 最小值
    /// @param max 最大值
    explicit ProgressBar(float min = 0.0f, float max = 100.0f);
    
    /// 设置当前进度值
    /// @param value 进度值，会被自动限制在[min, max]范围内
    void setValue(float value);
    
    /// 获取当前进度值
    float value() const { return value_; }
    
    /// 设置进度范围
    /// @param min 最小值
    /// @param max 最大值
    void setRange(float min, float max);
    
    float min() const { return min_; }
    float max() const { return max_; }
    
    /// 设置是否为不确定进度模式（显示动画而非具体进度）
    void setIndeterminate(bool indeterminate);
    bool indeterminate() const { return indeterminate_; }
    
    /// 设置进度条高度
    void setHeight(float height);
    float height() const { return height_; }
    
    /// 设置进度条颜色
    void setColor(core::Color color);
    core::Color color() const { return color_; }
    
    /// 设置背景颜色
    void setBackgroundColor(core::Color bgColor);
    core::Color backgroundColor() const { return backgroundColor_; }
    
    // Widget接口实现
    core::Size measure(const Constraints& constraints) override;
    void onDraw(rendering::Canvas& canvas) override;
    
private:
    float min_ = 0.0f;
    float max_ = 100.0f;
    float value_ = 0.0f;
    float height_ = 8.0f;
    bool indeterminate_ = false;
    core::Color color_ = core::colorRGB(33, 150, 243);  // Material Blue
    core::Color backgroundColor_ = core::colorRGB(200, 200, 200);
};

}  // namespace tinalux::ui
