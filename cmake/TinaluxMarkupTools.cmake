include_guard(GLOBAL)

# 普通页面开发只需要记住这两个高层入口：
# - tinalux_target_enable_markup_autogen(...)
# - tinalux_add_markup_executable(...)
#
# 推荐阅读顺序：
# 1. 先看这个文件
# 2. 需要高层实现细节时，再看 `TinaluxMarkupAutogenTools.cmake`
# 3. 只有排查低层生成链时，才继续看 `Bindings / Scaffold / Common`
#
# 低层 / 高级 helper 仍然可用，但实现统一收口到
# `TinaluxMarkupAdvancedTools.cmake`，避免这个入口文件继续堆满细节。
include(TinaluxMarkupAdvancedTools)
