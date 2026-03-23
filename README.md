# Tinalux

Tinalux 是一个基于 Skia 的跨平台自绘 UI 框架，当前仓库包含桌面窗口层、渲染后端、UI 组件体系，以及 Android 运行时与验证工程。

## 当前状态

- 渲染后端：`OpenGL`、`Vulkan` 已实现；`Metal` 已有 Apple 平台实现和 smoke 覆盖
- 平台：Windows / macOS / Linux(X11) 桌面链路已接通；Android 已具备原生运行时、宿主层和验证工程
- UI：布局、事件、主题、文本输入、资源管理、富文本、图片与常用控件已落地
- 工程现状：当前工作区里的 `cmake-build-debug` 已完成 smoke 目标构建；`ctest --test-dir cmake-build-debug -C Debug --output-on-failure --timeout 60 -j 4` 已跑通 `78/78` 个测试；顶层 CMake 也已补齐对普通 `pwsh` 环境的 MSVC/Windows SDK 头文件注入

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

如果你想最快验证仓库里推荐的 Markup 主路线，可以直接跑：

```bash
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug --config Debug --target TinaluxRunMarkupMentalModelExamples
```

这会直接构建并运行两份“官方最小基准示例”：

- `tests/markup/mental_model_smoke.cpp`
- `tests/markup/mental_model_scan_smoke.cpp`

如果需要显式选择桌面渲染后端，可设置环境变量 `TINALUX_DESKTOP_BACKEND`：

- `auto`
- `opengl`
- `vulkan`
- `metal`

## 目录入口

- `docs/README.md`：文档索引与当前结论
- `docs/项目概述.md`：项目定位、模块边界、平台状态
- `docs/项目审查与优先级建议-2026-03-23.md`：本轮源码审查、验证结论、提交脉络与优先级建议
- `docs/开发计划.md`：当前优先级与里程碑
- `android/validation-app`：Android 验证工程
- `android/tinalux-sdk`：Android 宿主运行时封装

## Markup 从哪开始看

如果你现在主要是想用仓库里的 Markup 页面开发能力，不要先钻 CMake 低层 helper。

直接按这个顺序看：

1. `docs/Markup一页式速查.md`
2. `samples/markup/README.md`
3. `docs/Markup页面推荐写法.md`
4. 带着概念困惑时，先看：
   `docs/Markup常见问题.md`
5. 只有遇到特定问题，再看：
   `docs/Markup高级接口.md` / `docs/MarkupDSL语法参考.md`

可以把它理解成：

- 先决定抄哪套模板
- 再抄 CMake 和 C++ 页面起手式
- 只有真的卡到 DSL 规则或低层接口，再往下看
- 想最快确认主路线没偏，可以直接跑 `TinaluxRunMarkupMentalModelExamples`
