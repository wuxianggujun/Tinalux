# Markup 页面推荐写法

这份文档只做一件事：

**给普通页面开发提供可直接抄的代码。**

如果你现在还在分不清该用哪套模板，先回：

- [`Markup一页式速查`](./Markup一页式速查.md)
- [`samples/markup/README.md`](../samples/markup/README.md)

如果你已经在看 `Handlers / slots::load / slots::actions / attachUi(...)`，
说明你已经进入低层场景了，直接改看：

- [`Markup高级接口`](./Markup高级接口.md)

## 1. 普通页面开发，先记这一句话

默认主路线就是：

- CMake 挂自动生成
- 页面类里持有 `Page`
- 构造时直接写 `ui.xxx.onXxx(...)`
- 构造完成后继续用 `page.ui.xxx`

别先退回 `Handlers`、`slots::actions`、独立 scaffold helper。

## 2. 直接抄这四段

### 2.1 单文件，最小起手式

```cmake
tinalux_add_markup_executable(
    TARGET LoginDemo
    SOURCE src/main.cpp
    INPUT ui/login.tui
)
```

适合：

- 一个页面先跑通
- 还不想拆目录

### 2.2 目录扫描，页面开始变多

```cmake
tinalux_add_markup_executable(
    TARGET MyApp
    SOURCE src/main.cpp
    DIRECTORY ui
)
```

适合：

- 已经有 `ui/auth/login.tui`
- 已经有 `ui/settings/profile.tui`

### 2.3 单文件，但让生成器顺手起页面骨架

```cmake
tinalux_add_markup_executable(
    TARGET LoginDemo
    SOURCE src/main.cpp
    INPUT ui/login.tui
    PAGE_SCAFFOLD_OUTPUT src/LoginPage.h
    PAGE_SCAFFOLD_CLASS_NAME LoginPage
    PAGE_SCAFFOLD_ONLY_IF_MISSING
)
```

适合：

- 你不想手写页面类外壳
- 你想直接从 `initUi(ui)` / `bindUi(ui)` 开始改

### 2.4 目录扫描，同时批量起页面骨架

```cmake
tinalux_add_markup_executable(
    TARGET MyApp
    SOURCE src/main.cpp
    DIRECTORY ui
    PAGE_SCAFFOLD_OUTPUT_DIRECTORY src/pages
    PAGE_SCAFFOLD_INDEX_HEADER app.pages.h
    PAGE_SCAFFOLD_ONLY_IF_MISSING
)
```

适合：

- 页面已经按目录拆开
- 你想统一生成一批页面类起手式

## 3. C++ 页面类直接抄这一段

```cpp
#include "login.markup.h"

class LoginPage {
public:
    std::string keyword;
    login::Page page;

    explicit LoginPage(const tinalux::ui::Theme& theme)
        : page(theme, [&](auto& ui) {
            ui.loginButton.onClick(this, &LoginPage::submitLogin);
            ui.queryInput.onTextChanged(this, &LoginPage::onQueryChanged);
        })
    {
    }

    void submitLogin()
    {
    }

    void onQueryChanged(std::string_view text)
    {
        keyword.assign(text);
    }
};
```

这里真正需要记的只有两句：

- 构造阶段：`ui.xxx.onXxx(...)`
- 构造完成后：`page.ui.xxx`

## 4. 如果 id 有分组，生成后就直接按对象层级访问

如果你写：

```tui
TextInput(id: form__queryInput, ...)
Button(id: form__loginButton, ...)
Button(id: toolbar__actions__clearButton, ...)
```

那你就直接写：

```cpp
page.ui.form.queryInput.onTextChanged(this, &LoginPage::onQueryChanged);
page.ui.form.loginButton.onClick(this, &LoginPage::submitLogin);
page.ui.toolbar.actions.clearButton.onClick(this, &LoginPage::clearQuery);
```

不要再自己回头拼 action path。

## 5. 常见事件，先只记这些

```cpp
ui.button.onClick(this, &Page::submit);
ui.input.onTextChanged(this, &Page::setKeyword);
ui.dropdown.onSelectionChanged(this, &Page::setIndex);
ui.checkbox.onToggle(this, &Page::setEnabled);
ui.slider.onValueChanged(this, &Page::setVolume);
ui.dialog.onDismiss(this, &Page::closeDialog);
```

不需要一上来把所有高级接口都背下来。

## 6. 什么时候不用往下看

只要你满足下面任意一条，就别再往低层钻：

- 你只是写业务页面
- 你已经有 `Page`
- 你想按控件对象思维开发
- 你只是想少写样板

这时就停在这里，或者直接去抄模板区：

- [`samples/markup/single-file`](../samples/markup/single-file)
- [`samples/markup/directory-scan`](../samples/markup/directory-scan)
- [`samples/markup/scaffold-single-file`](../samples/markup/scaffold-single-file)
- [`samples/markup/scaffold-directory-scan`](../samples/markup/scaffold-directory-scan)

## 7. 什么时候才需要继续往下看

只有这些场景，才建议继续看别的文档：

- 你分不清模板怎么选
  -> 看 [`Markup一页式速查`](./Markup一页式速查.md)
- 你要查 `Handlers / slots::load / slots::actions / attachUi(...)`
  -> 看 [`Markup高级接口`](./Markup高级接口.md)
- 你要查 DSL 语法细节
  -> 看 [`MarkupDSL语法参考`](./MarkupDSL语法参考.md)
