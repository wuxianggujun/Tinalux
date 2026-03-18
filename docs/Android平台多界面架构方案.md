# Android 平台多界面架构方案

> 更新时间：2026-03-18  
> 状态：已根据当前源码改写。Android 不是纯方案阶段，而是已经有一条可运行的实现链。

## 当前已落地的基础架构

当前 Android 侧已经形成如下接线：

```text
Activity / Fragment
  -> TinaluxSurfaceView
  -> TinaluxRendererHost
  -> TinaluxNativeBridge
  -> Android C ABI
  -> AndroidRuntime
  -> Application
  -> UIContext
```

其中：

- `AndroidRuntime` 管理 `Application` 生命周期
- `AndroidWindow` 对接 `ANativeWindow`
- 宿主侧 Kotlin 代码位于 `android/host`
- SDK 入口位于 `android/tinalux-sdk`
- 验证工程位于 `android/validation-app`

## 当前推荐方案

### 1. 同一个 Activity 内的多页面

推荐做法：

- 保持一个 `AndroidRuntime`
- 保持一个 `Application`
- 在同一棵 Widget 树内做页面切换或导航

原因：

- 当前 runtime 已经支持窗口 detach / attach、pause / resume
- 单 runtime 更容易保留 UI 会话和主题状态
- 更符合当前桌面和 Android 的共同架构

### 2. 多个 Activity

如果业务确实需要多个 Activity，更推荐：

- 每个 Activity 持有自己的宿主 view / runtime
- 跨 Activity 状态由上层应用层管理
- 不要指望当前 native 层自动共享渲染资源或 Widget 树

更准确地说，当前 native 层适合“一个 surface 对应一个 runtime”。

## 当前 runtime 的关键能力

- `attachWindow`
- `detachWindow`
- `renderOnce`
- `setPreferredBackend`
- `pause`
- `resume`

并且当前逻辑不是简单“窗口销毁就把整个 Application 杀掉”，而是：

- 可保留 UI 会话
- 释放并重建窗口 / context / surface

## 后端策略

- 默认后端：`OpenGL`
- 可显式切换：`Vulkan`

现阶段不建议把 Android 文档写成“多后端完全稳定可用”，因为真机验证还没有完成。

## 当前仍未解决的问题

- 真机渲染闭环验证
- 真机输入法与组合态验证
- 更高层的手势和触摸语义
- 多 runtime 之间的资源共享策略

## 当前文档应避免的说法

- “Android 还没有实现 Window 和 Runtime”
- “只能做纯桌面架构复用，Android 仍停留在讨论”
- “多 Activity 一定要共享一个 native Application 实例”

这些说法都已经和当前源码不一致。
