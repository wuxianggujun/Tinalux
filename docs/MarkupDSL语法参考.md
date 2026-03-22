# Tinalux Markup DSL 语法参考

> 更新时间：2026-03-22
> 说明：本文只描述当前源码已经实现并通过 smoke 的语法，不保留旧 `@if / @style / @import` 兼容写法。

## 1. 这套 DSL 是干什么的

Tinalux 的 Markup DSL 用来写界面布局文件，不是通用配置文件格式。

- 适用场景：`.tui` 布局文件，或者直接传给 `LayoutLoader::load(...)` 的 DSL 文本
- 典型用途：声明控件树、样式、组件复用、属性绑定、条件分支、循环展开
- 不适用场景：普通应用配置、构建配置、运行参数文件

像 `if / elseif / else / for` 这样的关键字，作用是控制界面节点是否生成，不是给普通配置项做分支判断用的。

## 2. 当前顶层语法

当前 DSL 顶层支持这些声明：

- `import "components/shared.tui"`
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
        Button(id: deployButton, "Deploy", res("icons/deploy.png"))
    } elseif(${model.showFallback}) {
        Label("Fallback")
    } else {
        SearchField("Filter", ${model.query})
    }

    Checkbox(id: remember, "Remember me", ${model.rememberMe})
    Toggle(id: rememberMirror, on: ${remember.checked})

    for(item in ${model.items}) {
        Label(${item.title})
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

## 7. 绑定表达式

属性绑定仍然统一使用 `${...}`。

例如：

```tui
TextInput(text: ${model.query})
Panel(visible: ${model.showStatus && model.count > 0})
Button(text: ${model.prefix + model.suffix})
```

当前支持：

- `${model.xxx}` 读取 `ViewModel`
- `${item.xxx}` 读取 `for` 循环作用域
- 普通表达式，例如比较、布尔组合、简单运算、字符串拼接
- 双向写回的输入控件绑定

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

- `id.property` 只支持普通属性绑定和样式绑定
- `id.property` 目前不支持 `if / elseif / else / for` 这种结构控制
- 也就是说，不支持用控件状态驱动整棵结构重建

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

### `for`

`for` 用来把一组数据展开成多个节点：

```tui
for(item in ${model.items}) {
    Label(${item.title})
}
```

循环变量只在当前 `for` 块里有效。

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

- 能省 `id` 就省，只给需要引用的控件写
- 单主参数控件优先直接写位置参数，例如 `Label("Hello")`
- 参数一多就改用命名参数，避免顺序不清
- 读取 `ViewModel` 用 `${model.xxx}`
- 读取别的控件状态用 `${someId.someProperty}`
- 控制结构分支时用 `if / elseif / else / for`
