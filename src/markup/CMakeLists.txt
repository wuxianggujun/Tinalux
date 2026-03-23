add_library(TinaluxMarkup STATIC
    BindingExpression.cpp
    DocumentLoader.cpp
    Lexer.cpp
    Parser.cpp
    ViewModel.cpp
    LayoutBuilder.cpp
    LayoutActionCatalog.cpp
    BuiltinRegistrations.cpp
    LayoutLoader.cpp
)

add_library(Tinalux::Markup ALIAS TinaluxMarkup)

target_include_directories(TinaluxMarkup
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(TinaluxMarkup
    PUBLIC
        Tinalux::Core
        Tinalux::UI
)

if(MSVC)
    target_compile_options(TinaluxMarkup PRIVATE /MP1)
endif()

add_executable(TinaluxMarkupActionHeaderTool
    ActionHeaderTool.cpp
)

target_link_libraries(TinaluxMarkupActionHeaderTool
    PRIVATE
        Tinalux::Markup
)
