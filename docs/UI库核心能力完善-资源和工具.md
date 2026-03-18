# UI 库核心能力完善：资源和工具

> 更新时间：2026-03-18  
> 状态：资源能力已初步落地；开发者工具仍明显不足。

## 资源能力现状

### ResourceManager

当前 `ResourceManager` 已支持：

- 设置设备像素比
- 搜索路径管理
- 根据 DPI 解析资源路径
- 自动尝试 `@2x / @3x`

这意味着“多分辨率适配完全没有实现”已经不准确。

### ResourceLoader

当前 `ResourceLoader` 已支持：

- 异步图片加载
- worker 线程处理
- in-flight 去重
- `preload`
- 主线程 pump 回调
- `clear`

适用场景：

- `TextInput` 图标异步加载
- `ImageWidget` 异步图片加载
- 其他控件的图片资源预热

### IconRegistry

当前 `IconRegistry` 已支持三类注册方式：

- 路径
- 编码后的字节数据
- 工厂函数

## 当前已有的工具化能力

### Debug HUD / 性能摘要

应用层当前已有：

- `FrameStats`
- 周期性性能日志
- Debug HUD

这部分能力存在于 `Application` / `UIContext`，不应继续写成“完全没有开发者工具”。

### 渲染缓存与基础性能设施

当前已有：

- 字体缓存
- 图片缓存
- 组件级渲染缓存
- 异步资源回调泵送

## 当前缺失的工具

下面这些工具仍然没有：

- Widget Inspector
- 资源热重载
- 布局调试器 UI
- 可视化 trace / flame graph 导出工具

所以更准确的描述是：

- 资源层基础设施已经初具规模
- 开发者可视化工具仍基本缺失

## 推荐文档表述

- `ResourceManager`：已实现基础资源解析和 DPI 适配
- `ResourceLoader`：已实现异步图片加载，但不是完整通用资源系统
- `IconRegistry`：已实现注册与按需获取
- 开发者工具：仅有 Debug HUD 和性能摘要，暂无 inspector / hot reload
