# Markup 高级接口

这份文档只给下面这些场景用：

- 你在写测试
- 你在写代码生成工具
- 你在接自己的构建链
- 你明确不想走 `Page` 这条高层主路线

如果你是在写普通页面，先不要看这里，先回：

- [`Markup一页式速查`](./Markup一页式速查.md)
- [`Markup页面推荐写法`](./Markup页面推荐写法.md)
- [`samples/markup/README.md`](../samples/markup/README.md)

一句话记忆：

**普通页面开发，优先想控件对象。高级接口，才去想 action path 和底层拼装。**

## 1. 先看这张反查图

```text
我现在到底遇到了什么问题？
├── 我只想先组一个 ViewModel / 显式事件表
│   └── 看 slots::binding::Handlers
├── 我不想用 Page，但还是想直接 load 生成布局
│   └── 看 slots::load(...)
├── 我想手工按 action path 一条条 connect
│   └── 看 slots::actions...
└── 我已经自己 load 完底层结果，只想补强类型 ui 代理
    └── 看 slots::attachUi(...)
```

这四个接口不是给普通页面平铺着背的。

它们只是四种低层场景的入口。

## 2. 场景一：我只想单独组一个 ViewModel

这种情况优先看 `slots::binding::Handlers`。

它本质上就是一张“显式事件表”。

```cpp
generated_bindings::slots::binding::Handlers handlers {};
handlers.onLoginClicked = [&] {
    submitLogin();
};
handlers.onQueryChanged = [&](std::string_view text) {
    keyword.assign(text);
};

auto viewModel = generated_bindings::slots::binding::makeViewModel(handlers);
```

适用：

- 你要单独拿到 `ViewModel`
- 你在写测试
- 你只想验证 action 到回调的映射

不适用：

- 普通页面开发

代价：

- 你要自己维护一张 handlers 表
- 你会重新回到 “action 名字” 思维，而不是 “控件对象” 思维

最佳实践：

- 只在测试、工具链或非常薄的组装层里用
- 业务页面不要默认从这里起手

## 3. 场景二：我不想用 `Page`，但还是想直接加载布局

这种情况看 `slots::load(...)`。

```cpp
generated_bindings::slots::binding::Handlers handlers {};
handlers.onLoginClicked = [&] {
    submitLogin();
};

auto loaded = generated_bindings::slots::load(theme, handlers);
if (!loaded.ok()) {
    return;
}

loaded.ui.loginButton->setEnabled(true);
```

适用：

- 你要自己掌控页面外层对象
- 你要把生成的 UI 接进已有架构
- 你不想直接持有 `Page`

不适用：

- 只是想写一个正常业务页面

代价：

- 你得自己处理高层页面对象之外的组织逻辑
- 你失去 `Page` 那种“开箱即用”的默认路径

最佳实践：

- 只有在现有工程已经有自己的页面外壳时再用
- 如果只是新页面开发，直接 `generated_bindings::Page` 更省脑子

## 4. 场景三：我想手工控制 action path 的连接

这种情况看 `slots::actions...`。

```cpp
auto viewModel = tinalux::markup::ViewModel::create();

generated_bindings::slots::actions.onLoginClicked.connect(
    viewModel,
    this,
    &LoginPage::submitLogin);

generated_bindings::slots::actions.onQueryChanged.connect(
    viewModel,
    this,
    &LoginPage::onQueryChanged);
```

如果 action path 有层级：

```cpp
grouped_bindings::slots::actions.toolbar.onClearClicked.connect(
    viewModel,
    this,
    &ToolbarLogic::clearQuery);
```

适用：

- 你要精确控制 action 到 `ViewModel` 的连接过程
- 你在写工具链
- 你在写偏底层的测试

不适用：

- 普通页面开发

代价：

- 你需要直接思考 action path
- 代码更接近“生成代码内部结构”，而不是“页面对象”

最佳实践：

- 只有在你真的需要手工 connect 时再用
- 一旦只是页面逻辑，退回 `ui.xxx.onXxx(...)`

## 5. 场景四：我已经自己 load 了底层结果，只想补一个强类型 ui 代理

这种情况看 `slots::attachUi(...)`。

```cpp
auto loaded = tinalux::markup::LayoutLoader::loadFile("ui/login.tui", theme);
auto ui = generated_bindings::slots::attachUi(loaded);

ui.loginButton.onClick(this, &LoginPage::submitLogin);
ui.queryInput.onTextChanged(this, &LoginPage::onQueryChanged);
```

适用：

- 你已有底层加载流程
- 你只想在现有流程上补一层强类型控件访问

不适用：

- 从零开始写普通页面

代价：

- 你要先自己处理底层加载流程
- 它只是补 UI 代理，不会帮你解决页面外壳组织问题

最佳实践：

- 适合已有 `LayoutHandle` / `LoadResult` 的场景
- 不要为了用它，反过来把高层 `Page` 拆掉

## 6. 什么时候应该立刻停在高层

只要满足下面任意一条，就不要继续往低层退：

- 你已经有 `Page`
- 你主要是在写页面逻辑
- 你想按控件对象思维开发
- 你不想再记 action path
- 你只是想少写样板

这时直接回到：

```cpp
page.ui.loginButton.onClick(this, &LoginPage::submitLogin);
page.ui.queryInput.onTextChanged(this, &LoginPage::onQueryChanged);
```

或者在构造期直接写：

```cpp
page(theme, [&](auto& ui) {
    ui.loginButton.onClick(this, &LoginPage::submitLogin);
    ui.queryInput.onTextChanged(this, &LoginPage::onQueryChanged);
})
```

## 7. 一个简单判断标准

如果你在问自己：

- “我要不要先学 `Handlers`？”
- “我要不要直接写 `slots::actions`？”
- “我要不要先自己拼 `ViewModel`？”

那通常说明你现在看得太低了。

普通页面开发，先从下面四个模板里选一个：

- `samples/markup/single-file`
- `samples/markup/directory-scan`
- `samples/markup/scaffold-single-file`
- `samples/markup/scaffold-directory-scan`

只有这些模板确实不够用时，才回来翻这份文档。
