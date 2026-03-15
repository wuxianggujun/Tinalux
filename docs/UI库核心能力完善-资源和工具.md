# UI库核心能力完善 - 资源管理和开发工具

> 创建日期：2026-03-15

---

## 一、异步资源加载 ⭐⭐⭐

### 当前问题
- 图片加载阻塞主线程
- 无预加载机制

### 解决方案

```cpp
// include/tinalux/ui/ResourceLoader.h
template<typename T>
class ResourceHandle {
public:
    bool isReady() const;
    const T& get() const;
    void onReady(std::function<void(const T&)> callback);
};

class ResourceLoader {
public:
    static ResourceLoader& instance();
    ResourceHandle<rendering::Image> loadImageAsync(const std::string& path);
    void preload(const std::vector<std::string>& paths);
};
```

### 使用示例
```cpp
auto handle = ResourceLoader::instance().loadImageAsync("avatar.png");
handle.onReady([this](const auto& image) {
    this->image_ = image;
    this->markDirty();
});
```

---

## 二、多分辨率适配 ⭐⭐

```cpp
class ResourceManager {
public:
    void setDevicePixelRatio(float ratio);
    
    // 自动选择：icon.png / icon@2x.png / icon@3x.png
    rendering::Image getImage(const std::string& name);
};
```

---

## 三、热重载 ⭐

```cpp
class HotReload {
public:
    void enable();
    void watch(const std::string& path);
    void onResourceChanged(std::function<void(const std::string&)> callback);
};
```

---

## 四、Widget树可视化 ⭐⭐

```cpp
class WidgetInspector {
public:
    static WidgetInspector& instance();
    void enable();
    void showTree(Widget* root);
    void highlightWidget(Widget* widget);
    void showProperties(Widget* widget);
};
```

---

## 五、布局调试 ⭐⭐

```cpp
class LayoutDebugger {
public:
    void enableBoundsDisplay();    // 显示边界
    void enablePaddingDisplay();   // 显示padding
    void highlightLayoutIssues();  // 高亮布局问题
};
```

---

## 六、性能分析 ⭐⭐

```cpp
class Profiler {
public:
    void beginFrame();
    void endFrame();
    
    struct Stats {
        float layoutTime;
        float drawTime;
        int widgetCount;
        int drawCallCount;
    };
    
    Stats currentFrameStats() const;
    void exportTrace(const std::string& path);  // 导出火焰图
};

// RAII使用
class ProfileScope {
public:
    explicit ProfileScope(const std::string& name);
};
```

---

## 实施优先级

1. **第1周**：异步资源加载
2. **第2周**：多分辨率适配
3. **第3周**：Widget树可视化
4. **第4周**：性能分析器
5. **第5周**：热重载（可选）
