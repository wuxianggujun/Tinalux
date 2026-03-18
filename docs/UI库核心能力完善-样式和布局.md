# UI 库核心能力完善：样式和布局

> 更新时间：2026-03-18  
> 状态：原文中的多数“待实现方案”已经落地，本文改为当前实现说明。

## 布局系统现状

### 基础布局

已实现：

- `VBoxLayout`
- `HBoxLayout`
- `Constraints`

特点：

- 两阶段布局：`measure -> arrange`
- 基础布局包含 `spacing` 和 `padding`
- `VBoxLayout` / `HBoxLayout` 还带有 measure cache

### FlexLayout

当前 `FlexLayout` 已实现，不再是设计稿。

可用能力：

- `direction`
  - `Row`
  - `RowReverse`
  - `Column`
  - `ColumnReverse`
- `justifyContent`
  - `Start`
  - `Center`
  - `End`
  - `SpaceBetween`
  - `SpaceAround`
- `alignItems`
  - `Start`
  - `Center`
  - `End`
  - `Stretch`
- `wrap`
  - `NoWrap`
  - `Wrap`
- `setFlex(child, grow, shrink)`

示例：

```cpp
auto layout = std::make_unique<tinalux::ui::FlexLayout>();
layout->direction = tinalux::ui::FlexDirection::Row;
layout->justifyContent = tinalux::ui::JustifyContent::SpaceBetween;
layout->alignItems = tinalux::ui::AlignItems::Center;
layout->spacing = 12.0f;
layout->setFlex(leftChild.get(), 1.0f, 1.0f);
layout->setFlex(rightChild.get(), 2.0f, 1.0f);
```

### GridLayout

当前 `GridLayout` 已实现。

可用能力：

- 设置行列数
- 指定子项位置
- 指定 span
- 行间距 / 列间距 / padding

示例：

```cpp
auto layout = std::make_unique<tinalux::ui::GridLayout>();
layout->setColumns(3);
layout->setRows(2);
layout->columnGap = 12.0f;
layout->rowGap = 8.0f;
layout->setPosition(card.get(), 1, 0);
layout->setSpan(card.get(), 2, 1);
```

### ResponsiveLayout

当前 `ResponsiveLayout` 已实现，用于按宽度切换子布局。

可用能力：

- `addBreakpoint(minWidth, layout)`
- `clearBreakpoints()`

更准确的描述应是：

- 已支持按 breakpoint 选择布局
- 但还不是一个完整的响应式样式系统

## 样式系统现状

### 已有能力

当前样式体系由三部分组成：

- `Theme`
- `typed style struct`
- `StateStyle<T>`

`StateStyle<T>` 已覆盖：

- `normal`
- `hovered`
- `pressed`
- `disabled`
- `focused`

这意味着当前框架已经具备状态样式，而不是“只有一个全局 Theme”。

### 当前不是通用样式引擎

下面这些说法现在依然不准确：

- “有类似 CSS 的通用 Style 类”
- “所有 Widget 都有统一的 setStyle/merge/inherit 机制”
- “已经存在样式级联与选择器”

目前真实情况是：

- 主题和控件样式 token 已存在
- 但体系是强类型、按控件拆分的

## 仍未完成的部分

- 通用样式继承规则
- 通用样式解析优先级系统
- 布局动画
- 页面级布局切换框架

## 当前建议

- 文档里应直接使用真实 API 名称：`JustifyContent`、`AlignItems`、`FlexWrap`
- 不再保留早期虚构的 `Style` / `StyleSelector` 方案代码
- 若未来真的要做统一样式语言，应另开设计文档，而不是继续混用当前 typed style 与早期草案
