cmake_minimum_required(VERSION 3.5)

project(debug_demo)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/linenoise
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(SRC_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/minidbg.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/linenoise/linenoise.c
)

add_executable(demo ${SRC_FILE})
