include_guard(GLOBAL)

# 普通页面开发只需要记住这两个高层入口：
# - tinalux_target_enable_markup_autogen(...)
# - tinalux_add_markup_executable(...)
#
# 低层 / 高级 helper 仍然可用，但实现统一收口到
# `TinaluxMarkupAdvancedTools.cmake`，避免这个入口文件继续堆满细节。
include(TinaluxMarkupAdvancedTools)
