# Canvas 收口阶段交付

## 背景

这一阶段的目标，是把 Tinalux 的公开 UI / Rendering API
从 Skia 类型和原生画布接口中抽离出来，为后续继续做渲染后端解耦、
接入自研引擎、补充调试能力和做跨后端实现铺平路径。

本阶段不追求一次性替换底层 Skia 实现，而是先完成两件更重要的事：

1. 公开头文件不再泄漏 Skia 关键类型。
2. 控件和应用逻辑尽量只依赖自有 `Canvas` 和几何类型。

## 本阶段已完成

### 1. 公共几何类型已脱离 Skia 头

- 公开几何类型统一收敛到 `tinalux::core::Geometry`。
- `Geometry.h` 已经是纯值类型头，不再直接包含 Skia 头。
- Skia 几何转换被压回内部适配层：
  - `SkiaGeometry.h`

### 2. 公开渲染句柄已改为自有包装

- `rendering.h` 公开的是：
  - `RenderContext`
  - `RenderSurface`
  - `Canvas`
- 公开层不再暴露 `GrDirectContext`、`SkSurface`、`SkCanvas*`。
- Skia 原生句柄只保留在内部桥接层：
  - `RenderHandles.h`

### 3. 公开绘制入口已切到 `Canvas`

- `Widget::draw(...)`
- `Widget::onDraw(...)`
- 各控件公开头

以上入口已经统一改为依赖 `rendering::Canvas`。

### 4. 普通控件绘制已基本迁移

以下实现已经主要使用 `Canvas` 方法完成绘制：

- `Button`
- `Checkbox`
- `Label`
- `Panel`
- `Radio`
- `ScrollView`
- `Slider`
- `TextInput`
- `Toggle`
- `UIContext` 内调试 HUD

### 5. 段落绘制特例已压回渲染内部

`ParagraphLabel` 仍然依赖 SkParagraph，但它不再直接拿原生 `SkCanvas`。

新增内部 helper：

- `ParagraphPainter.h`
- `ParagraphPainter.cpp`

现在由渲染内部负责把 `Canvas` 映射到 SkParagraph 所需的原生画布，
UI 控件层只关心“绘制段落”这一动作。

### 6. 文本度量实现已从头文件剥离

- `TextPrimitives.h` 现在只保留 `TextMetrics` 和度量函数声明。
- `TextPrimitives.cpp` 现在只负责缓存和门面。
- `SkFont`、UTF-8 统计和真实度量逻辑已经迁到 `TextMetricsBackend.cpp`。

这一步的价值在于：

- UI 头文件不再因为文本度量而传播 Skia 字体头依赖
- 文本缓存从“每个编译单元各一份”收敛成了统一实现
- 后续替换文本度量后端时，改动点会集中在 backend 实现

### 7. 测试侧已经有正式的离屏渲染入口

- 渲染公开层新增 `createRasterSurface(...)`
- 文本输入和段落相关 smoke test 已切到这个入口

这一步的价值在于：

- 测试不再直接 include 内部 `RenderHandles`
- 测试不再直接依赖 `SkSurface` / `SkImageInfo` 创建离屏画布
- 离屏绘制路径也开始走正式的公开渲染抽象

### 8. 图像抽象和图片控件已落地

- 渲染公开层新增 `Image`
- `Canvas` 现在支持 `drawImage(...)`
- UI 层新增 `ImageWidget`
- `Button` 已支持可选图标与起止布局
- 文件加载路径已经有进程内缓存
- 公开层已支持编码字节直接解码
- 已提供显式缓存清理入口

这一步的价值在于：

- 图片显示已经能走正式的公开渲染抽象
- UI 组件库不再只停留在文本、输入和基础容器
- 图标开始接入真实交互控件，而不只是独立图片控件
- 图片文件重复加载已经开始具备资源复用能力
- 非文件来源的图片加载路径也已经存在
- 后续补图标系统和更完整资源管理时，已经有稳定入口

### 9. Overlay 顶层交互层已落地

- `UIContext` 现在支持独立的 overlay tree
- Overlay 会在 root tree 之后绘制
- Overlay 会优先参与命中测试和焦点遍历
- 基于 Overlay 的最小 `Dialog` 组件已落地

这一步的价值在于：

- 弹窗、浮层、下拉面板已经有正式挂载点
- 顶层交互不需要再硬塞回普通布局树
- `Dialog` 已经能作为真实弹窗场景使用
- 为后续 Popup / Dropdown 组件提供了真实基础

### 10. 最小组件级渲染缓存已落地

- `RenderSurface` 现在支持 `snapshotImage()`
- `Panel` 现在支持可选离屏缓存
- 已有 smoke test 验证缓存命中和失效

这一步的价值在于：

- 静态或低频变化容器已经能作为最小 Repaint Boundary 使用
- 下一帧可以直接复用缓存图像，而不是重绘整棵子树
- 为后续更通用的 Render Caching 机制探路

## 当前边界

当前架构可以概括成：

```text
UI / App 逻辑
    ↓ 仅依赖
core::Geometry + rendering::Canvas
    ↓ 内部桥接
Rendering Internal Adapter
    ↓ 当前后端
Skia
```

这意味着：

- **公开 API 已经基本脱离 Skia**
- **实现层仍然允许内部使用 Skia**
- **后续替换后端时，主要改动会集中在 Rendering 内部**

## 验证方式

本阶段提交前，使用以下命令串行验证：

```bash
cmake --build build-codex --config Debug
ctest --test-dir build-codex -C Debug --output-on-failure
```

之所以强调串行，是因为 Windows / MSBuild 环境下，增量链接和测试可执行文件
偶尔会出现瞬时文件占用或链接异常；串行执行更稳定。

## 已知剩余热点

以下部分仍然是下一阶段的重点，不属于这次交付的 unfinished bug，
而是后续架构演进目标：

### 1. 段落布局仍依赖 SkParagraph

- `ParagraphLabel.cpp`

虽然它已经不再直接拿原生画布，但段落构建和 layout 仍然由 SkParagraph 提供。
这是可接受的阶段性状态。

### 2. 文本度量后端仍由 Skia 提供

- `TextMetricsBackend.cpp`

虽然依赖已经被压到了实现文件里，但当前文本度量的真实后端仍然是 `SkFont`。
如果后面要支持多后端或自研引擎文本系统，这里还需要继续抽象。

### 3. 离屏 surface 仍由 Skia 在内部承载

- `RasterSurface.cpp`

虽然测试已经改走 `createRasterSurface(...)`，但当前离屏 surface 的真实实现
仍然由 `SkSurface` 承载。后续如果继续抽象渲染后端，这里仍然是一个集中改动点。

### 4. 图片资源生命周期还比较基础

- `Image.cpp`
- `ImageWidget.cpp`

当前已经有最小可用的图像创建、文件加载缓存和绘制能力，但还没有统一异步加载、
缓存淘汰策略或更完整的图片适配策略。这属于功能增强阶段的下一步工作。

### 5. Overlay 还缺少现成组件

- `UIContext.cpp`

虽然 Overlay 层和最小 `Dialog` 已经可用，但目前还没有现成的 `PopupMenu`、
`Dropdown` 等上层组件，仍需要在这个基础上继续往上封装。

### 6. 组件级缓存目前只覆盖最小容器场景

- `Panel.cpp`

当前缓存能力已经能覆盖静态 Panel 这类高收益场景，但还没有扩展到更通用的
Widget / Container 粒度，也还没有缓存淘汰和统计面板。

## 建议时间框架

如果按当前节奏持续推进，建议按下面的节奏安排：

### 第 1 周

- 继续梳理文本度量后端的最小抽象边界
- 盘点离屏 surface 需要暴露到什么程度才算“最小公开接口”
- 继续减少实现层对内部桥接细节的直接依赖

### 第 2 周

- 继续扩展 `Canvas` 能力
- 评估 `ParagraphLabel` 的进一步抽象方式
- 为图片资源增加缓存淘汰与更完整加载策略
- 基于 Overlay 层继续封装 Popup / Dropdown
- 将渲染缓存从 Panel 推广到更通用容器
- 开始梳理可替换渲染后端所需的最小接口集合

## 下一阶段建议

下一阶段优先级建议如下：

1. 抽象文本测量后端，继续压缩实现层对 Skia 字体 API 的直接依赖。
2. 盘点 `Canvas` 当前缺失的图元与文本能力，形成最小后端接口清单。
3. 给 `Image` 增加缓存淘汰和更完整的加载路径。
4. 基于 Overlay 层补 Popup / Dropdown 组件。
5. 将渲染缓存从 Panel 推广到更通用容器。
6. 继续收紧离屏 surface / native handle 的内部边界。

## 结论

这一阶段结束后，Tinalux 已经不再只是“把 Skia 包一层名字”。
现在的公开边界已经开始具备真正的后端抽象价值。

后续工作重点应该继续放在：

- 文本测量抽象
- 测试辅助设施
- 渲染后端最小接口整理

而不是再回头保留旧兼容接口。
