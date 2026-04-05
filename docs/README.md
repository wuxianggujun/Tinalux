# Tinalux 文档索引

> 更新时间：2026-04-05
> 说明：本目录只保留面向用户和贡献者的项目文档。阶段汇报、审查、计划、进度记录等过程性文档已移除。

## 当前结论

- 渲染后端：`OpenGL`、`Vulkan` 已实现；`Metal` 在 Apple 平台已有上下文与窗口 surface 实现，`Backend::Auto` 会按平台候选顺序自动选择并在失败时 fallback。
- 平台层：桌面端使用仓库内固定的 `3rdparty/tina_glfw`；`GLFWWindow` 已覆盖 Windows / macOS / Linux。Linux/X11 的代码链路已接通，但仍需更多真机验证。
- Android：当前仓库已包含 `AndroidWindow`、`AndroidRuntime`、JNI/C ABI、`android/host`、`android/tinalux-sdk` 和 `android/validation-app`。
- UI 与 Markup：布局、事件、主题、控件、资源和轻量声明式 Markup DSL 都已落地；页面开发的推荐入口仍是 `TinaluxRunMarkupMentalModelExamples` 与 `samples/markup` 模板区。

## 新用户先看

- [用户快速上手](./用户快速上手.md)  
  第一次接触项目时的最短路径。
- [源码导览](./源码导览.md)  
  从 `main.cpp` 到 `Application / UIContext / Markup / Android` 的源码地图。
- [项目概述](./项目概述.md)  
  项目定位、模块边界、当前能力和主要缺口。

## 推荐阅读顺序

1. [用户快速上手](./用户快速上手.md)
2. [源码导览](./源码导览.md)
3. [项目概述](./项目概述.md)
4. [架构设计](./架构设计.md)
5. [UI框架能力清单](./UI框架能力清单.md)
6. 如果你要写页面，再进入下面的 Markup 文档组

## Markup 文档

- [Markup一页式速查](./Markup一页式速查.md)  
  先跑最小基准，再记推荐主路线。
- [samples/markup 模板区](../samples/markup/README.md)  
  直接可抄的四种模板。
- [Markup页面推荐写法](./Markup页面推荐写法.md)  
  抄 CMake 和 C++ 页面起手式。
- [Markup常见问题](./Markup常见问题.md)  
  先解高频困惑，再看低层接口。
- [Markup高级接口](./Markup高级接口.md)  
  只在低层场景反查。
- [MarkupDSL语法参考](./MarkupDSL语法参考.md)  
  `.tui` 语法速查。

## 专题文档

- [目录结构规划](./目录结构规划.md)
- [CI验证入口](./CI验证入口.md)
- [设计规则](./设计规则.md)
- [主题系统设计与实现](./主题系统设计与实现.md)
- [动画系统使用指南](./动画系统使用指南.md)
- [UI库核心能力完善-样式和布局](./UI库核心能力完善-样式和布局.md)
- [UI库核心能力完善-文本和动画](./UI库核心能力完善-文本和动画.md)
- [UI库核心能力完善-资源和工具](./UI库核心能力完善-资源和工具.md)
- [Android平台多界面架构方案](./Android平台多界面架构方案.md)
- [LinuxX11构建验证](./LinuxX11构建验证.md)
