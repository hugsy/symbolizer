include(FetchContent)

FetchContent_Declare(
    Deps_Fmt
    URL https://github.com/fmtlib/fmt/releases/download/10.1.1/fmt-10.1.1.zip
    URL_HASH MD5=5b74fd3cfa02058855379da416940efe
)

FetchContent_MakeAvailable(Deps_Fmt)

message(STATUS "Using fmtlib::fmt in '${deps_fmt_SOURCE_DIR}'")

add_library(Deps::Fmt ALIAS fmt)
