tinalux_add_markup_executable(
    TARGET TinaluxMarkupScaffoldSingleFileSample
    SOURCE src/main.cpp
    INPUT ui/login.tui
    PAGE_SCAFFOLD_OUTPUT generated_page_scaffold.h
    PAGE_SCAFFOLD_CLASS_NAME LoginPage
    PAGE_SCAFFOLD_ONLY_IF_MISSING
)
