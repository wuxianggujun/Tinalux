# Markup 页面推荐写法

这份文档只讲**日常页面开发**最推荐的一条主路线：

- CMake 自动生成
- 页面类里持有 `Page`
- 通过 `ui.xxx.onClick(...)`、`ui.xxx.onTextChanged(...)` 直接绑事件
- `Handlers / slots::load / slots::actions...` 归类为高级接口，平时可以先忽略

如果你确实要碰这些低层接口，单独看 [`Markup高级接口`](./Markup高级接口.md)。

## 1. 最小 CMake

```cmake
tinalux_add_markup_executable(
    TARGET LoginDemo
    SOURCE src/main.cpp
    INPUT ui/login.tui
)
```

如果你已经自己写了 target，也可以只挂自动生成：

```cmake
tinalux_target_enable_markup_autogen(
    TARGET LoginDemo
    INPUT ui/login.tui
)
```

## 2. 推荐页面类写法

```cpp
#include "login.markup.h"

class LoginPage {
public:
    int loginClicks = 0;
    std::string keyword;
    app::login::Page page;

    explicit LoginPage(const tinalux::ui::Theme& theme)
        : page(theme, [&](auto& ui) {
            ui.loginButton.onClick(this, &LoginPage::submitLogin);
            ui.queryInput.onTextChanged(this, &LoginPage::onQueryChanged);
        })
    {
    }

    void submitLogin()
    {
        ++loginClicks;
    }

    void onQueryChanged(std::string_view text)
    {
        keyword.assign(text);
    }
};
```

这里有两个关键点：

- 事件直接绑在控件对象上，不再绕回 `Handlers`
- 业务状态成员最好放在 `Page page;` 前面，避免 `page` 构造时回调访问还没构造好的成员

## 3. 分组控件写法

如果你在 markup 里写：

```tui
TextInput(id: form__queryInput, ...)
Button(id: form__loginButton, ...)
Button(id: toolbar__actions__clearButton, ...)
```

生成后直接就是：

```cpp
page.ui.form.queryInput.onTextChanged(this, &LoginPage::onQueryChanged);
page.ui.form.loginButton.onClick(this, &LoginPage::submitLogin);
page.ui.toolbar.actions.clearButton.onClick(this, &LoginPage::clearQuery);
```

## 4. 默认只记这一套

正常页面开发，优先只记这一套：

```cpp
page.ui.xxx->setText(...);
page.ui.xxx.onClick(...);
page.ui.xxx.onTextChanged(...);
page.ui.xxx.onSelectionChanged(...);
```

如果你后面真的需要低层接口，再回头看 [`Markup高级接口`](./Markup高级接口.md)。
