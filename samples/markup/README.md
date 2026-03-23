# Markup 模板区

这不是新的 API 区。

这里的目标只有一个：

**让你少记东西，直接照着抄。**

## 1. 先记住这件事

`namespace` 不是额外调用层，只是生成代码的组织方式。

真正的页面主路线还是两段：

- 构造期绑定：`Page(theme, [&](auto& ui) { ... })`
- 页面持有后访问：`page.ui.xxx`

也就是说：

- 你写事件绑定时，更像 Qt，直接拿 `ui.loginButton`、`ui.queryInput`
- 你把页面对象挂在类上之后，再通过 `page.ui.xxx` 访问生成出来的控件代理

## 2. 模板目录

- [`single-file`](./single-file)
  最小单文件入口，适合一个 `.tui` 就够的页面。
- [`directory-scan`](./directory-scan)
  按目录批量扫描，适合 `auth/login.tui`、`settings/profile.tui` 这种分层结构。

## 3. 什么场景用哪种模板

- 页面很少，先跑通一个界面：抄 `single-file`
- 已经开始按目录拆页面：抄 `directory-scan`
- 只是普通业务页面：不用先碰 `NAMESPACE` / `NAMESPACE_PREFIX`

只有你真的要防重名，或者要和现有工具链拼装，才需要再看这些低层参数。

## 4. 如果你想在仓库里编译模板

先开启样板构建：

```powershell
cmake -S . -B cmake-build-debug -DTINALUX_BUILD_SAMPLES=ON
```

然后按需编译：

```powershell
cmake --build cmake-build-debug --config Debug --target TinaluxMarkupSingleFileSample
cmake --build cmake-build-debug --config Debug --target TinaluxMarkupDirectoryScanSample
```

## 5. 如果你还在纠结是不是过度封装

可以用一个简单标准判断：

- 如果日常页面开发时，你大多数时间都在写 `ui.xxx.onClick(...)`，这不算过度封装
- 如果你被迫先学 `Handlers / slots::actions / 一堆 helper` 才能开工，那才叫心智负担过重

当前仓库的推荐主路线就是前者。
