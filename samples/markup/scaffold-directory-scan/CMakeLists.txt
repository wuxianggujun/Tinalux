tinalux_add_markup_executable(
    TARGET TinaluxMarkupScaffoldDirectoryScanSample
    SOURCE src/main.cpp
    DIRECTORY ui
    PAGE_SCAFFOLD_OUTPUT_DIRECTORY generated_pages
    PAGE_SCAFFOLD_INDEX_HEADER generated_scaffold.pages.h
    PAGE_SCAFFOLD_ONLY_IF_MISSING
)
