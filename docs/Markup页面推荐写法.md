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

如果你想先自动起一个“推荐写法”的页面类骨架，**优先还是直接写在同一个 `tinalux_add_markup_executable(...)` 里**：

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

这几个参数的定位是**起手式**：

- 它会根据当前 target 已挂的单文件 markup 自动生成一个页面类
- 生成出来的代码默认就是 `Page + ui.xxx.onXxx(...)`
- 生成骨架会固定分成 `initUi(ui)` 和 `bindUi(ui)` 两段，事件默认放在 `bindUi(ui)` 里用内联 lambda 占位
- `PAGE_SCAFFOLD_ONLY_IF_MISSING` 打开后，文件已经存在就不会覆盖你的手改内容

如果你挂的是目录扫描模式，也一样优先写在同一个入口里：

```cmake
tinalux_add_markup_executable(
    TARGET MyApp
    SOURCE src/main.cpp
    DIRECTORY ui
    NAMESPACE_PREFIX app_ui
    PAGE_SCAFFOLD_OUTPUT_DIRECTORY src/pages
    PAGE_SCAFFOLD_INDEX_HEADER app_ui.pages.h
    PAGE_SCAFFOLD_ONLY_IF_MISSING
)
```

这会把 `ui/auth/login.tui` 之类的文件批量生成成：

- `src/pages/auth/login.page.h`
- `src/pages/settings/profile.page.h`
- 总索引头 `src/pages/app_ui.pages.h`

如果你真的要把生成动作拆出去单独调用，`tinalux_generate_markup_page_scaffold(...)` /
`tinalux_generate_markup_page_scaffolds_for_directory(...)` 这两个 helper 还在，
但它们现在更适合高级场景，不再是推荐主路线。

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

## 4. 常用事件对象方法

除了 `onClick(...)`、`onTextChanged(...)`，常见控件现在也可以直接在对象上绑事件：

```cpp
class SettingsPage {
public:
    generated_controls::Page page;
    bool subscribed = false;
    float volume = 0.0f;

    explicit SettingsPage(const tinalux::ui::Theme& theme)
        : page(theme, [&](auto& ui) {
            ui.searchInput.onLeadingIconClick(this, &SettingsPage::openSearch);
            ui.searchInput.onTrailingIconClick(this, &SettingsPage::clearKeyword);
            ui.subscribeBox.onToggle(this, &SettingsPage::setSubscribed);
            ui.syncToggle.onToggle(this, &SettingsPage::setSyncEnabled);
            ui.volumeSlider.onValueChanged(this, &SettingsPage::setVolume);
            ui.confirmDialog.onDismiss(this, &SettingsPage::closeDialog);
        })
    {
    }

    void openSearch();
    void clearKeyword();
    void setSubscribed(bool checked);
    void setSyncEnabled(bool enabled);
    void setVolume(float value);
    void closeDialog();
};
```

推荐记忆方式只有两句：

- 构造页面时，用 `ui.xxx.onXxx(...)` 绑定事件
- 页面构造完成后，用 `page.ui.xxx->...` 操作控件对象

## 5. 默认只记这一套

正常页面开发，优先只记这一套：

```cpp
page.ui.xxx->setText(...);
page.ui.xxx.onClick(...);
page.ui.xxx.onTextChanged(...);
page.ui.xxx.onSelectionChanged(...);
```

如果你后面真的需要低层接口，再回头看 [`Markup高级接口`](./Markup高级接口.md)。
