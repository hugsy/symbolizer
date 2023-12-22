include(FetchContent)

FetchContent_Declare(
    Deps_Cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11
    GIT_TAG v2.3.2
)

FetchContent_MakeAvailable(Deps_Cli11)

message(STATUS "Using CLI11 in '${deps_cli11_SOURCE_DIR}'")

add_library(Deps_Cli11 INTERFACE EXCLUDE_FROM_ALL)
add_library(Deps::Cli11 ALIAS Deps_Cli11)
target_compile_features(Deps_Cli11 INTERFACE cxx_std_20)
target_include_directories(Deps_Cli11 INTERFACE ${deps_cli11_SOURCE_DIR}/include)
