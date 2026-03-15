# UI库核心能力完善 - 样式和布局系统

> 创建日期：2026-03-15

---

## 一、样式系统 ⭐⭐⭐

### 当前问题
- 只有全局Theme，无法为单个控件定制样式
- 无样式继承机制
- 主题切换无过渡动画

### 解决方案

```cpp
// include/tinalux/ui/Style.h
class Style {
public:
    std::optional<core::Color> backgroundColor;
    std::optional<core::Color> foregroundColor;
    std::optional<float> cornerRadius;
    std::optional<float> padding;
    std::optional<float> fontSize;
    
    Style merge(const Style& other) const;
};

class StyleSelector {
public:
    enum class State { Normal, Hovered, Pressed, Focused, Disabled };
    void setStyle(State state, Style style);
    const Style& getStyle(State state) const;
};

// Widget添加
class Widget {
    void setStyle(Style style);
    void setStyleSelector(std::shared_ptr<StyleSelector> selector);
};
```

### 使用示例
```cpp
auto button = std::make_shared<Button>("Custom");
Style customStyle;
customStyle.backgroundColor = core::colorRGB(255, 87, 34);
customStyle.cornerRadius = 20.0f;
button->setStyle(customStyle);
```

---

## 二、FlexLayout ⭐⭐⭐

### 当前问题
- 只有VBox/HBox，无法灵活分配空间
- 无法实现复杂的响应式布局

### 解决方案

```cpp
// include/tinalux/ui/FlexLayout.h
enum class FlexDirection { Row, Column };
enum class FlexJustify { Start, End, Center, SpaceBetween, SpaceAround };
enum class FlexAlign { Start, End, Center, Stretch };

class FlexLayout : public Layout {
public:
    void setDirection(FlexDirection direction);
    void setJustifyContent(FlexJustify justify);
    void setAlignItems(FlexAlign align);
    void setGap(float gap);
    
    void setFlexGrow(Widget* child, float grow);
    void setFlexShrink(Widget* child, float shrink);
};
```

### 使用示例
```cpp
auto flex = std::make_unique<FlexLayout>();
flex->setDirection(FlexDirection::Row);
flex->setJustifyContent(FlexJustify::SpaceBetween);
flex->setGap(12.0f);

// 子项1占1份，子项2占2份空间
flex->setFlexGrow(child1.get(), 1.0f);
flex->setFlexGrow(child2.get(), 2.0f);
```

---

## 三、GridLayout ⭐⭐

```cpp
class GridLayout : public Layout {
public:
    void setColumns(int columns);
    void setRows(int rows);
    void setColumnGap(float gap);
    void setRowGap(float gap);
    
    void setSpan(Widget* child, int colSpan, int rowSpan);
    void setPosition(Widget* child, int col, int row);
};
```

---

## 四、响应式布局 ⭐⭐

```cpp
class ResponsiveLayout : public Layout {
public:
    void addBreakpoint(float minWidth, std::unique_ptr<Layout> layout);
};

// 使用
auto responsive = std::make_unique<ResponsiveLayout>();
responsive->addBreakpoint(0, std::make_unique<VBoxLayout>());      // < 600
responsive->addBreakpoint(600, std::make_unique<HBoxLayout>());    // >= 600
responsive->addBreakpoint(1200, std::make_unique<GridLayout>());   // >= 1200
```

---

## 实施优先级

1. **第1-2周**：样式系统
2. **第3-4周**：FlexLayout
3. **第5周**：GridLayout
4. **第6周**：响应式布局
