# Tinalux Markup DSL 语法参考

> 更新时间：2026-03-23
> 说明：本文只描述当前源码已经实现并通过 smoke 的 DSL 语法。

这份文档只回答一个问题：

**`.tui` 文件到底该怎么写。**

如果你要看页面怎么接到 C++ / CMake，请直接跳转：

- 普通页面写法：[`Markup页面推荐写法`](./Markup页面推荐写法.md)
- 低层接口：[`Markup高级接口`](./Markup高级接口.md)

## 1. 先看这张分层图

```text
我现在要查什么？
├── 最常用的 .tui 写法
│   ├── 控件调用 / 位置参数
│   ├── id 规则
│   ├── ${...} 绑定
│   ├── if / elseif / else / for
│   └── res("...")
├── 较少使用但已支持
│   ├── switch / case
│   ├── style
│   ├── component
│   ├── Slot
│   └── id.property
└── CMake / Page / 自动生成
    └── 这不属于 DSL 语法，去看别的文档
```

普通页面开发，大多数时候只会用到第一层。

## 2. 这套 DSL 是干什么的

Tinalux 的 Markup DSL 用来写界面布局，不是通用配置文件格式。

适合：

- `.tui` 布局文件
- 直接传给 `LayoutLoader::load(...)` 的 DSL 文本

不适合：

- 普通应用配置
- 构建配置
- 运行参数文件

像 `if / elseif / else / switch / case / for` 这些关键字，
作用是控制**界面节点是否生成**，不是给普通配置做分支。

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

## 4. 最常用语法

### 4.1 顶层能写什么

当前顶层支持这些声明：

- `import "components/shared.tui"`
- `let gap: 12`
- `style primaryAction: Button(...)`
- `component SearchField(...): TextInput(...)`
- 一个根节点，例如 `VBox(...) { ... }`

旧的 `@import / @style / @component / @if / @for / @res(...)`
已经废弃，现在直接写关键字本身。

### 4.2 控件调用

基本形式：

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

### 4.3 位置参数

位置参数语法已经支持，写法像普通函数调用。

```tui
Label("Hello")
Button("Deploy", res("icons/deploy.png"))
Checkbox("Remember me", true)
VBox(12, 8) {
    Label("Dashboard")
}
```

规则：

- 位置参数按控件注册顺序匹配
- 可以和命名参数混写
- `id` 不参与位置参数匹配
- 参数一多就建议改回命名参数，避免读错顺序

更稳妥的写法：

```tui
Button(text: "Deploy", icon: res("icons/deploy.png"))
```

图标类属性同时支持资源路径和内置图标枚举：

```tui
Button(text: "Search", icon: Search)
TextInput(placeholder: "Query", leadingIcon: Search, trailingIcon: Clear)
Dropdown(placeholder: "Pick one", indicatorIcon: ArrowDown)
```

### 4.4 `id` 规则

`id` 现在是可选的，不是每个控件都必须写。

建议只在这些场景写 `id`：

- 需要交互绑定
- 需要在别的表达式里引用
- 需要生成强类型 `ui` 代理
- 需要 `findById(...)`

合法写法：

```tui
Button(id: submitButton, text: "Submit")
```

非法写法：

```tui
Button(id: "submitButton", text: "Submit")
```

规则总结：

- `id` 必须是裸标识符
- 不需要引用时可以完全省略
- 用 `__` 可以表达分组层级

例如：

```tui
Button(id: form__loginButton, text: "Login")
Button(id: toolbar__actions__clearButton, text: "Clear")
```

生成后会更接近：

```cpp
page.ui.form.loginButton
page.ui.toolbar.actions.clearButton
```

### 4.5 绑定表达式 `${...}`

属性绑定统一使用 `${...}`。

```tui
TextInput(text: ${model.query})
Panel(visible: ${model.showStatus && model.count > 0})
Button(text: ${model.prefix + model.suffix})
TextInput(text: "共 ${model.count} 条记录")
```

当前支持：

- `${model.xxx}` 读取 `ViewModel`
- `${item.xxx}` 读取 `for` 循环作用域
- 比较、布尔组合、简单运算、字符串拼接
- 双引号字符串插值
- 输入控件双向写回
- 事件属性绑定，例如 `onClick: ${model.onSubmit}`

如果你需要输出字面量 `${...}`，请写 `\${...}`。

事件属性要求：

- 必须写成 direct path，例如 `${model.onSubmit}`
- 不能写成普通表达式

示例：

```tui
Button(text: "登录", onClick: ${model.onLoginClicked})
TextInput(
    text: ${model.query},
    onTextChanged: ${model.onQueryChanged},
    onLeadingIconClick: ${model.onSearchClicked})
```

### 4.6 `if / elseif / else / for`

这几个是最常用的结构控制语法。

`if / elseif / else`：

```tui
if(${model.loggedIn}) {
    Label("Welcome")
} elseif(${model.guest}) {
    Label("Guest mode")
} else {
    Button("Sign In")
}
```

`for`：

```tui
for(item in ${model.items}) {
    Label(${item.title})
}
```

带索引：

```tui
for(item, index in ${model.items}) {
    Label(text: ${index + ": " + item.title})
}
```

### 4.7 资源引用

资源引用统一使用 `res("...")`：

```tui
Button("Deploy", res("icons/deploy.png"))
ImageWidget(source: res("images/hero.png"), fit: Cover)
```

## 5. 较少使用但已支持

### 5.1 `id.property`

普通属性绑定和样式绑定里，可以读取别的控件状态：

```tui
TextInput(id: queryInput, text: ${model.query})
Panel(visible: ${queryInput.text != ""})
TextInput(id: echoField, text: ${queryInput.text})

Checkbox(id: remember, "Remember me", ${model.rememberMe})
Toggle(id: rememberMirror, on: ${remember.checked})
```

当前已验证的可读状态包括：

- `TextInput.text`
- `Checkbox.checked`
- `Toggle.on`
- `Slider.value`
- `Dropdown.selectedIndex`
- `Dropdown.placeholder`
- `visible`

最稳妥的写法仍然是：

- 前面的控件影响后面的分支或属性

### 5.2 `switch / case / else`

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

- `switch(...)` 里必须是 `${...}`
- `case(...)` 支持字符串、数字、布尔、颜色、裸标识符、`${...}`
- `else` 是可选兜底分支

### 5.3 `style`

样式定义：

```tui
style primaryAction: Button(
    backgroundColor: #FF336699,
    borderRadius: 12
)
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

### 5.4 `component`

组件定义：

```tui
component SearchField(placeholder: "Search", currentText: ""):
    TextInput(placeholder, currentText)
```

组件实例化：

```tui
SearchField("Email", ${model.email})
```

### 5.5 `Slot`

组件内容投递使用 `Slot()`：

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

## 6. 这些不属于 DSL 本身

下面这些内容经常和 DSL 一起出现，但它们不是 DSL 语法本身：

- `tinalux_add_markup_executable(...)`
- `tinalux_target_enable_markup_autogen(...)`
- `Page / ui / page.ui.xxx`
- 页面骨架 `initUi(ui)` / `bindUi(ui)`
- `Handlers / slots::load / slots::actions / attachUi(...)`

查这些内容时，不要继续留在这份文档里：

- 普通页面接法 -> [`Markup页面推荐写法`](./Markup页面推荐写法.md)
- 低层接口 -> [`Markup高级接口`](./Markup高级接口.md)

## 7. 迁移提示

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

## 8. 当前推荐写法

普通页面开发，优先只记这些：

- 能省 `id` 就省，只给需要引用的控件写
- 单主参数控件优先直接写位置参数，例如 `Label("Hello")`
- 参数一多就改用命名参数
- 读取 `ViewModel` 用 `${model.xxx}`
- 读取别的控件状态用 `${someId.someProperty}`
- 结构控制优先记 `if / elseif / else / for`
- 资源引用统一记 `res("...")`

如果你现在已经觉得信息很多，可以按这个顺序读：

1. 先看本页的“最常用语法”
2. 真遇到特定场景，再看“较少使用但已支持”
3. 如果问题已经变成 CMake / Page / 低层绑定，就跳去别的文档
