# Markup 高级接口

这份文档只讲**低层 / 高级**用法。

如果你是在写普通页面，不要先看这里，先看 [`Markup页面推荐写法`](./Markup页面推荐写法.md)。

适合来看这份文档的场景：

- 你要脱离 `Page` 单独组装 `ViewModel`
- 你要手工绑定 `slots::actions...`
- 你要复用一份 markup，但不想引入页面类
- 你在写测试、代码生成工具或自定义构建链

## 1. `slots::binding::Handlers`

这是最低成本的“显式事件表”写法。

```cpp
generated_bindings::slots::binding::Handlers handlers {};
handlers.onLoginClicked = [] {
    submitLogin();
};
handlers.onQueryChanged = [&](std::string_view text) {
    keyword.assign(text);
};

auto viewModel = generated_bindings::slots::binding::makeViewModel(handlers);
```

适用：

- 你想单独拿到 `ViewModel`
- 你在做测试
- 你不需要页面对象，只想验证 action 路径是否正确

不适用：

- 正常页面开发

因为它会让你重新回到“事件表”和“路径”思维，而不是“控件对象”思维。

## 2. `slots::load(...)`

当你只想加载布局，但不想经过高层 `Page` 时可以用它。

```cpp
auto loaded = generated_bindings::slots::load(theme, handlers);
if (!loaded.ok()) {
    return;
}

loaded.ui.loginButton->setEnabled(true);
```

适用：

- 你要自己掌控页面外层对象
- 你要把生成的 ui 挂到已有架构里

不适用：

- 正常页面开发

正常页面开发直接用 `generated_bindings::Page` 更省心。

## 3. `slots::actions...`

这是最接近“裸 action path”的接口。

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

- 你要手工控制 action 到 `ViewModel` 的绑定
- 你在写工具链或测试

不适用：

- 正常页面开发

因为它要求你直接思考 action path，而不是控件对象。

## 4. `slots::attachUi(...)`

如果你手上已经有 `LayoutHandle` 或 `LoadResult`，可以后挂 ui 代理：

```cpp
auto loaded = tinalux::markup::LayoutLoader::loadFile("ui/login.tui", theme);
auto ui = generated_bindings::slots::attachUi(loaded);
ui.loginButton.onClick(this, &LoginPage::submitLogin);
```

适用：

- 你已有底层加载流程
- 你只是想补一层强类型控件访问

## 5. 什么时候该停在高层

只要你满足下面任意一条，就不要继续往低层退：

- 你已经有 `Page`
- 你主要是为了写页面逻辑
- 你想按控件对象思维开发
- 你不想再记 action path 和 `Handlers`

这时直接回到：

```cpp
page.ui.loginButton.onClick(this, &LoginPage::submitLogin);
page.ui.queryInput.onTextChanged(this, &LoginPage::onQueryChanged);
```
