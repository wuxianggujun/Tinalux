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

target_link_libraries(TinaluxCore
    PUBLIC
        spdlog::spdlog_header_only
)
