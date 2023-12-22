cmake_minimum_required(VERSION 3.20)

project(symbolizer-python LANGUAGES CXX VERSION 0.2.0)

set(PROJECT_AUTHOR 0vercl0k)
set(PROJECT_LICENSE MIT)
set(CMAKE_CXX_STANDARD 20)

find_package(Python 3
    REQUIRED COMPONENTS Interpreter Development.Module
    OPTIONAL_COMPONENTS Development.SABIModule
)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(SYMBOLIZER_SRC_DIR ${PROJECT_ROOT_DIR}/symbolizer CACHE INTERNAL "SYMBOLIZER_SRC_DIR")
set(SYMBOLIZER_HDR_DIR ${SYMBOLIZER_SRC_DIR}/include CACHE INTERNAL "SYMBOLIZER_HDR_DIR")
set(BINDINGS_SRC_DIR ${SYMBOLIZER_SRC_DIR}/python CACHE INTERNAL "BINDINGS_SRC_DIR")

execute_process(
    COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE NB_DIR)

list(APPEND CMAKE_PREFIX_PATH "${NB_DIR}")

find_package(nanobind CONFIG REQUIRED)

nanobind_add_module(_symbolizer NB_STATIC src/symbolizer.cc)
target_include_directories(_symbolizer PRIVATE ${SYMBOLIZER_HDR_DIR})

target_link_directories(_symbolizer
    PRIVATE
        $<TARGET_PROPERTY:symbolizer,INTERFACE_LINK_DIRECTORIES>
)

target_link_libraries(_symbolizer
    PRIVATE
        $<TARGET_PROPERTY:symbolizer,INTERFACE_LINK_LIBRARIES>
)

target_link_options(_symbolizer
    PRIVATE
        $<TARGET_PROPERTY:symbolizer,INTERFACE_LINK_OPTIONS>
)

if(BUILD_PYTHON_PACKAGE)
    install(DIRECTORY symbolizer DESTINATION .)
    install(TARGETS _symbolizer LIBRARY DESTINATION symbolizer)
else()
    target_include_directories(_symbolizer PRIVATE $<TARGET_PROPERTY:symbolizer,INTERFACE_INCLUDE_DIRECTORIES>)
    install(TARGETS _symbolizer DESTINATION Bindings/Python)
    if(MSVC)
        install(FILES $<TARGET_PDB_FILE:_symbolizer> DESTINATION Bindings/Python OPTIONAL)
    endif(MSVC)
endif()
