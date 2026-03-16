# Tinalux

跨平台 Skia 自绘 UI 框架

## 最新状态

- [源码核对-当前状态-2026-03-16](./源码核对-当前状态-2026-03-16.md) - 基于源码与构建产物的最新状态说明，优先于历史报告
- 多后端渲染已实现：OpenGL + Vulkan 完整可用，含自动 fallback 机制
- RichText 展示层已具备主题化 block token、本地 style override、多级列表、全局 `maxLines` 与 DemoScene 文档展示卡片

> 说明：`docs` 下带日期的“进度报告/完整状态报告”属于历史快照，可能与最新源码不完全一致。

## 文档

### 核心文档
| 文档 | 内容 |
|------|------|
| [项目概述](./项目概述.md) | 项目定位、技术栈、设计参考、当前状态 |
| [架构设计](./架构设计.md) | 模块划分、依赖关系、Constraints 布局、三阶段事件、完整 API |
| [设计规则](./设计规则.md) | 架构约束、Skia 类型隔离规则、编码规范、游戏引擎集成预留 |
| [开发计划](./开发计划.md) | 7 阶段路线图 |
| [代码审查报告](./代码审查报告.md) | Bug 列表、性能问题、修复优先级 |
| [目录结构规划](./目录结构规划.md) | 文件结构和 CMake 配置 |

### UI库核心能力完善
- [样式和布局](UI库核心能力完善-样式和布局.md) - FlexLayout、GridLayout、响应式布局
- [文本和动画](UI库核心能力完善-文本和动画.md) - 富文本、IME、控件动画
- [资源和工具](UI库核心能力完善-资源和工具.md) - 资源管理、开发工具
- [动画系统使用指南](动画系统使用指南.md) - 动画系统详细文档
- [主题系统设计与实现](主题系统设计与实现.md) - 主题系统详细文档
- [UI框架能力清单](UI框架能力清单.md) - 完整功能清单和优先级

### 改进文档
- [代码改进分析报告-2026-03-15](代码改进分析报告-2026-03-15.md) - 详细改进分析
- [实施建议-优先级清单](实施建议-优先级清单.md) - P0-P3优先级任务
- [快速改进指南](快速改进指南.md) - 快速修复指南
- [改进工作总结-2026-03-15](改进工作总结-2026-03-15.md) - 改进成果总结

### 进度报告
- [源码核对-当前状态-2026-03-16](源码核对-当前状态-2026-03-16.md) - 最新源码核对结论
- [修复进度报告-2026-03-15](修复进度报告-2026-03-15.md) - 当前修复状态
- [项目完整状态报告-2026-03-15](项目完整状态报告-2026-03-15.md) - 项目完整状态总结
- [文档更新说明-2026-03-15](文档更新说明-2026-03-15.md) - 文档更新记录

### 平台移植
- [Android平台多界面架构方案](Android平台多界面架构方案.md) - Android移植架构设计
- [Android平台实现进展-2026-03-17](Android平台实现进展-2026-03-17.md) - Android 平台骨架实现进度

## 架构

```
              ┌→ Platform ──→ Core
App (胶水) ───┼→ Rendering ──→ Core + Skia
              └→ UI ─────────→ Core + Skia
```

模块间互不依赖，App 是唯一连接点。

### 多后端渲染

```
rendering::createContext(ContextConfig)
    ├─ Backend::OpenGL  → Skia Ganesh GL     ✅ 完整实现
    ├─ Backend::Vulkan  → Skia Ganesh Vulkan ✅ 完整实现
    ├─ Backend::Metal   → (待实现)            ⏳
    └─ Backend::Auto    → 按平台优先级自动选择 + fallback
```

### 多平台支持

```
Window 接口 (平台抽象层)
    ├─ GLFWWindow (Windows/macOS/Linux) - 使用 tina_glfw
    │   ├─ OpenGL 窗口 (GLFW_OPENGL_API)
    │   └─ Vulkan 窗口 (GLFW_NO_API + Vulkan surface)
    └─ AndroidWindow (Android) - 原生 ANativeWindow + EGL (待实现)
```

**当前平台**：
- ✅ Windows - 完整支持 (GLFW + IME + OpenGL/Vulkan)
- ✅ macOS - 完整支持 (GLFW + Cocoa文本输入 + OpenGL/Vulkan)
- ⚠️ Linux/X11 - 代码链路已接通，但尚未做真机验证
- ⏳ Android - 架构设计完成，待实现

**依赖库**：
- **tina_glfw（基于 GLFW 3.5.0）** - zlib/libpng 许可证
  - 桌面平台窗口管理
  - 作为仓库内 `3rdparty/tina_glfw` 正式源码依赖固定
- **Skia** - BSD 3-Clause 许可证
  - 2D图形渲染引擎
- **spdlog** - MIT 许可证
  - 日志系统

## 核心设计

| 机制 | 方案 | 参考 |
|------|------|------|
| 布局 | BoxConstraints 约束协商（两遍：measure + arrange） | Flutter / Druid |
| 事件 | 捕获 → 目标 → 冒泡 三阶段 | Chromium / GTK4 |
| 渲染 | 多后端（OpenGL/Vulkan）+ Skia Canvas 直出 + 脏标记按需重绘 + 自动 fallback | LVGL |
| Widget | 5 方法接口（measure / arrange / onEventCapture / onEvent / onDraw） | Druid |

## 构建

```bash
scripts/syncSkia.sh
cmake -B build -S .
cmake --build build
```
