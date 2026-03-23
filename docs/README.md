# Tinalux 文档索引

> 更新时间：2026-03-22
> 说明：本目录已按“当前事实 / 历史归档”重整。带日期的阶段报告默认视为归档，不再作为当前实现依据。

## 当前结论

- 渲染后端：`OpenGL`、`Vulkan` 已实现；`Metal` 在 Apple 平台已有上下文与窗口 surface 实现，`Backend::Auto` 会按平台候选顺序自动选择并在失败时 fallback。
- 平台层：桌面端使用仓库内固定的 `3rdparty/tina_glfw`；`GLFWWindow` 已覆盖 Windows / macOS / Linux。Linux/X11 的 IME 与窗口链路代码已接通，但缺少真机运行验证。
- Android：不再只是方案稿，当前已有 `AndroidWindow`、`AndroidRuntime`、JNI/C ABI、`android/host` 宿主层、`android/tinalux-sdk` 和 `android/validation-app` 验证工程。
- UI：约束布局、VBox/HBox/Flex/Grid/Responsive、三阶段事件、主题系统、富文本、文本输入、资源管理、异步图片加载、图标注册、组件级渲染缓存都已落地。
- Markup：轻量声明式布局 DSL 已落地，当前使用无 `@` 关键字语法，支持 `style / import / component / if / elseif / else / for / res(...)`、位置参数、可选裸标识符 `id`、`${model.xxx}` 单向/双向属性绑定、`${someId.someProperty}` 普通属性/样式绑定、`Slot`、树状 `ViewModel`、声明式事件绑定；默认高层入口就是 `tinalux_add_markup_executable(...)` / `tinalux_target_enable_markup_autogen(...)`，单文件和目录扫描都可直接生成强类型 `Page / ui / slots` 头文件，默认输出到 `${CMAKE_CURRENT_BINARY_DIR}/tinalux_markup/<target>`；同一个入口现在还能顺手生成页面类骨架，默认走 `Page + ui.xxx.onXxx(...)` 主路线；普通页面开发建议先看 [`Markup一页式速查`](./Markup一页式速查.md)，再看仓库里的 [`samples/markup`](../samples/markup/README.md) 模板区，最后按需看 [`Markup页面推荐写法`](./Markup页面推荐写法.md)；`Handlers / slots::load / slots::actions...` 和独立 scaffold helper 这些低层接口统一放到 [`Markup高级接口`](./Markup高级接口.md)。
- 测试：源码中的 [`tests/CMakeLists.txt`](../tests/CMakeLists.txt) 当前包含 `74` 个 smoke 声明和 `2` 个脚本测试；当前工作区里的 `cmake-build-debug` 通过 `ctest -N -C Debug --test-dir cmake-build-debug` 可见 `76` 个测试。

## 优先阅读

- [项目概述](./项目概述.md)  
  项目定位、模块边界、当前能力和主要缺口。
- [源码核对-当前状态-2026-03-16](./源码核对-当前状态-2026-03-16.md)  
  基于源码和当前构建产物的最新核对结论，虽然文件名保留旧日期，但内容已更新到 2026-03-18。
- [架构设计](./架构设计.md)  
  当前真实架构，而不是早期设计草案。
- [UI框架能力清单](./UI框架能力清单.md)  
  按模块罗列已实现能力、部分实现项和明确未完成项。
- [Markup一页式速查](./Markup一页式速查.md)  
  先看这一份，压缩成“普通页面开发真正需要记的内容”。
- [samples/markup 模板区](../samples/markup/README.md)  
  直接可抄的四种模板：单文件、目录扫描、单文件 scaffold、目录扫描 scaffold。
- [Markup页面推荐写法](./Markup页面推荐写法.md)  
  正常页面开发的抄代码版，默认主路线是 `Page + ui + onClick(...)`。
- [Markup高级接口](./Markup高级接口.md)  
  只在低层场景反查：`Handlers / slots::load / slots::actions / attachUi(...)`。
- [MarkupDSL语法参考](./MarkupDSL语法参考.md)  
  只讲 `.tui` 本身怎么写，已按“最常用 / 较少使用”分区。

## 当前文档

- [目录结构规划](./目录结构规划.md)  
  反映当前仓库目录、Android 模块和构建入口。
- [开发计划](./开发计划.md)  
  当前主线任务，不再沿用早期阶段式完成度叙述。
- [主题系统设计与实现](./主题系统设计与实现.md)  
  说明 `Theme`、`ThemeManager`、样式 token 和已实现边界。
- [动画系统使用指南](./动画系统使用指南.md)  
  说明 `AnimationSink`、调度器接线和控件动画现状。
- [UI库核心能力完善-样式和布局](./UI库核心能力完善-样式和布局.md)
- [UI库核心能力完善-文本和动画](./UI库核心能力完善-文本和动画.md)
- [UI库核心能力完善-资源和工具](./UI库核心能力完善-资源和工具.md)
- [Android平台多界面架构方案](./Android平台多界面架构方案.md)
- [Android平台实现进展-2026-03-17](./Android平台实现进展-2026-03-17.md)
- [LinuxX11构建验证](./LinuxX11构建验证.md)
- [IME进度记录-2026-03-16](./IME进度记录-2026-03-16.md)
- [设计规则](./设计规则.md)
- [Markup一页式速查](./Markup一页式速查.md)
- [Markup页面推荐写法](./Markup页面推荐写法.md)
- [Markup高级接口](./Markup高级接口.md)
- [阶段交付-Canvas收口](./阶段交付-Canvas收口.md)

## 历史归档

下面这些文档保留为阶段记录或回顾材料，不再代表当前源码状态：

- [项目当前进度总结-2026-03-15](./项目当前进度总结-2026-03-15.md)
- [项目完整状态报告-2026-03-15](./项目完整状态报告-2026-03-15.md)
- [修复进度报告-2026-03-15](./修复进度报告-2026-03-15.md)
- [代码改进分析报告-2026-03-15](./代码改进分析报告-2026-03-15.md)
- [改进工作总结-2026-03-15](./改进工作总结-2026-03-15.md)
- [文档更新说明-2026-03-15](./文档更新说明-2026-03-15.md)
- [快速改进指南](./快速改进指南.md)
- [实施建议-优先级清单](./实施建议-优先级清单.md)
- [代码审查报告](./代码审查报告.md)
