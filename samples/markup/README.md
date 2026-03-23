# Markup 模板区

这不是新的 API 区。

这里的目标只有一个：

**让你少记东西，直接照着抄。**

## 0. 想最快确认主路线，先跑这个

如果你现在不想先比较模板，想先确认仓库里的推荐主路线没偏，直接跑：

```powershell
cmake -S . -B cmake-build-debug -DTINALUX_BUILD_SAMPLES=ON
cmake --build cmake-build-debug --config Debug --target TinaluxRunMarkupMentalModelExamples
```

这会直接构建并运行两份“官方最小基准示例”：

- [`tests/markup/mental_model_smoke.cpp`](../tests/markup/mental_model_smoke.cpp)
- [`tests/markup/mental_model_scan_smoke.cpp`](../tests/markup/mental_model_scan_smoke.cpp)

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
- [`scaffold-single-file`](./scaffold-single-file)
  让生成器顺手起一个页面类骨架，适合你不想手写 `Page` 外壳时直接开工。
- [`scaffold-directory-scan`](./scaffold-directory-scan)
  目录扫描 + 批量页面骨架，适合页面一多就想统一起手式的场景。

## 3. 四选一就够了

```text
我现在想怎么开工？
├── 我想自己写页面类
│   ├── 一个 .tui              -> single-file
│   └── 一整个 ui/ 目录        -> directory-scan
└── 我想让生成器先起页面骨架
    ├── 一个 .tui              -> scaffold-single-file
    └── 一整个 ui/ 目录        -> scaffold-directory-scan
```

这四个目录只是四种起手模板，不是四套独立调用机制。

## 4. 什么场景用哪种模板

- 页面很少，先跑通一个界面：抄 `single-file`
- 已经开始按目录拆页面：抄 `directory-scan`
- 你想让生成器把页面类起手式也一起写出来：抄 `scaffold-single-file`
- 你已经在按目录拆页面，还想顺手生成一批页面类：抄 `scaffold-directory-scan`
- 只是普通业务页面：不用先碰 `NAMESPACE` / `NAMESPACE_PREFIX`

只有你真的要防重名，或者要和现有工具链拼装，才需要再看这些低层参数。

## 5. 命名空间到底是干嘛的

大多数时候，它只是“生成代码的名字路径”。

直接看例子就够了：

```text
ui/login.tui                  -> login::Page
ui/auth/login.tui             -> auth::login::Page
ui/auth/login.tui + NAMESPACE_PREFIX app_ui
                              -> app_ui::auth::login::Page
```

所以自定义命名空间主要就干两件事：

- 防重名
- 给一批生成页挂统一前缀

如果你只是普通业务页面，默认推导通常已经够用。

## 6. 如果你想在仓库里编译模板

先开启样板构建：

```powershell
cmake -S . -B cmake-build-debug -DTINALUX_BUILD_SAMPLES=ON
```

然后按需编译：

```powershell
cmake --build cmake-build-debug --config Debug --target TinaluxMarkupSingleFileSample
cmake --build cmake-build-debug --config Debug --target TinaluxMarkupDirectoryScanSample
cmake --build cmake-build-debug --config Debug --target TinaluxMarkupScaffoldSingleFileSample
cmake --build cmake-build-debug --config Debug --target TinaluxMarkupScaffoldDirectoryScanSample
```

上面那条 `TinaluxRunMarkupMentalModelExamples` 命令，
跑的就是这两份持续回归的最小基准示例。

## 7. 如果你还在纠结是不是过度封装

可以用一个简单标准判断：

- 如果日常页面开发时，你大多数时间都在写 `ui.xxx.onClick(...)`，这不算过度封装
- 如果你被迫先学 `Handlers / slots::actions / 一堆 helper` 才能开工，那才叫心智负担过重

当前仓库的推荐主路线就是前者。

如果你选的是 scaffold 模板，也还是同一个思路：

- 生成器只是在帮你先写好页面类外壳
- 生成出来的骨架内部仍然是 `setupUi(ui)` -> `initUi(ui)` + `bindUi(ui)`
- 你后面继续维护时，核心仍然是直接看 `ui.xxx` 和 `page.ui.xxx`
