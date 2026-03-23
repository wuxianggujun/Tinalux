include_guard(GLOBAL)

# 如果你只是普通页面开发，先停在 `TinaluxMarkupTools.cmake` 即可。
# 这个文件不是新的高层入口，只是把低层模块聚合起来。
# 只有在下面这些场景，才值得继续往下看：
# - 你在排查代码生成链
# - 你在接自己的工具链
# - 你真的要碰低层 helper
#
# 否则，优先回去看：
# - `tinalux_add_markup_executable(...)`
# - `tinalux_target_enable_markup_autogen(...)`
# - `samples/markup/*`
#
# `TinaluxMarkupTools.cmake` 会先 include 这里。
# 这个文件现在只做高级模块聚合，具体实现继续按主题拆分。
#
# 文件职责：
# - Common: 名字推导、默认路径、类名/命名空间小工具
# - Bindings: `.tui -> .slots.h`
# - Scaffold: `.tui -> .page.h`
# - Autogen: 高层入口，负责把上面几块拼起来
include(TinaluxMarkupCommonTools)
include(TinaluxMarkupBindingsTools)
include(TinaluxMarkupScaffoldTools)
include(TinaluxMarkupAutogenTools)
