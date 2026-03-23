include_guard(GLOBAL)

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

