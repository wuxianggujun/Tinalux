# UI库核心能力完善 - 文本和动画系统

> 创建日期：2026-03-15

---

## 一、富文本系统 ⭐⭐⭐

### 当前问题
- 无富文本支持（不同颜色、字体混排）
- 无超链接支持
- 无跨Widget文本选择

### 解决方案

```cpp
// include/tinalux/ui/RichText.h
struct TextSpan {
    std::string text;
    std::optional<core::Color> color;
    std::optional<float> fontSize;
    bool bold = false;
    bool italic = false;
    bool underline = false;
    std::function<void()> onClick;  // 超链接
};

class RichTextBuilder {
public:
    RichTextBuilder& addText(const std::string& text);
    RichTextBuilder& addBold(const std::string& text);
    RichTextBuilder& addColored(const std::string& text, core::Color color);
    RichTextBuilder& addLink(const std::string& text, std::function<void()> onClick);
    std::vector<TextSpan> build() const;
};

class RichTextWidget : public Widget {
public:
    explicit RichTextWidget(const std::vector<TextSpan>& spans);
    void setSpans(const std::vector<TextSpan>& spans);
};
```

### 使用示例
```cpp
auto richText = std::make_shared<RichTextWidget>(
    RichTextBuilder()
        .addText("普通文本，")
        .addBold("粗体，")
        .addColored("红色", core::colorRGB(255, 0, 0))
        .addLink("链接", []{ /* 点击处理 */ })
        .build()
);
```

---

## 二、字形缓存 ⭐⭐

```cpp
// src/rendering/text/GlyphCache.h
class GlyphCache {
public:
    static GlyphCache& instance();
    rendering::Image getGlyph(uint32_t codepoint, float fontSize);
    void setMaxCacheSize(size_t bytes);
    float hitRate() const;
};
```

---

## 三、布局动画 ⭐⭐

### 当前问题
- 添加/删除Widget无过渡动画
- 布局变化生硬

### 解决方案

```cpp
// include/tinalux/ui/LayoutAnimation.h
class LayoutAnimation {
public:
    enum class Type { None, Fade, Slide, Scale };
    
    static void enable(Container* container, Type type = Type::Fade);
    void setDuration(float seconds);
};
```

### 使用示例
```cpp
auto container = std::make_shared<Container>();
LayoutAnimation::enable(container.get(), LayoutAnimation::Type::Slide);

container->addChild(newWidget);  // 自动播放滑入动画
```

---

## 四、页面切换动画 ⭐⭐

```cpp
class PageNavigator : public Container {
public:
    enum class Transition { Slide, Fade, Zoom };
    
    void push(std::shared_ptr<Widget> page, Transition transition = Transition::Slide);
    void pop(Transition transition = Transition::Slide);
    Widget* currentPage() const;
};
```

---

## 五、动画编排 ⭐

```cpp
class AnimationSequence {
public:
    AnimationSequence& then(std::shared_ptr<Animation> anim);  // 串行
    AnimationSequence& with(std::shared_ptr<Animation> anim);  // 并行
    AnimationSequence& delay(float seconds);
    AnimationSequence& repeat(int count);
    void start();
};

// 使用
AnimationSequence()
    .then(fadeIn)
    .delay(0.5)
    .with(slideUp).with(scaleUp)  // 同时执行
    .then(fadeOut)
    .start();
```

---

## 实施优先级

1. **第1-2周**：富文本系统
2. **第3周**：字形缓存
3. **第4周**：布局动画
4. **第5周**：页面切换和动画编排
