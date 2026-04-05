# Tinalux

[简体中文](./README.md) | English

> A cross-platform custom-drawn UI framework on top of Skia, covering desktop runtime, declarative Markup UI, and Android host/native runtime wiring.

Tinalux does not wrap native system widgets. It owns its widget tree, layout,
hit testing, painting, and theming runtime. The current repository already
includes the desktop window layer, multiple render backends, a UI widget
system, a declarative Markup pipeline, and an Android native runtime plus host
scaffolding.

Current source version: `0.1.0`

## Overview

Based on the current source tree, the project is organized around these modules:

- `Core`: base types, events, logging, version info
- `Platform`: desktop `GLFWWindow` and Android `AndroidWindow`
- `Rendering`: `OpenGL`, `Vulkan`, `Metal`, and Skia surface/context integration
- `UI`: layout, event dispatch, themes, resources, text, images, and widgets
- `Markup`: DSL, parser, AST, `LayoutBuilder`, `LayoutLoader`, tree-shaped
  `ViewModel`
- `App`: `Application`, `UIContext`, backend fallback, main loop, runtime sync

The desktop entry point is `main.cpp`. It creates `Application`, resolves the
desktop backend, applies `Theme::dark()`, and boots the `createDemoScene(...)`
demo UI.

## Platform Status

- Windows: desktop runtime path is implemented
- macOS: desktop runtime path is implemented, including Cocoa text input bridge
- Linux/X11: code path is connected, but IME and device validation still need
  more coverage
- Android: native runtime, JNI/C ABI, Kotlin host layer, and validation app are
  already present

## Implemented Capabilities

- Render backends: `Auto`, `OpenGL`, `Vulkan`, `Metal`
- Layout: `VBox`, `HBox`, `Flex`, `Grid`, `Responsive`
- Event model: capture → target → bubble, plus overlay hit priority, focus,
  hover tracking, and mouse capture
- Widgets: `Button`, `TextInput`, `Checkbox`, `Radio`, `Toggle`, `Slider`,
  `ScrollView`, `ListView`, `Dialog`, `Dropdown`, `ProgressBar`, `RichText`,
  `ImageWidget`
- Theming and styles: `Theme`, `ThemeManager`, `ColorScheme`, `Typography`,
  typed style structs
- Resources: `IconRegistry`, `ResourceManager`, `ResourceLoader`
- Markup: `style`, `import`, `component`, `Slot`, `if / elseif / else / for`,
  `${model.xxx}` bindings, generated page scaffolding
- Android packaging: `android/host`, `android/tinalux-sdk`,
  `android/validation-app`

## Build Requirements

Desktop builds expect these local source directories:

- `3rdparty/skia`
- `3rdparty/spdlog`
- `3rdparty/tina_glfw`

Basic requirements:

- CMake `3.31+`
- A C++20-capable compiler
- Available Skia build outputs, or use the project's Skia CMake flow to build
  them

## Quick Start

### 1. Build the desktop demo

```bash
cmake -S . -B build
cmake --build build --config Debug --target Tinalux
```

If you use a single-config generator such as Ninja, remove `--config Debug`.

### 2. Select a desktop backend

Use the `TINALUX_DESKTOP_BACKEND` environment variable:

- `auto`
- `opengl`
- `vulkan`
- `metal`

### 3. Verify the recommended Markup path

The `TinaluxRunMarkupMentalModelExamples` target is defined under the test
project, so `TINALUX_BUILD_TESTS` must be enabled. It is `ON` by default.

```bash
cmake -S . -B build -DTINALUX_BUILD_TESTS=ON
cmake --build build --config Debug --target TinaluxRunMarkupMentalModelExamples
```

This runs the Markup mental-model smoke coverage used as the recommended page
authoring baseline.

### 4. Common CMake options

- `TINALUX_BUILD_TESTS=ON`: build tests, enabled by default
- `TINALUX_BUILD_SAMPLES=OFF`: build `samples/`, disabled by default

## Repository Layout

- `include/`: public headers
- `src/`: implementation split into `core / platform / rendering / ui / markup / app`
- `tests/`: smoke tests and script-based validation
- `samples/markup/`: Markup templates and examples
- `docs/`: architecture notes, current-state docs, and guides
- `android/host/`: Android host-side Kotlin scaffold
- `android/tinalux-sdk/`: Android SDK/AAR-style packaging module
- `android/validation-app/`: Android validation project
- `sponsor/`: sponsorship QR code assets

## Recommended Reading Order

- [`docs/用户快速上手.md`](./docs/用户快速上手.md): shortest onboarding path for first-time users (Chinese)
- [`docs/源码导览.md`](./docs/源码导览.md): source map from `main.cpp` to `Application / UIContext / Markup` (Chinese)
- [`docs/README.md`](./docs/README.md): documentation index (Chinese)
- [`docs/项目概述.md`](./docs/项目概述.md): module boundaries and current status
- [`docs/UI框架能力清单.md`](./docs/UI框架能力清单.md): UI capability overview
- [`docs/Markup一页式速查.md`](./docs/Markup一页式速查.md): Markup quick start and cheat sheet
- [`samples/markup/README.md`](./samples/markup/README.md): directly reusable templates

## License

This repository uses the [`Tinalux Learning-Only License 1.0`](./LICENSE).

It is a **source-available, personal-learning-only, non-commercial** license.
It is **not** an OSI-approved open source license.

- Allowed: personal study, research, evaluation, local builds, private changes
- Prohibited: commercial use, production use, redistribution, sublicensing,
  resale, or use as part of paid services
- For commercial or redistribution rights, obtain prior written permission from
  the repository maintainer

A Chinese explanatory copy is available in [`LICENSE.zh-CN.md`](./LICENSE.zh-CN.md).

## Support The Project

If Tinalux is useful to you, you can support ongoing maintenance here.

### WeChat

<img src="./sponsor/weixin.jpg" alt="WeChat support QR code" width="260" />

### Alipay

<img src="./sponsor/zhifubao.jpg" alt="Alipay support QR code" width="260" />
