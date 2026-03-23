# Markup 常见问题

这份文档只回答几个最常见的困惑。

如果你是第一次接触这套能力，建议先配合下面几份一起看：

- [`Markup一页式速查`](./Markup一页式速查.md)
- [`samples/markup/README.md`](../samples/markup/README.md)
- [`Markup页面推荐写法`](./Markup页面推荐写法.md)

## 1. `page.ui` 是唯一调用机制吗？

不是。

普通页面开发其实有两种视角：

- 构造阶段：`Page(theme, [&](auto& ui) { ... })`
- 构造完成后：`page.ui.xxx`

也就是说：

- 你在绑定事件时，最推荐的是直接写 `ui.loginButton.onClick(...)`
- 你在页面对象已经持有之后，再通过 `page.ui.loginButton` 访问控件代理

所以 `page.ui` 不是唯一机制，它只是“页面构造完成后的访问入口”。

## 2. 这种设计会不会过度封装？

看你平时主要在写什么。

如果你大多数时间写的是：

```cpp
ui.loginButton.onClick(this, &LoginPage::submitLogin);
ui.queryInput.onTextChanged(this, &LoginPage::onQueryChanged);
page.ui.loginButton->setEnabled(false);
```

这不算过度封装。

真正的过度封装更像这样：

- 普通页面开发也被迫先学 `Handlers`
- 先学 `slots::actions`
- 先记 action path
- 先理解一堆独立 helper 才能开工

当前仓库的推荐路线已经尽量避免后者。

## 3. 为什么不直接只保留 `ui.xxx`，不要 `page.ui.xxx`？

因为这两个时机不同。

`ui.xxx`：

- 适合在构造回调里集中绑事件
- 让写法更像 Qt 的“拿控件对象直接 connect”

`page.ui.xxx`：

- 适合页面对象已经创建完成后的访问
- 适合构造后再做启用、赋值、状态同步

所以不是二选一，而是：

- 构造时用 `ui.xxx`
- 构造后用 `page.ui.xxx`

## 4. 命名空间什么时候才需要自己写？

大多数普通页面开发，不需要。

默认推导通常已经够用：

```text
ui/login.tui      -> login::Page
ui/auth/login.tui -> auth::login::Page
```

只有这些场景才值得主动写：

- 你要防重名
- 你要给一批页面挂统一前缀
- 你在接自己的工具链，想固定生成符号路径

如果你只是普通业务页面，先别主动碰 `NAMESPACE` / `NAMESPACE_PREFIX`。

## 5. 什么时候该选 scaffold？

只看一个判断标准就够了：

- 你想自己写页面类外壳
  -> 用 `single-file` / `directory-scan`
- 你想让生成器先把页面类起手式写出来
  -> 用 `scaffold-single-file` / `scaffold-directory-scan`

scaffold 不是另一套机制。

它只是先帮你生成：

- `setupUi(ui)`
- `initUi(ui)`
- `bindUi(ui)`

后面你继续维护时，核心仍然是 `ui.xxx` 和 `page.ui.xxx`。

## 6. 为什么不推荐普通页面先看 `Handlers / slots::actions`？

因为它们更接近“生成代码内部结构”，不是最省脑子的业务开发入口。

它们适合：

- 写测试
- 写工具链
- 手工组 `ViewModel`
- 自己接已有页面外壳

它们不适合：

- 一个普通业务页面刚开工的时候

如果你只是写页面逻辑，优先保持在：

```cpp
page(theme, [&](auto& ui) {
    ui.loginButton.onClick(this, &LoginPage::submitLogin);
})
```

## 7. CMake 自定义方法很多，看得头大，怎么办？

普通页面开发只记两个高层入口：

- `tinalux_add_markup_executable(...)`
- `tinalux_target_enable_markup_autogen(...)`

其他 helper 先全部当成“低层实现细节”。

更实际一点的做法是：

1. 先跑 `TinaluxRunMarkupMentalModelExamples`
2. 再从 `samples/markup` 四套模板里选一个抄
3. 真遇到特殊场景，再回头查低层 helper

不要一开始就试图把所有自定义方法都读懂。

## 8. 最快验证推荐主路线的命令是什么？

直接跑：

```powershell
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug --config Debug --target TinaluxRunMarkupMentalModelExamples
```

它会直接构建并运行两份最小基准示例：

- [`tests/markup/mental_model_smoke.cpp`](../tests/markup/mental_model_smoke.cpp)
- [`tests/markup/mental_model_scan_smoke.cpp`](../tests/markup/mental_model_scan_smoke.cpp)
