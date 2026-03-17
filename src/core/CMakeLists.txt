add_library(TinaluxCore STATIC
    core.cpp
    Log.cpp
)

add_library(Tinalux::Core ALIAS TinaluxCore)

target_include_directories(TinaluxCore
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(TinaluxCore PUBLIC cxx_std_20)

if(ANDROID)
    # Android NDK clang 21 still trips over fmt's consteval-heavy format path.
    target_compile_definitions(TinaluxCore PUBLIC
        FMT_CONSTEVAL=
        FMT_USE_NONTYPE_TEMPLATE_ARGS=0
    )
endif()

target_link_libraries(TinaluxCore
    PUBLIC
        spdlog::spdlog_header_only
)
