# Markup 一页式速查

这份文档只回答一个问题：

**普通页面开发，到底只需要记什么？**

## 0. 想最快验证主路线，先跑这个

如果你现在不想先读概念，想先确认仓库里的推荐主路线没偏，直接跑：

```powershell
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug --config Debug --target TinaluxRunMarkupMentalModelExamples
```

这会直接构建并运行两份“官方最小基准示例”：

- [`tests/markup/mental_model_smoke.cpp`](../tests/markup/mental_model_smoke.cpp)
- [`tests/markup/mental_model_scan_smoke.cpp`](../tests/markup/mental_model_scan_smoke.cpp)

如果你跑完之后，脑子里冒出来的是这些问题：

- `page.ui` 和 `ui.xxx` 到底什么关系
- 命名空间什么时候才需要自己写
- 什么时候该选 scaffold
- 为什么不推荐先看 `Handlers / slots::actions`

直接跳：

- [`Markup常见问题`](./Markup常见问题.md)

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

## 3. 真要开工时，只看这个选型图

这四个名字不是四套 API，只是四种起手模板：

```text
我现在想怎么开工？
├── 我想自己写页面类
│   ├── 只有一个 .tui            -> samples/markup/single-file
│   └── 有一整个 ui/ 目录        -> samples/markup/directory-scan
└── 我想让生成器先起页面骨架
    ├── 只有一个 .tui            -> samples/markup/scaffold-single-file
    └── 有一整个 ui/ 目录        -> samples/markup/scaffold-directory-scan
```

你平时只需要先回答两个问题：

- 你是想自己写页面类，还是让生成器先起骨架
- 你面对的是单文件，还是目录扫描

## 4. C++ 只记这一种页面写法

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
- [`samples/markup/scaffold-single-file/src/main.cpp`](../samples/markup/scaffold-single-file/src/main.cpp)
- [`samples/markup/scaffold-directory-scan/src/main.cpp`](../samples/markup/scaffold-directory-scan/src/main.cpp)

如果你想看已经挂到 smoke 里的受测版本，再看：

- [`tests/markup/mental_model_smoke.cpp`](../tests/markup/mental_model_smoke.cpp)
- [`tests/markup/mental_model_scan_smoke.cpp`](../tests/markup/mental_model_scan_smoke.cpp)
- [`tests/CMakeLists.txt`](../tests/CMakeLists.txt)

上面那条 `TinaluxRunMarkupMentalModelExamples` 命令，
跑的就是这里这两份受测版最小基准示例。

## 5. 哪些东西大多数时候不用管

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

把它理解成“生成代码放在哪个名字下面”就够了，例如：

```text
ui/login.tui                  -> login::Page
ui/auth/login.tui             -> auth::login::Page
ui/auth/login.tui + NAMESPACE_PREFIX app_ui
                              -> app_ui::auth::login::Page
```

它的作用主要只有两个：

- 防止不同页面重名
- 给一批生成页面挂一个统一前缀

## 6. 什么时候才需要看高级接口

只有下面这些场景，才建议继续往低层看：

- 你在写测试
- 你在写代码生成工具
- 你要手工拼 `ViewModel`
- 你要把 markup 接进自己现有的构建链

这时再去看：

- [`Markup高级接口`](./Markup高级接口.md)

## 7. 推荐阅读顺序

如果你现在已经觉得信息很多，就按这个顺序看：

1. 先看这份 [`Markup一页式速查`](./Markup一页式速查.md)
2. 再看 [`samples/markup/README.md`](../samples/markup/README.md)
3. 还带着概念困惑时，看 [`Markup常见问题`](./Markup常见问题.md)
4. 再看 [`Markup页面推荐写法`](./Markup页面推荐写法.md)
5. 只有需要低层细节时，才看 [`Markup高级接口`](./Markup高级接口.md)

如果你要看 CMake 模块源码，就按这个顺序：

1. `TinaluxMarkupTools.cmake`
2. `TinaluxMarkupAutogenTools.cmake`
3. `TinaluxMarkupBindingsTools.cmake` / `TinaluxMarkupScaffoldTools.cmake`
4. `TinaluxMarkupCommonTools.cmake`
