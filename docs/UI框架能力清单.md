# Tinalux UI 框架能力清单

> 更新时间：2026-03-18  
> 说明：本清单只记录当前源码已实现、部分实现和明确未完成的能力，不再使用早期“完成度百分比”写法。

## 已实现

### 核心架构

- `Application + UIContext` 的分层结构
- 公共渲染抽象：`Canvas / RenderContext / RenderSurface / Image`
- `BackendPlan` 后端候选与 fallback
- `RuntimeHooks` 测试注入点

### 渲染

- OpenGL 后端
- Vulkan 后端
- `Backend::Auto`
- 窗口 surface
- 离屏 raster surface
- 图片加载与 RGBA 创建
- 字体缓存
- 图片缓存
- 组件级渲染缓存（`Panel`）

### 布局

- `Constraints`
- `VBoxLayout`
- `HBoxLayout`
- `FlexLayout`
- `GridLayout`
- `ResponsiveLayout`

### 事件与输入

- capture → target → bubble
- hit test
- overlay 优先命中
- 焦点管理
- hover 追踪
- 鼠标捕获
- 键盘事件
- 文本输入事件
- IME 组合态事件

### 主题与样式

- `Theme`
- `ThemeManager`
- `ColorScheme`
- `Typography`
- `Spacing`
- typed style structs：
  - `ButtonStyle`
  - `TextInputStyle`
  - `SelectionControlStyle`
  - `SliderStyle`
  - `ScrollViewStyle`
  - `DialogStyle`
  - `PanelStyle`
  - `ListViewStyle`
  - `RichTextStyle`
- 主题切换动画
- 主题偏好持久化

### 文本与控件

- `Label`
- `ParagraphLabel`
- `RichTextWidget`
- `Button`
- `Checkbox`
- `Radio`
- `Toggle`
- `Slider`
- `ScrollView`
- `ListView`
- `TextInput`
- `ImageWidget`
- `Dialog`
- `Dropdown`
- `ProgressBar`

### 资源

- `IconRegistry`
- `ResourceManager`
- `ResourceLoader`
- 多分辨率图片路径解析
- 异步图片加载与回调

### Android 相关

- `AndroidWindow`
- `AndroidRuntime`
- C ABI / JNI
- `android/host`
- `android/tinalux-sdk`
- `android/validation-app`

## 部分实现 / 需要验证

### 平台

- Linux/X11：代码链路已接通，但缺少真机闭环验证
- Android：运行时和宿主已接线，但缺少真机渲染、输入、生命周期验证

### 动画

- 调度器与补间已实现
- 部分控件已集成 hover / focus 动画：
  - `Button`
  - `Checkbox`
  - `Radio`
  - `Toggle`
  - `Slider`
  - `TextInput`
- 页面切换动画、布局动画还没有形成统一框架

### 测试

- 源码定义的 smoke target 数量已经扩展到 `46`
- 本地 `cmake-build-debug` 的 `ctest -N` 只登记到 `44`
- 还存在测试可执行文件缺失，说明构建目录未收敛

## 明确未完成

### 渲染

- `Metal` 正式后端

### 输入与交互

- 手势系统
- 触摸语义层
- 无障碍

### 开发者工具

- Widget Inspector
- 热重载
- 布局调试器 UI
- 可视化性能分析工具

## 当前更值得继续投入的方向

- 构建与测试清单收敛
- Android 真机验证
- Linux/X11 真机验证
- Metal 后端
- 更完整的移动端输入与手势能力
