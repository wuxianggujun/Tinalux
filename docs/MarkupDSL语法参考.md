# Tinalux Markup DSL 语法参考

> 更新时间：2026-03-22
> 说明：本文只描述当前源码已经实现并通过 smoke 的语法，不保留旧 `@if / @style / @import` 兼容写法。

## 1. 这套 DSL 是干什么的

Tinalux 的 Markup DSL 用来写界面布局文件，不是通用配置文件格式。

- 适用场景：`.tui` 布局文件，或者直接传给 `LayoutLoader::load(...)` 的 DSL 文本
- 典型用途：声明控件树、样式、组件复用、属性绑定、条件分支、循环展开
- 不适用场景：普通应用配置、构建配置、运行参数文件

像 `if / elseif / else / switch / case / for` 这样的关键字，作用是控制界面节点是否生成，不是给普通配置项做分支判断用的。

## 2. 当前顶层语法

当前 DSL 顶层支持这些声明：

- `import "components/shared.tui"`
- `let gap: 12`
- `style primaryAction: Button(...)`
- `component SearchField(...): TextInput(...)`
- 一个根节点，例如 `VBox(...) { ... }`

旧语法里的 `@import / @style / @component / @if / @for / @res(...)` 已经废弃，当前应直接写关键字本身。

## 3. 一个完整示例

```tui
import "components/shared.tui"

style primaryAction: Button(
    backgroundColor: #FF336699,
    borderRadius: 12
)

component SearchField(placeholder: "Search", currentText: ""):
    TextInput(placeholder, currentText)

VBox(id: root, 12, 8) {
    Label("Dashboard")

    if(${model.showAdvanced}) {
        Button(
            id: deployButton,
            text: "Deploy",
            icon: res("icons/deploy.png"),
            onClick: ${model.onDeploy})
    } elseif(${model.showFallback}) {
        Label("Fallback")
    } else {
        SearchField("Filter", ${model.query})
    }

    Checkbox(id: remember, "Remember me", ${model.rememberMe})
    Toggle(id: rememberMirror, on: ${remember.checked})

    for(item, index in ${model.items}) {
        Label(text: ${item.title + " #" + index})
    }
}
```

## 4. 控件调用

控件调用的基本形式：

```tui
Button(id: submit, text: "Sign In")
```

也可以直接带子节点：

```tui
VBox(id: root, spacing: 12) {
    Label("Title")
    Button("OK")
}
```

## 5. 位置参数

当前已经支持位置参数语法糖，也就是你说的“像 C++ 调实参那样直接传”。

例如：

```tui
Label("Hello")
Button("Deploy", res("icons/deploy.png"))
Checkbox("Remember me", true)
VBox(12, 8) {
    Label("Dashboard")
}
```

规则：

- 位置参数按控件注册的参数顺序匹配
- 命名参数和位置参数可以混写
- 匿名实参会落到下一个还没填过的槽位，不会覆盖前面已经命名过的同名参数
- `id` 不参与位置参数匹配，`Button(id: submit, "提交")` 这种写法是合法的
- 一个控件只有一个主参数时，直接写字符串最省事，例如 `Label("Hello")`
- 多参数控件也可以直接写，但越多越建议改成命名参数，避免读错顺序
- 组件参数同样支持按声明顺序传位置参数

更稳妥的写法：

```tui
Button(text: "Deploy", icon: res("icons/deploy.png"))
```

图标类属性现在同时支持资源路径和内置图标枚举：

```tui
Button(text: "Search", icon: Search)
TextInput(placeholder: "Query", leadingIcon: Search, trailingIcon: Clear)
Dropdown(placeholder: "Pick one", indicatorIcon: ArrowDown)
```

当前已经接通内置图标枚举的常用属性包括：

- `Button.icon`
- `TextInput.leadingIcon`
- `TextInput.trailingIcon`
- `Dropdown.indicatorIcon`
- `Checkbox.checkmarkIcon`
- `Radio.selectionIcon`

也支持混写：

```tui
Slider(id: volume, max: 100, 25, 0, 5)
Button(id: submitButton, iconPlacement: End, "Submit", res("icons/send.png"))
Dropdown(id: choices, selectedIndex: 2, "Pick one", 8, res("icons/chevron.png"))
```

## 6. `id` 规则

`id` 现在是可选的，不是每个控件都必须写。

建议只在这些场景写 `id`：

- 需要用 `findById(...)` 查找控件
- 需要绑定交互
- 需要在别的绑定表达式里引用这个控件

当前合法写法：

```tui
Button(id: submitButton, text: "Submit")
```

当前非法写法：

```tui
Button(id: "submitButton", text: "Submit")
```

也就是说：

- `id` 必须是裸标识符
- 旧的字符串 `id` 设计已经删除
- 如果一个控件不需要被引用，可以完全省略 `id`
- 如果你希望生成分组化的 `page.ui` 结构，可以用 `__` 作为层级分隔符
  例如 `id: form__loginButton` 会生成成 `page.ui.form.loginButton`
  更深一层像 `id: toolbar__actions__clearButton` 会生成成 `page.ui.toolbar.actions.clearButton`

## 7. 绑定表达式

属性绑定仍然统一使用 `${...}`。

例如：

```tui
TextInput(text: ${model.query})
Panel(visible: ${model.showStatus && model.count > 0})
Button(text: ${model.prefix + model.suffix})
TextInput(text: "共 ${model.count} 条记录")
```

当前支持：

- `${model.xxx}` 读取 `ViewModel`
- `${item.xxx}` 读取 `for` 循环作用域
- 普通表达式，例如比较、布尔组合、简单运算、字符串拼接
- 双引号字符串插值，词法阶段会自动降级成普通 binding 表达式
- 双向写回的输入控件绑定
- 事件属性绑定，例如 `onClick: ${model.onSubmit}`、`onSelectionChanged: ${model.onChoiceChanged}`

字符串插值示例：

```tui
TextInput(text: "共 ${model.count} 条记录")
TextInput(text: "启用=${model.enabled}, 比例=${model.scale}")
```

如果需要输出字面量 `${...}`，请转义成 `\${...}`。

当前事件属性要求：

- 必须写成 direct path，例如 `${model.onSubmit}`
- 不能写成 `${model.prefix + model.suffix}` 这种普通表达式
- `ViewModel` 侧需要提供 action 节点

声明式事件绑定的写法就是：

```tui
Button(text: "登录", onClick: ${model.onLoginClicked})
TextInput(
    text: ${model.query},
    onTextChanged: ${model.onQueryChanged},
    onLeadingIconClick: ${model.onSearchClicked})
```

对应的 C++ `ViewModel` 写法：

```cpp
TINALUX_ACTION_SLOT(onLoginClicked, void());
TINALUX_ACTION_SLOT(onQueryChanged, void(std::string_view));
TINALUX_ACTION_SLOT(onDismiss, void());
TINALUX_ACTION_SLOT(onChoiceChanged, void(int));
TINALUX_ACTION_SLOT_PATH(profileSaved, "profile.onSave", void());

onLoginClicked.connect(*viewModel, []() {
    // Button / Dialog / 图标点击这类无 payload 事件可以直接写成 void()
    });

onQueryChanged.connect(*viewModel, [](std::string_view text) {
    // TextInput.onTextChanged 会自动解包成 string_view
});

viewModel->setActions({
    onDismiss([]() {}),
    onChoiceChanged([](int index) {
        // Dropdown / ListView 的选择事件会自动解包成 int
    }),
    profileSaved([]() {}),
});
```

推荐做法不是在业务代码里手工调用 `emitHeader(...)`，而是在 CMake 里直接挂到目标上自动生成：

```cmake
tinalux_target_enable_markup_autogen(
    TARGET MyApp
    INPUT ui/login.tui
    NAMESPACE_PREFIX app::login
    OUTPUT_HEADER login.markup.h
)
```

如果你就是想要最省事的默认路径，单文件现在也可以只写：

```cmake
tinalux_target_enable_markup_autogen(
    TARGET MyApp
    INPUT ui/login.tui
)
```

这时会自动使用这些默认值：

- 输出目录：`${CMAKE_CURRENT_BINARY_DIR}/tinalux_markup/MyApp`
- 输出头：`MyApp.markup.h`
- 命名空间：按输入文件 stem 推导，例如 `ui/login.tui` 会得到 `login::slots`

如果你连 `add_executable(...)` 这层都不想手写，现在也可以直接用更高层入口：

```cmake
tinalux_add_markup_executable(
    TARGET MyApp
    SOURCE src/main.cpp
    LINK_LIBS Tinalux::App
    INPUT ui/login.tui
    COPY_SKIA_DATA
)
```

这层会自动做几件事：

- `add_executable(MyApp ...)`
- 默认链接 `Tinalux::Markup`
- 继续追加你传入的 `LINK_LIBS`
- 自动调用 `tinalux_target_enable_markup_autogen(...)`
- 如果传了 `COPY_SKIA_DATA`，就顺手拷贝运行时所需的 Skia 数据

如果你后面还要继续补 include、宏定义、编译选项，也可以照常对这个 target 再调用标准 CMake 命令。

这会自动把生成头挂到目标上，所以 C++ 里直接：

```cpp
#include "login.markup.h"
```

更底层的 `tinalux_generate_markup_bindings(...)` 仍然保留，适合你要手工控制输出头路径、include guard 或自定义构建链时使用。

如果你不想手写第一页页面类，最省脑子的写法是继续放在同一个高层入口里：

```cmake
tinalux_add_markup_executable(
    TARGET MyApp
    SOURCE src/main.cpp
    INPUT ui/login.tui
    PAGE_SCAFFOLD_OUTPUT src/LoginPage.h
    PAGE_SCAFFOLD_CLASS_NAME LoginPage
    PAGE_SCAFFOLD_ONLY_IF_MISSING
)
```

这条 helper 会复用当前 target 的单文件 markup autogen 配置，生成的骨架默认就是：

- 持有 `Page`
- 固定分成 `initUi(ui)` 和 `bindUi(ui)` 两段
- `initUi(ui)` 里会先生成按控件类型分组的本地别名，方便直接写 `queryInput->...`
- `bindUi(ui)` 里也会先生成同样的本地别名，再通过别名直接绑事件

更适合“先起页面，再自己继续写”，而不是把业务逻辑长期放在生成文件里。

如果你挂的是目录扫描模式，也可以继续放在同一个入口里批量生成：

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

这条 helper 会：

- 递归扫描当前 target 已挂的 `ui/**/*.tui`
- 为每个布局生成一份对应的 `.page.h`
- 类名按相对路径自动展开，例如 `ui/auth/login.tui` 会得到 `AuthLoginPage`
- 额外生成一个总索引头，方便你直接 `#include "app_ui.pages.h"`

底层 helper `tinalux_generate_markup_page_scaffold(...)` /
`tinalux_generate_markup_page_scaffolds_for_directory(...)` 仍然保留，
更适合你要拆分构建步骤或自己控制输出文件时使用。

如果目录下有很多 `.tui`，默认推荐直接挂到目标上自动生成：

```cmake
tinalux_target_enable_markup_autogen(
    TARGET MyApp
    DIRECTORY ui
    NAMESPACE_PREFIX app_ui
)
```

这会自动做几件事：

- 递归扫描 `ui/**/*.tui`
- 为每个 `.tui` 生成一份对应的 `.slots.h`
- 命名空间按相对路径自动展开
  例如 `ui/auth/login.tui` 会生成 `app_ui::auth::login::slots`
- 默认生成目标级总索引头 `MyApp.markup.h`
- 默认输出目录为 `${CMAKE_CURRENT_BINARY_DIR}/tinalux_markup/MyApp`

如果你想自定义总索引头名字，也可以继续显式传：

```cmake
tinalux_target_enable_markup_autogen(
    TARGET MyApp
    DIRECTORY ui
    NAMESPACE_PREFIX app_ui
    INDEX_HEADER app_ui.markup.h
)
```

更底层的 `tinalux_generate_markup_bindings_for_directory(...)` 仍然保留，适合你要自己控制输出目录或自定义构建链时使用。

CMake 自动生成并不神秘，当前就是这 3 步：

1. `add_custom_command(OUTPUT ...)`
   先声明“这个头文件不是手写的，而是由工具生成的”
2. 生成命令调用 `TinaluxMarkupActionHeaderTool`
   把 `.tui` 读进去，输出 `.slots.h`
3. `target_sources(...) + target_include_directories(...)`
   把生成出来的头文件挂到目标上，保证编译前先生成，再让 C++ 正常 `#include`

也就是说，**CMake 本身不懂 TUI**，它只是负责在编译前调用你的生成工具。
如果以后你要自动生成 `.cpp`，原理也是一样：把生成工具输出改成 `.cpp/.h`，然后继续挂到 `target_sources(...)` 上即可。

然后在 C++ 里直接 include 生成头：

```cpp
#include "login.slots.h"

app::login::Page page(theme, [&](auto& ui) {
    ui.loginButton.onClick([&] {
        submitLogin();
    });
    ui.queryInput.onTextChanged([&](std::string_view text) {
        keyword.assign(text);
    });
});

page.ui.loginButton->setEnabled(false);
page.ui.queryInput->setText("seed");
```

目录扫描模式下可以直接 include 总索引头：

```cpp
#include "MyApp.markup.h"

app_ui::auth::login::Page loginPage(theme);
loginPage.ui.loginButton.onClick([&] {
    submitLogin();
});
loginPage.ui.queryInput.onTextChanged([&](std::string_view text) {
    keyword.assign(text);
});

app_ui::settings::profile::Page profilePage(theme);
profilePage.ui.saveButton.onClick([&] {
    saveProfile();
});
profilePage.ui.choiceDropdown.onSelectionChanged([&](int index) {
    selectProfile(index);
});
profilePage.ui.saveButton->setEnabled(true);
```

生成出来的头文件会直接给出：

```cpp
namespace app::login::slots {

class Actions {
public:
    class profile_Group {
    public:
        ::tinalux::markup::ActionSlot<void(int)> onChoiceChanged { "profile.onChoiceChanged" };
    };

    profile_Group profile {};
    ::tinalux::markup::ActionSlot<void()> onLoginClicked { "onLoginClicked" };
    ::tinalux::markup::ActionSlot<void(std::string_view)> onQueryChanged { "onQueryChanged" };
};

inline const Actions actions {};

namespace binding {

struct Handlers {
    class profile_Group {
    public:
        std::function<void(int)> onChoiceChanged {};
    };

    profile_Group profile {};
    std::function<void()> onLoginClicked {};
    std::function<void(std::string_view)> onQueryChanged {};
};

void bind(::tinalux::markup::ViewModel& viewModel, const Handlers& handlers);
std::shared_ptr<::tinalux::markup::ViewModel> makeViewModel(const Handlers& handlers);

} // namespace binding

class Ui {
public:
    class ButtonProxy : public ::tinalux::markup::WidgetRef<::tinalux::ui::Button> {
    public:
        using Base = ::tinalux::markup::WidgetRef<::tinalux::ui::Button>;

        ::tinalux::markup::SignalRef<void()> clicked;
    };

    class TextInputProxy : public ::tinalux::markup::WidgetRef<::tinalux::ui::TextInput> {
    public:
        using Base = ::tinalux::markup::WidgetRef<::tinalux::ui::TextInput>;

        ::tinalux::markup::SignalRef<void(std::string_view)> textChanged;
    };

    ButtonProxy loginButton;
    TextInputProxy queryInput;
};

struct LoadedPage {
    ::tinalux::markup::LoadResult layout;
    Ui ui;

    const std::shared_ptr<::tinalux::markup::ViewModel>& model() const;
};

LoadedPage load(const ::tinalux::ui::Theme& theme);
LoadedPage load(const ::tinalux::ui::Theme& theme, const binding::Handlers& handlers);

} // namespace app::login::slots

namespace app::login {

class Page {
public:
    Page(const ::tinalux::ui::Theme& theme);

    const std::shared_ptr<::tinalux::markup::ViewModel>& model() const;

    ::tinalux::markup::LoadResult layout;
    slots::Ui ui;
};

} // namespace app::login
```

规则说明：

- 正常页面开发如果只想看一套最省事的主路线，直接看 [`Markup页面推荐写法`](./Markup页面推荐写法.md)
- 低层接口如果需要单独查，再看 [`Markup高级接口`](./Markup高级接口.md)
- 只导出最终落到 `ViewModel` 的 direct action path
- 组件参数透传后的实际 path 会被自动展开
- 默认推荐直接构造 `app::login::Page page(theme)`，然后直接在控件对象上写 `page.ui.xxx.onClick(...)`、`page.ui.xxx.onTextChanged(...)` 这类对象方法绑定事件
- 如果你希望在构造时就把页面逻辑集中绑定好，也可以直接写 `app::login::Page page(theme, [&](auto& ui) { ... })`
- 这个 setup 参数本身就是一个 `Ui` 风格上下文，所以可以直接写 `ui.loginButton.onClick(...)`
- 如果你本身就在页面类里写业务逻辑，也可以直接写 `ui.loginButton.onClick(this, &LoginPage::submitLogin)`、`ui.queryInput.onTextChanged(this, &LoginPage::onQueryChanged)`
- 如果 setup 里还需要访问页面状态，也可以直接用 `ui.model()`、`ui.layout` 或 `ui.page()`
- 底层的 signal 字段也仍然保留，所以如果你更喜欢 Qt 式信号接口，依然可以继续写 `page.ui.xxx.clicked.connect(...)`
- 正常页面开发可以先完全忽略 `slots::binding::Handlers`、`slots::load(...)`、`slots::actions...`
- 同一个 signal 可以多次 `connect(...)`，所有已连接的处理器都会按连接顺序依次触发
- 低层接口的完整适用场景和示例统一挪到了 [`Markup高级接口`](./Markup高级接口.md)
- `app::login` 这一层现在只保留页面入口；显式 `Handlers` 已经下沉到 `app::login::slots::binding`，显式加载入口则下沉到 `app::login::slots`
- 旧的 `Controller` 同名方法自动匹配路线已经移除，避免业务层继续堆大而散的逻辑对象
- 当前不会再额外生成“控制层继承文件”这类中间层；高层默认就是 `Page + ui + model()` 这一套
- 所有带 `id` 的控件都会额外生成 `Ui` 强类型控件代理，既能 `->` 访问控件本体，也能直接通过 `.onClick(...) / .onTextChanged(...) / .onSelectionChanged(...)` 这类对象方法绑定声明式事件
- 这些控件代理底层现在统一继承 `WidgetRef<T>`，生成头不再为每个控件重复展开同一套 `get / operator-> / valid` 样板代码
- `Ui` 代理内部会同时持有 `LayoutHandle` 和 `ViewModel`，所以无论你写 `page.ui.xxx.onClick(...)` 还是 `page.ui.xxx.signal.connect(...)`，都不会覆盖 markup 自己的事件刷新链路
- `Ui` 字段代理内部始终通过 `LayoutHandle::findById<T>()` 实时查找，结构重建后不会因为缓存旧指针而失效
- 同一路径如果被多个事件以不同 payload 类型复用，会自动降级成 `void(const core::Value&)`

底层仍然保留 `LayoutActionCatalog::collectFile(...) / emitHeader(...)`，主要给代码生成工具和自定义构建链使用，不推荐业务层直接手写这段流程。

当前已经接通的事件属性包括：

- `Button.onClick`
- `TextInput.onTextChanged`
- `TextInput.onSelectedTextChanged`
- `TextInput.onLeadingIconClick`
- `TextInput.onTrailingIconClick`
- `Dropdown.onSelectionChanged`
- `Checkbox.onToggle`
- `Toggle.onToggle`
- `Radio.onChanged`
- `Slider.onValueChanged`
- `ScrollView.onScrollChanged`
- `ListView.onSelectionChanged`
- `Dialog.onDismiss`
- `RichText` span 对象里的 `onClick`

## 8. `id.property` 引用

当前普通属性绑定和样式绑定里，已经可以直接读取别的控件状态：

```tui
TextInput(id: queryInput, text: ${model.query})
Panel(visible: ${queryInput.text != ""})
TextInput(id: echoField, text: ${queryInput.text})

Checkbox(id: remember, "Remember me", ${model.rememberMe})
Toggle(id: rememberMirror, on: ${remember.checked})

Slider(id: volumeInput, value: ${model.volume}, min: 0, max: 100, step: 1)
ProgressBar(value: ${volumeInput.value})
```

当前已验证的可读控件状态包括：

- `TextInput.text`
- `Checkbox.checked`
- `Toggle.on`
- `Slider.value`
- `Dropdown.selectedIndex`
- `Dropdown.placeholder`
- `visible`

当前限制：

- `id.property` 支持普通属性绑定、样式绑定，以及结构控制条件
- 结构控制里引用 widget id 时，推荐只引用当前构建顺序里已经出现的控件
- 最稳妥的写法是“前面的输入控件影响后面的条件分支”

## 9. 控制流

### `if / elseif / else`

这几个关键字用于控制某一段子树是否生成。

```tui
if(${model.loggedIn}) {
    Label("Welcome")
} elseif(${model.guest}) {
    Label("Guest mode")
} else {
    Button("Sign In")
}
```

用途：

- `if`：主分支
- `elseif`：补充分支
- `else`：兜底分支

它们是给布局文件用的，不是给普通配置文件用的。

也支持用前面已经声明过的控件状态来驱动分支：

```tui
TextInput(id: query, text: ${model.query})

if(${query.text}) {
    Button(text: "Clear")
}
```

组件参数也可以直接驱动结构控制：

```tui
component SearchPanel(showAdvanced: ${model.showAdvanced}): VBox {
    if(${showAdvanced}) {
        TextInput(id: advancedQuery, text: ${model.advancedQuery})
    }
}
```

### `switch / case / else`

多分支枚举场景可以直接写 `switch`：

```tui
switch(${model.phase}) {
    case(Idle) {
        Label("Idle")
    }
    case("running") {
        Label("Running")
    }
    case(${model.donePhase}) {
        Label("Done")
    }
    else {
        Label("Fallback")
    }
}
```

规则：

- `switch(...)` 里必须是绑定表达式，也就是 `${...}`
- `case(...)` 支持字符串、数字、布尔、颜色、裸标识符、`${...}` 动态表达式
- 裸标识符会按文本常量处理，适合写 `case(Idle)` 这种枚举风格分支
- `else` 是可选兜底分支

### `for`

`for` 用来把一组数据展开成多个节点：

```tui
for(item in ${model.items}) {
    Label(${item.title})
}
```

循环变量只在当前 `for` 块里有效。

也支持索引变量：

```tui
for(item, index in ${model.items}) {
    Label(text: ${index + ": " + item.title})
}
```

组件参数同样可以作为 `for` 的数据源：

```tui
component ActionList(source: ${model.actions}): VBox {
    for(item in ${source}) {
        Label(text: ${item.title})
    }
}
```

## 10. 资源引用

资源引用使用 `res("...")`：

```tui
Button("Deploy", res("icons/deploy.png"))
ImageWidget(source: res("images/hero.png"), fit: Cover)
```

它适合图片、图标等资源路径，不需要再写旧的 `@res(...)`。

## 11. 样式与组件

样式定义：

```tui
style primaryAction: Button(
    backgroundColor: #FF336699,
    borderRadius: 12
)
```

组件定义：

```tui
component SearchField(placeholder: "Search", currentText: ""):
    TextInput(placeholder, currentText)
```

组件实例化：

```tui
SearchField("Email", ${model.email})
```

内联样式块也支持表达式：

```tui
Button(
    text: "Run",
    style: {
        borderRadius: ${model.radius * model.scale}
    }
)
```

## 12. Slot

组件内容投递仍使用 `Slot()`：

```tui
component CardShell: Panel(id: shell) {
    Slot()
}
```

命名 slot 也支持：

```tui
component DialogFrame: Panel(id: frame) {
    Panel {
        Slot(name: body)
    }
    Panel {
        Slot(name: "footer")
    }
}
```

## 13. 迁移提示

旧写法：

```tui
@import "shared.tui"
@style primary: Button(...)
@component SearchField(...): TextInput(...)
@if(${model.visible}) { ... }
@for(item in ${model.items}) { ... }
@res("icons/search.png")
```

新写法：

```tui
import "shared.tui"
style primary: Button(...)
component SearchField(...): TextInput(...)
if(${model.visible}) { ... }
for(item in ${model.items}) { ... }
res("icons/search.png")
```

## 14. 当前推荐写法

- 正常页面开发直接按 [`Markup页面推荐写法`](./Markup页面推荐写法.md) 写
- 低层 `Handlers / slots::load / slots::actions...` 统一按 [`Markup高级接口`](./Markup高级接口.md) 查
- 能省 `id` 就省，只给需要引用的控件写
- 单主参数控件优先直接写位置参数，例如 `Label("Hello")`
- 参数一多就改用命名参数，避免顺序不清
- 读取 `ViewModel` 用 `${model.xxx}`
- 读取别的控件状态用 `${someId.someProperty}`
- 控制结构分支时用 `if / elseif / else / for`
