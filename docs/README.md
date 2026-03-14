# Tinalux

跨平台 Skia 自绘 UI 框架

## 文档

| 文档 | 内容 |
|------|------|
| [项目概述](./项目概述.md) | 项目定位、技术栈、设计参考、当前状态 |
| [架构设计](./架构设计.md) | 模块划分、依赖关系、Constraints 布局、三阶段事件、完整 API |
| [设计规则](./设计规则.md) | 架构约束、Skia 类型隔离规则、编码规范、游戏引擎集成预留 |
| [开发计划](./开发计划.md) | 6 阶段路线图 |
| [代码审查报告](./代码审查报告.md) | Bug 列表、性能问题、修复优先级 |
| [目录结构规划](./目录结构规划.md) | 文件结构和 CMake 配置 |

## 架构

```
              ┌→ Platform ──→ Core
App (胶水) ───┼→ Rendering ──→ Core + Skia
              └→ UI ─────────→ Core + Skia
```

模块间互不依赖，App 是唯一连接点。

## 核心设计

| 机制 | 方案 | 参考 |
|------|------|------|
| 布局 | BoxConstraints 约束协商（两遍：measure + arrange） | Flutter / Druid |
| 事件 | 捕获 → 目标 → 冒泡 三阶段 | Chromium / GTK4 |
| 渲染 | Skia SkCanvas 直出 + 脏标记按需重绘 | LVGL |
| Widget | 5 方法接口（measure / arrange / onEventCapture / onEvent / onDraw） | Druid |

## 构建

```bash
scripts/syncSkia.sh
cmake -B build -S .
cmake --build build
```
