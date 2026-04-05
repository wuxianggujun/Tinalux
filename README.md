# Tinalux

[English](./README.en.md) | 简体中文

Tinalux 是一个基于 Skia 的跨平台自绘 UI 框架，当前源码已经覆盖桌面窗口层、
多渲染后端、UI 组件体系、声明式 Markup 页面能力，以及 Android 原生运行时和
宿主层骨架。

当前源码版本：`0.1.0`

## 项目简介

Tinalux 的目标不是包装系统原生控件，而是自己维护 Widget 树、布局、命中测试、
绘制和主题系统。

从当前源码看，仓库已经具备这些核心能力：

- `Core`：基础类型、事件、日志、版本信息
- `Platform`：桌面 `GLFWWindow`、Android `AndroidWindow`
- `Rendering`：`OpenGL`、`Vulkan`、`Metal` 后端和 Skia surface/context 封装
- `UI`：布局、事件系统、主题、资源、文本、图片和常用控件
- `Markup`：DSL、Parser、AST、`LayoutBuilder`、`LayoutLoader`、树状 `ViewModel`
- `App`：`Application`、`UIContext`、后端 fallback、主循环和运行时同步

顶层桌面入口在 `main.cpp`，默认创建 `Application`，选择桌面渲染后端，加载
`Theme::dark()`，并启动 `createDemoScene(...)` 演示界面。

## 当前平台状态

- Windows：桌面主链路已实现
- macOS：桌面主链路已实现，包含 Cocoa 文本输入桥接
- Linux/X11：代码链路已接通，但 IME 和真机行为仍需更多验证
- Android：已具备原生运行时、JNI/C ABI、宿主层 Kotlin 代码和验证工程

## 已实现能力概览

- 渲染后端：`Auto`、`OpenGL`、`Vulkan`、`Metal`
- 布局系统：`VBox`、`HBox`、`Flex`、`Grid`、`Responsive`
- 事件模型：capture → target → bubble，含 overlay 命中、焦点、hover、鼠标捕获
- UI 控件：`Button`、`TextInput`、`Checkbox`、`Radio`、`Toggle`、`Slider`、
  `ScrollView`、`ListView`、`Dialog`、`Dropdown`、`ProgressBar`、`RichText`、
  `ImageWidget`
- 主题与样式：`Theme`、`ThemeManager`、`ColorScheme`、`Typography`、typed styles
- 资源能力：`IconRegistry`、`ResourceManager`、`ResourceLoader`
- Markup：`style`、`import`、`component`、`Slot`、`if / elseif / else / for`、
  `${model.xxx}` 绑定、声明式页面生成
- Android：`android/host`、`android/tinalux-sdk`、`android/validation-app`

## 构建要求

桌面构建默认依赖以下本地源码目录：

- `3rdparty/skia`
- `3rdparty/spdlog`
- `3rdparty/tina_glfw`

基础要求：

- CMake `3.31+`
- 支持 C++20 的编译器
- 可用的 Skia 构建产物，或按项目里的 Skia CMake 逻辑自动构建

## 快速开始

### 1. 构建桌面演示程序

```bash
cmake -S . -B build
cmake --build build --config Debug --target Tinalux
```

如果你使用的是单配置生成器，比如 Ninja，可以去掉 `--config Debug`。

### 2. 切换桌面渲染后端

可通过环境变量 `TINALUX_DESKTOP_BACKEND` 指定：

- `auto`
- `opengl`
- `vulkan`
- `metal`

### 3. 快速验证 Markup 主路线

`TinaluxRunMarkupMentalModelExamples` 目标定义在测试工程里，因此需要开启
`TINALUX_BUILD_TESTS`（默认就是 `ON`）：

```bash
cmake -S . -B build -DTINALUX_BUILD_TESTS=ON
cmake --build build --config Debug --target TinaluxRunMarkupMentalModelExamples
```

它会执行两组 Markup mental model smoke，用来确认当前推荐页面写法没有跑偏。

### 4. 常用 CMake 选项

- `TINALUX_BUILD_TESTS=ON`：构建测试，默认开启
- `TINALUX_BUILD_SAMPLES=OFF`：构建 `samples/`，默认关闭

## 仓库结构

- `include/`：公开头文件
- `src/`：核心实现，按 `core / platform / rendering / ui / markup / app` 分层
- `tests/`：smoke tests 与脚本验证入口
- `samples/markup/`：Markup 页面模板和示例
- `docs/`：当前事实文档、架构说明和使用指引
- `android/host/`：Android 宿主层 Kotlin 骨架
- `android/tinalux-sdk/`：Android SDK/AAR 形态封装
- `android/validation-app/`：Android 验证工程
- `sponsor/`：赞赏二维码素材

## 推荐阅读顺序

- [`docs/README.md`](./docs/README.md)：文档索引
- [`docs/项目概述.md`](./docs/项目概述.md)：模块边界和源码状态
- [`docs/UI框架能力清单.md`](./docs/UI框架能力清单.md)：能力清单
- [`docs/Markup一页式速查.md`](./docs/Markup一页式速查.md)：Markup 快速入口
- [`samples/markup/README.md`](./samples/markup/README.md)：可直接照抄的模板

## 许可证

本仓库采用 [`Tinalux Learning-Only License 1.0`](./LICENSE)。

这是一份**仅允许个人学习、研究、评估和非商用使用**的源码可见许可证，不属于
OSI 定义下的开源许可证。

- 允许：个人下载、阅读、编译、运行、私下修改，用于学习和研究
- 禁止：任何商用、生产环境使用、对外分发、再授权、售卖或作为付费服务的一部分
- 如需商用授权或分发授权，请联系仓库维护者获得书面许可

中文说明见 [`LICENSE.zh-CN.md`](./LICENSE.zh-CN.md)。

## 赞赏支持

如果这个项目对你有帮助，欢迎通过下面的方式支持持续维护。

### 微信赞赏

<img src="./sponsor/weixin.jpg" alt="微信赞赏二维码" width="260" />

### 支付宝赞赏

<img src="./sponsor/zhifubao.jpg" alt="支付宝赞赏二维码" width="260" />
