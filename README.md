# Tinalux

Tinalux 是一个基于 Skia 的跨平台自绘 UI 框架，当前仓库包含桌面窗口层、渲染后端、UI 组件体系，以及 Android 运行时与验证工程。

## 当前状态

- 渲染后端：`OpenGL`、`Vulkan` 已实现；`Metal` 已有 Apple 平台实现和 smoke 覆盖
- 平台：Windows / macOS / Linux(X11) 桌面链路已接通；Android 已具备原生运行时、宿主层和验证工程
- UI：布局、事件、主题、文本输入、资源管理、富文本、图片与常用控件已落地
- 工程现状：当前工作区里的 `cmake-build-debug` 已完成 smoke 目标构建，`ctest` 共登记并跑通 `63` 个测试；顶层 CMake 也已补齐对普通 `pwsh` 环境的 MSVC/Windows SDK 头文件注入

## 构建

桌面端默认依赖以下本地源码目录：

- `3rdparty/skia`
- `3rdparty/spdlog`
- `3rdparty/tina_glfw`

典型构建流程：

```bash
cmake -B build -S .
cmake --build build
```

如果需要显式选择桌面渲染后端，可设置环境变量 `TINALUX_DESKTOP_BACKEND`：

- `auto`
- `opengl`
- `vulkan`
- `metal`

## 目录入口

- `docs/README.md`：文档索引与当前结论
- `docs/项目概述.md`：项目定位、模块边界、平台状态
- `docs/开发计划.md`：当前优先级与里程碑
- `android/validation-app`：Android 验证工程
- `android/tinalux-sdk`：Android 宿主运行时封装
