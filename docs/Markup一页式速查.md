# Markup 一页式速查

这份文档只回答一个问题：

**普通页面开发，到底只需要记什么？**

## 1. 只记两个 CMake 入口

日常开发只记这两个就够了：

- `tinalux_add_markup_executable(...)`
- `tinalux_target_enable_markup_autogen(...)`

理解方式很简单：

- 你想少写样板，就优先用 `tinalux_add_markup_executable(...)`
- 你已经自己有 target 了，就用 `tinalux_target_enable_markup_autogen(...)`

## 2. 只记这三种抄法

单文件，新建 target：

```cmake
tinalux_add_markup_executable(
    TARGET LoginDemo
    SOURCE src/main.cpp
    INPUT ui/login.tui
)
```

单文件，已有 target：

```cmake
add_executable(LoginDemo src/main.cpp)
target_link_libraries(LoginDemo PRIVATE Tinalux::Markup)

tinalux_target_enable_markup_autogen(
    TARGET LoginDemo
    INPUT ui/login.tui
)
```

目录扫描，顺手起页面骨架：

```cmake
tinalux_add_markup_executable(
    TARGET MyApp
    SOURCE src/main.cpp
    DIRECTORY ui
    PAGE_SCAFFOLD_OUTPUT_DIRECTORY src/pages
    PAGE_SCAFFOLD_ONLY_IF_MISSING
)
```

一句话记忆：

- 单文件用 `INPUT`
- 批量扫描用 `DIRECTORY`
- 想省脑子就优先用 `tinalux_add_markup_executable(...)`

## 3. C++ 只记这一种页面写法

普通页面开发，主路线就是：

- 页面类里持有 `Page`
- 直接拿 `ui.xxx` 控件对象绑事件
- 不要先退回 `Handlers / slots::actions...`

```cpp
#include "login.markup.h"

class LoginPage {
public:
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
    }

    void onQueryChanged(std::string_view text)
    {
        keyword.assign(text);
    }
};
```

如果你生成的是页面骨架，默认也会朝这个方向靠：

- `initUi(ui)`
- `bindUi(ui)`
- 先起本地控件别名，再直接绑事件

想直接抄代码，先看模板区：

- [`samples/markup/README.md`](../samples/markup/README.md)
- [`samples/markup/single-file/src/main.cpp`](../samples/markup/single-file/src/main.cpp)
- [`samples/markup/directory-scan/src/main.cpp`](../samples/markup/directory-scan/src/main.cpp)

如果你想看已经挂到 smoke 里的受测版本，再看：

- [`tests/markup/mental_model_smoke.cpp`](../tests/markup/mental_model_smoke.cpp)
- [`tests/markup/mental_model_scan_smoke.cpp`](../tests/markup/mental_model_scan_smoke.cpp)
- [`tests/CMakeLists.txt`](../tests/CMakeLists.txt)

## 4. 哪些东西大多数时候不用管

下面这些概念，普通页面开发大多数时候可以先忽略：

- `NAMESPACE`
- `NAMESPACE_PREFIX`
- `Handlers`
- `slots::load(...)`
- `slots::actions...`
- 独立 scaffold helper

特别是命名空间：

- 单文件默认会自动按文件名推导
- 目录扫描只有你想统一前缀时，才需要主动写 `NAMESPACE_PREFIX`
- 真正需要手写命名空间，通常是防重名或工具链场景

## 5. 什么时候才需要看高级接口

只有下面这些场景，才建议继续往低层看：

- 你在写测试
- 你在写代码生成工具
- 你要手工拼 `ViewModel`
- 你要把 markup 接进自己现有的构建链

这时再去看：

- [`Markup高级接口`](./Markup高级接口.md)

## 6. 推荐阅读顺序

如果你现在已经觉得信息很多，就按这个顺序看：

1. 先看这份 [`Markup一页式速查`](./Markup一页式速查.md)
2. 再看 [`Markup页面推荐写法`](./Markup页面推荐写法.md)
3. 只有需要低层细节时，才看 [`Markup高级接口`](./Markup高级接口.md)

如果你要看 CMake 模块源码，就按这个顺序：

1. `TinaluxMarkupTools.cmake`
2. `TinaluxMarkupAutogenTools.cmake`
3. `TinaluxMarkupBindingsTools.cmake` / `TinaluxMarkupScaffoldTools.cmake`
4. `TinaluxMarkupCommonTools.cmake`
