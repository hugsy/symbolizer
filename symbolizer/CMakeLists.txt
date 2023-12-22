cmake_minimum_required(VERSION 3.20)

project(symbolizer LANGUAGES CXX VERSION 0.2.0)

set(PROJECT_LIB_SRC_DIR ${PROJECT_ROOT_DIR}/symbolizer CACHE INTERNAL "PROJECT_LIB_SRC_DIR")
set(HEADER_DIR ${PROJECT_LIB_SRC_DIR}/include)
set(SOURCE_DIR ${PROJECT_LIB_SRC_DIR}/src)

set(LIBRARY_NAME ${PROJECT_NAME})
set(LIBRARY_AUTHOR ${PROJECT_AUTHOR})
set(LIBRARY_LICENSE ${PROJECT_LICENSE})
set(LIBRARY_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(LIBRARY_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(LIBRARY_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(LIBRARY_VERSION_RELEASE "Standalone")
set(LIBRARY_VERSION ${PROJECT_VERSION})
set(BUILD_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
set(BUILD_OS ${CMAKE_SYSTEM_NAME})

if(NOT ${BUILD_OS} STREQUAL "Windows")
    message(FATAL "${PROJECT_NAME} only works on Windows")
endif()

configure_file(
    ${HEADER_DIR}/constants.hpp.in
    ${HEADER_DIR}/constants.hpp
    NEWLINE_STYLE WIN32
)

set(FMT_INSTALL OFF)
find_package(Fmt REQUIRED)

#
# Make Symbolizer an interface
#
add_library(${PROJECT_NAME} INTERFACE)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)
target_include_directories(${PROJECT_NAME} INTERFACE ${HEADER_DIR} $<TARGET_PROPERTY:Deps::Fmt,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(${PROJECT_NAME} INTERFACE dbgeng.lib Deps::Fmt)
install(TARGETS ${PROJECT_NAME} DESTINATION Library)

#
# Build the binary
#
if(NOT ${BUILD_TOOL})
    return()
endif()

find_package(Cli11 REQUIRED)

add_executable(${PROJECT_NAME}-bin ${SOURCE_DIR}/symbolizer.cpp)
add_dependencies(${PROJECT_NAME}-bin ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME}-bin PROPERTIES CXX_STANDARD 20)

target_compile_features(${PROJECT_NAME}-bin PUBLIC cxx_std_20)

target_compile_options(${PROJECT_NAME}-bin
    PRIVATE
        $<TARGET_PROPERTY:${PROJECT_NAME},INTERFACE_COMPILE_OPTIONS>
)

target_include_directories(${PROJECT_NAME}-bin
    PRIVATE
        $<TARGET_PROPERTY:${PROJECT_NAME},INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:Deps::Cli11,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_directories(${PROJECT_NAME}-bin
    PRIVATE
        $<TARGET_PROPERTY:${PROJECT_NAME},INTERFACE_LINK_DIRECTORIES>
)

target_link_libraries(${PROJECT_NAME}-bin
    PRIVATE
        $<TARGET_PROPERTY:${PROJECT_NAME},INTERFACE_LINK_LIBRARIES>
)

target_link_options(${PROJECT_NAME}-bin
    PRIVATE
        $<TARGET_PROPERTY:${PROJECT_NAME},INTERFACE_LINK_OPTIONS> /SUBSYSTEM:Console
)

install(TARGETS ${PROJECT_NAME}-bin DESTINATION Tools)
