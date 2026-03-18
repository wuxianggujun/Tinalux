# UI 库核心能力完善：文本和动画

> 更新时间：2026-03-18  
> 状态：本文改为当前文本与动画能力说明。

## 文本能力现状

### 富文本

`RichTextWidget` 已实现，当前能力包括：

- `TextSpan`
- `RichTextBuilder`
- 链接点击
- 标题、说明文字、引用、代码块
- 项目符号列表和有序列表
- 行高倍率
- 对齐方式
- `maxLines`
- 行内背景色和删除线 / 下划线

当前可用构建器接口包括：

- `addText`
- `addParagraph`
- `addBold`
- `addItalic`
- `addHeading`
- `addCaption`
- `addQuote`
- `addCode`
- `addCodeBlock`
- `addBulletItem`
- `addOrderedItem`
- `addLink`

### 文本输入

`TextInput` 已支持：

- UTF-8 文本编辑
- 光标移动
- 选择与拖拽选择
- `Ctrl+A`
- 密码遮蔽
- IME 组合态
- 剪贴板
- 前后缀图标
- 异步图标加载

因此“富文本未实现”“IME 未接通”这类旧说法都不再准确。

## 动画能力现状

动画系统已具备：

- `AnimationSink`
- `AnimationScheduler`
- 补间动画
- `requestAnimationFrame`
- 缓动函数
- 主循环接线
- 主题切换动画

并且已被多个控件实际使用。

## 示例

### RichText

```cpp
using namespace tinalux::ui;

auto widget = std::make_shared<RichTextWidget>(
    RichTextBuilder()
        .addHeading("Release Notes")
        .addParagraphBreak()
        .addText("Current backend: ")
        .addBold("Vulkan")
        .addText(" / ")
        .addLink("Open docs", [] {})
        .build());
```

### 动画

```cpp
application.animationSink().animate(
    {
        .from = 0.0f,
        .to = 1.0f,
        .durationSeconds = 0.2,
        .easing = tinalux::ui::Easing::EaseOutCubic,
    },
    [](float value) {
        (void)value;
    });
```

## 当前缺口

下面这些能力目前仍未形成统一框架：

- 页面切换动画
- 布局变化动画
- 动画编排 / 时间轴
- 跨 Widget 文本选择
- 更高级的文本编辑控件族

## 当前建议

- 文档中把富文本定位为“已实现并可用”，不要再当作规划项
- 动画系统应描述为“已实现并已接入部分控件”，不要再写成“只有调度器，没有实际使用”
