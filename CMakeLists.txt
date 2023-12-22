cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION True)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

project(
    symbolizer
    LANGUAGES CXX
    VERSION 0.2.0
    DESCRIPTION "symbolizer"
    HOMEPAGE_URL https://github.com/0vercl0k/symbolizer
)

set(PROJECT_AUTHOR 0vercl0k)
set(PROJECT_LICENSE MIT)

set(CXX_STANDARD 20)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(BUILD_TOOL       "Build the executable"   ON)
option(BUILD_PYTHON_BINDINGS   "Build the Python bindings"   OFF)
option(BUILD_PYTHON_PACKAGE    "Build the Python bindings package"   OFF)

set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(symbolizer)

if(BUILD_PYTHON_BINDINGS)
    add_subdirectory(symbolizer/python)
endif()

